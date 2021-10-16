# DuiLib消息处理剖析

分为几个大部分：

1. 控件
2. 容器（本质也是控件）
3. UI构建解析器（XML解析）
4. 窗体管理器（消息循环，消息映射，消息处理，窗口管理等）
5. 渲染引擎







Win32消息路由如下：

1. 消息产生。
2. 系统将消息排列到其应该排放的线程消息队列中。
3. 线程中的消息循环调用GetMessage（or PeekMessage）获取消息。
4. 传送消息TranslateMessage and DispatchMessage to 窗口过程（Windows procedure）。
5. 在窗口过程里进行消息处理

​        我们看到消息经过几个步骤，DuiLib架构可以让你在某些步骤间进行消息过滤。首先，第1、2和3步骤，DuiLib 并不关心。DuiLib 对消息处理集中在 **CPaintManagerUI** 类中（也就是上面提到的**窗体管理器**）。DuiLib 在发送到窗口过程的前和后都进行了消息过滤。

​		DuiLib 的消息渠，也就是所谓的消息循环在 **CPaintManagerUI::MessageLoop()** 或者 **CWindowWnd::ShowModal()** 中实现。俩套代码的核心基本一致，以MessageLoop为例：

```c++
void CPaintManagerUI::MessageLoop()
{
    MSG msg = { 0 };
    while( ::GetMessage(&msg, NULL, 0, 0) ) {
        // CPaintManagerUI::TranslateMessage进行消息过滤
        if( !CPaintManagerUI::TranslateMessage(&msg) ) {
            ::TranslateMessage(&msg);
            try{
            ::DispatchMessage(&msg);
            } catch(...) {
                DUITRACE(_T("EXCEPTION: %s(%d)\n"), 
                         __FILET__, 
                         __LINE__);
                #ifdef _DEBUG
                throw "CPaintManagerUI::MessageLoop";
                #endif
            }
        }
    }
}
```

​		3 和 4 之间，DuiLib 调用 **CPaintManagerUI::TranslateMessage** 做了过滤，类似 MFC 的 **PreTranlateMessage**。

​		想象一下，如果不使用这套消息循环代码，我们如何能做到在消息发送到窗口过程前进行常规过滤（Hook等拦截技术除外）？答案肯定是做不到。因为那段循环代码你是无法控制的。CPaintManagerUI::TranslateMessage 将无法被调用，所以，可以看到 DuiLib 中几乎所有的 demo 在创建完消息后，都调用了这俩个消息循环函数。下面是 TranslateMessage 代码：

```c++
bool CPaintManagerUI::TranslateMessage(const LPMSG pMsg) {
    // Pretranslate Message takes care of system-wide messages, such as
    // tabbing and shortcut key-combos. We'll look for all messages for
    // each window and any child control attached.
    UINT uStyle = GetWindowStyle(pMsg->hwnd);
    UINT uChildRes = uStyle & WS_CHILD;    
    LRESULT lRes = 0;
    if (uChildRes != 0) {// 判断子窗口还是父窗口
        HWND hWndParent = ::GetParent(pMsg->hwnd);
        for( int i = 0; i < m_aPreMessages.GetSize(); i++ ) {
            CPaintManagerUI* pT = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);        
            HWND hTempParent = hWndParent;
            while(hTempParent) {
                if(pMsg->hwnd == pT->GetPaintWindow() || hTempParent == pT->GetPaintWindow()) {
                    if (pT->TranslateAccelerator(pMsg))
                        return true;
                    // 这里进行消息过滤
                    if( pT->PreMessageHandler(pMsg->message, pMsg->wParam, pMsg->lParam, lRes) ) 
                        return true;
                    return false;
                }
                hTempParent = GetParent(hTempParent);
            }
        }
    } else {
        for( int i = 0; i < m_aPreMessages.GetSize(); i++ ) {
            CPaintManagerUI* pT = static_cast<CPaintManagerUI*>(m_aPreMessages[i]);
            if(pMsg->hwnd == pT->GetPaintWindow()) {
                if (pT->TranslateAccelerator(pMsg))
                    return true;
                if( pT->PreMessageHandler(pMsg->message, pMsg->wParam, pMsg->lParam, lRes) ) 
                    return true;
                return false;
            }
        }
    }
    return false;
}


bool CPaintManagerUI::PreMessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& /*lRes*/) {
    for( int i = 0; i < m_aPreMessageFilters.GetSize(); i++) {
        bool bHandled = false;
        LRESULT lResult = static_cast<IMessageFilterUI*>
		  							(m_aPreMessageFilters[i])->
          					MessageHandler(uMsg, 
																	 wParam, 
																	 lParam, 
																	 bHandled); // 这里调用接口 IMessageFilterUI::MessageHandler 来进行消息过滤
        if( bHandled ) {
            return true;
        }
}
  …… ……
  return false;
}
```

​		在发送到窗口过程前，有一个过滤接口：IMessageFilterUI，此接口只有一个成员：MessageHandler，我们的窗口类要提前过滤消息，只要实现这个IMessageFilterUI，调用 **CPaintManagerUI::AddPreMessageFilter**，将我们的窗口类实例指针添加到 **CPaintManagerUI::m_aPreMessageFilters** 数组中。当消息到达窗口过程之前，就会会先调用我们的窗口类的成员函数：MessageHandler。

下面是AddPreMessageFilter代码：

```c++
bool CPaintManagerUI::AddPreMessageFilter(IMessageFilterUI* pFilter)
{
    // 将实现好的接口实例，保存到数组 m_aPreMessageFilters 中。
    ASSERT(m_aPreMessageFilters.Find(pFilter)<0);
    return m_aPreMessageFilters.Add(pFilter);
}
```

​		我们从函数 **CPaintManagerUI::TranslateMessage** 代码中能够看到，这个过滤是在大循环：

​		for( int i = 0; i < m_aPreMessages.GetSize(); i++ )

中被调用的。如果m_aPreMessages.GetSize()为0，也就不会调用过滤函数。从代码中追溯其定义：

static CStdPtrArray m_aPreMessages;

是个静态变量，MessageLoop，TranslateMessage等也都是静态函数。其值在CPaintManagerUI::Init中被初始化：

```c++
void CPaintManagerUI::Init(HWND hWnd)
{
    ASSERT(::IsWindow(hWnd));
    // Remember the window context we came from
    m_hWndPaint = hWnd;
    m_hDcPaint = ::GetDC(hWnd);
    // We'll want to filter messages globally too
    m_aPreMessages.Add(this);
}
```





## 2.控件消息

消息如何进入控件的呢？在xml中配置了一个Buttom，点击这个按钮，按钮是如何捕捉到消息的呢？下面接着分析。

### (1) UI管理器CPaintManagerUI

​		在主窗口"C360SafeFrameWnd"中定义了一个成员变量
​		 " CPaintManagerUI     m_pm"，顾名思义就叫它 UI 管理器，管理 U I消息。

​		在**消息入口**这一节中，我们看到消息路由到了函数 " HandleMessage"  ，接着看这个函数，可以看到有下面的调用：

`if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) `
		`return lRes;`

即将消息传入进了 UI 管理器 " CPaintManagerUI::MessageHandler " ，这里管理所有的UI消息，然后将相应的消息传递给对应的控件，这里就解释了开头的疑问，消息是如何传递到 UI 控件的。

### (2) 消息通知监听

在 OnCreate 函数中添加事件监听 m_pm.AddNotifier(this);

```c++
 bool CPaintManagerUI::AddNotifier(INotifyUI* pNotifier)
 
{
 
if (pNotifier == NULL) return false;
 
ASSERT(m_aNotifiers.Find(pNotifier)<0);
 
return m_aNotifiers.Add(pNotifier);
 
}
 
```

分发消息通知*CPaintManagerUI::SendNotify*，向添加了监听的窗口发送消息通知。

### (3) 通知处理

重写虚函数*Notify*，在这里接收消息事件，处理自己的消息事件。

以单击按钮为例，看看是如何流程：

CPaintManagerUI::MessageHandler-->

pClick->Event--> 分发事件

CControlUI::Event-->

CButtonUI::DoEvent--> 响应事件

CButtonUI::Activate-->

CPaintManagerUI::SendNotify-->

CPaintManagerUI::SendNotify--> 分发消息通知

C360SafeFrameWnd::Notify--> 响应消息通知
