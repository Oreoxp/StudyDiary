# Duilib UI渲染流程

[TOC]

## 1. 流程简图

![img](https://static.oschina.net/uploads/space/2018/0606/224841_ONf8_3443876.png)

## 2. 自己的窗口类

​        以360浏览器demo为例, 这个窗口类是C360SafeFrameWnd，它继承了两个类，一个是窗口类CWindowWnd，另一个是消息类INotifyUI。在UI这一块我们主要关注CWindowWnd类。CWindowWnd类：这个类可以叫它窗口类，它封装了窗口的基本属性和接口，便于理解，可以把它类比为MFC中的CWnd类。

## 3. 创建窗口

WindMain中的创建流程如下：

```c++
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow)
{
	CPaintManagerUI::SetInstance(hInstance);
  CPaintManagerUI::SetResourcePath(
             CPaintManagerUI::GetInstancePath() + _T("skin"));
	CPaintManagerUI::SetResourceZip(_T("360SafeRes.zip"));
 
	HRESULT Hr = ::CoInitialize(NULL);
	if( FAILED(Hr) ) return 0;
 
	C360SafeFrameWnd* pFrame = new C360SafeFrameWnd();
	if( pFrame == NULL ) return 0;
	pFrame->Create(NULL, _T("360安全卫士"),
                 UI_WNDSTYLE_FRAME, 0L, 0, 
                 0, 800, 572);
	pFrame->CenterWindow();
	::ShowWindow(*pFrame, SW_SHOW);
 
	CPaintManagerUI::MessageLoop();
 
	::CoUninitialize();
	return 0;
}
```

这里主要在做三件事：

1. 设置资源
           SetResourcePath 函数设置资源路径，并将这个值保存在成员变量 " m_pStrResourcePath " 中，SetResourceZip 函数是设置资源包，这里是压缩成 zip 格式，文件名保存在 " m_pStrResourceZip " 中。这个 zip 压缩文件中包含了程序用到的资源，例如图片和 xml 文件，将 360SafeRes.zip解压可以看到所有的资源文件，其中 " skin.xml " 是主窗口UI配置文件。

2. 创建窗口
           这一步做了很多事情，包括 xml 文件加载、解析，UI对象创建渲染，窗口显示等。

3. 消息循环
           消息相关的有机会再后续章节再介绍。

## 4. XML 解析流程

​        上一步中说道了创建窗口，我们知道 WM_CREATE 是常规意义上窗口收到的第一个消息，在 windows 程序开发中我们一般在这条消息中做一些初始化的工作。为了处理消息，需要实现虚函数HandleMessage:

```c++
LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lRes = 0;
		BOOL bHandled = TRUE;
		switch( uMsg ) {
		case WM_CREATE:        
						lRes = OnCreate(uMsg, wParam, lParam, bHandled);
						break;
		case WM_CLOSE:         
						lRes = OnClose(uMsg, wParam, lParam, bHandled); 
						break;
		case WM_DESTROY:       
						lRes = OnDestroy(uMsg, wParam, lParam, bHandled); 
						break;
		case WM_NCACTIVATE:    
						lRes = OnNcActivate(uMsg, wParam, lParam, bHandled); 
						break;
		case WM_NCCALCSIZE:    
						lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled); 
						break;
		case WM_NCPAINT:       
						lRes = OnNcPaint(uMsg, wParam, lParam, bHandled); 
						break;
		case WM_NCHITTEST:     
						lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); 
						break;
		case WM_SIZE:          
						lRes = OnSize(uMsg, wParam, lParam, bHandled); 		
						break;
		case WM_GETMINMAXINFO: 
						lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); 
						break;
		case WM_SYSCOMMAND:    
						lRes = OnSysCommand(uMsg, wParam, lParam, bHandled); 
						break;
		default:
						bHandled = FALSE;
		}
		if( bHandled ) return lRes;
		if( m_pm.MessageHandler(uMsg, wParam, lParam, lRes) ) 
					return lRes;
  
		return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	}
```

​        这样就可以接收我们想要处理的消息了。再看 OnCreate() 函数，这里会进行一系列初始化工作：

```c++
LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
	styleValue &= ~WS_CAPTION;
	::SetWindowLong(*this, GWL_STYLE, 
					styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
 
	m_pm.Init(m_hWnd);
	CDialogBuilder builder;
	CDialogBuilderCallbackEx cb;
	CControlUI* pRoot = builder.Create(_T("skin.xml"), 
																		 (UINT)0,  &cb, &m_pm);
	ASSERT(pRoot && "Failed to parse XML");
	m_pm.AttachDialog(pRoot);
	m_pm.AddNotifier(this);
 
	Init();
	return 0;
}
```

​		这里又出现了一个新的类"CDialogBuilder"，主要功能是什么呢？看它的名字大概就是对话框创建类的意思。首先通过 GetWindowLong 和 SetWindowLong 两个函数改变了窗口风格，**<u>去掉了 " WS_CAPTION " 风格，即去掉了标题栏和边框</u>**，**<u>然后加上了风格"WS_CLIPSIBLINGS"和"WS_CLIPCHILDREN"，简单来说作用是减少绘制。</u>**

​		接下来进入到 CDialogBuilder::Create() 函数，这里面会加载 xml 文件，这里又引出一个新类 " CMarkup " ，这个类完成 xml的 加载与解析等工作，具体怎么解析的我也就大概看了下，不好发表看法，以后有机会补上。回到CDialogBuilder::Create()，最开始解析出所有的顶层节点属性，包括 " Font "、" Default "、" Window "等，这里主要是看" Window "，即主窗口。

#### 1、解析主窗口属性

```c++
pstrClass = root.GetName();
if( _tcsicmp(pstrClass, _T("Window")) == 0 ) {
    if( pManager->GetPaintWindow() ) {
        int nAttributes = root.GetAttributeCount();
        for( int i = 0; i < nAttributes; i++ ) {
            pstrName = root.GetAttributeName(i);
            pstrValue = root.GetAttributeValue(i);
            pManager->SetWindowAttribute(pstrName, pstrValue);
        }
    }
}
```

​		进入函数 CPaintManagerUI::SetWindowAttribute()，这里会设置主窗口的所有属性，像size(大小)，sizebox(边框)，caption(标题栏)等。

#### (2) 创建 UI 控件对象

​        解析完主窗口属性后，就开始解析所有的控件，进入函数CDialogBuilder::_Parse():

```c++
SIZE_T cchLen = _tcslen(pstrClass);
switch( cchLen ) {
case 4:
    if( _tcsicmp(pstrClass, DUI_CTR_EDIT) == 0 )                  pControl = new CEditUI;
    else if( _tcsicmp(pstrClass, DUI_CTR_LIST) == 0 )             pControl = new CListUI;
    else if( _tcsicmp(pstrClass, DUI_CTR_TEXT) == 0 )             pControl = new CTextUI;
    else if( _tcsicmp(pstrClass, DUI_CTR_TREE) == 0 )             pControl = new CTreeViewUI;
	else if( _tcsicmp(pstrClass, DUI_CTR_HBOX) == 0 )             pControl = new CHorizontalLayoutUI;
	else if( _tcsicmp(pstrClass, DUI_CTR_VBOX) == 0 )             pControl = new CVerticalLayoutUI;
    break;
case 5:
```

感兴趣的可以自己看下源码，这里只截取片段。功能就是根据节点名创建对应的对象，例如：

![img](https://static.oschina.net/uploads/space/2018/0606/225609_zdDA_3443876.png)

xml中配置的节点名是"Text"，c++中对应宏定义为:

![img](https://static.oschina.net/uploads/space/2018/0606/225626_i5Du_3443876.png)

字符创长度为 4，进入分支"case 4"，然后根据节点名创建对应的对象"CTextUI"。**<u>如果有子节点，那么嵌套调用函数_Parse()自己：</u>**

![img](https://static.oschina.net/uploads/space/2018/0606/225639_B0B5_3443876.png)

### 3、 解析控件属性

​		创建了 UI 对象后，接着就是设置 UI 属性：

​		每个控件都有自己的属性，且每个控件对属性可能都有自己独特的理解，所以每个控件都重写了虚函数 SetAttribute()，例如按钮的 SetAttribute() 如下：

```cpp
// Init default attributes
if( pManager ) {
    pControl->SetManager(pManager, NULL, false);
    LPCTSTR pDefaultAttributes = pManager->GetDefaultAttributeList(pstrClass);
    if( pDefaultAttributes ) {
        pControl->SetAttributeList(pDefaultAttributes);
    }
}
// Process attributes
if( node.HasAttributes() ) {
    // Set ordinary attributes
    int nAttributes = node.GetAttributeCount();
    for( int i = 0; i < nAttributes; i++ ) {
        pControl->SetAttribute(node.GetAttributeName(i), node.GetAttributeValue(i));
    }
}
```

到这里就已经完成了 xml 的解析和 UI 对象的创建等一系列功能了。

```c++
void CButtonUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
	if( _tcscmp(pstrName, _T("normalimage")) == 0 ) SetNormalImage(pstrValue);
	else if( _tcscmp(pstrName, _T("hotimage")) == 0 ) SetHotImage(pstrValue);
	else if( _tcscmp(pstrName, _T("pushedimage")) == 0 ) SetPushedImage(pstrValue);
	else if( _tcscmp(pstrName, _T("focusedimage")) == 0 ) SetFocusedImage(pstrValue);
	else if( _tcscmp(pstrName, _T("disabledimage")) == 0 ) SetDisabledImage(pstrValue);
	else if( _tcscmp(pstrName, _T("foreimage")) == 0 ) SetForeImage(pstrValue);
	else if( _tcscmp(pstrName, _T("hotforeimage")) == 0 ) SetHotForeImage(pstrValue);
	else if( _tcscmp(pstrName, _T("fivestatusimage")) == 0 ) SetFiveStatusImage(pstrValue);
	else if( _tcscmp(pstrName, _T("fadedelta")) == 0 ) SetFadeAlphaDelta((BYTE)_ttoi(pstrValue));
	else if( _tcscmp(pstrName, _T("hotbkcolor")) == 0 )
	{
		if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
		LPTSTR pstr = NULL;
		DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
		SetHotBkColor(clrColor);
	}
	else if( _tcscmp(pstrName, _T("hottextcolor")) == 0 )
	{
		if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
		LPTSTR pstr = NULL;
		DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
		SetHotTextColor(clrColor);
	}
	else if( _tcscmp(pstrName, _T("pushedtextcolor")) == 0 )
	{
		if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
		LPTSTR pstr = NULL;
		DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
		SetPushedTextColor(clrColor);
	}
	else if( _tcscmp(pstrName, _T("focusedtextcolor")) == 0 )
	{
		if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
		LPTSTR pstr = NULL;
		DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
		SetFocusedTextColor(clrColor);
	}
	else CLabelUI::SetAttribute(pstrName, pstrValue);
}
```



## 5. 渲染

​		经过上面几个步骤，完成了 xml 文件的解析和 UI 对象的创建，接下来要做的就是把控件绘制到窗口上。Duilib 中的控件分两种，一是容器，二是普通控件，且都是从类 " CControlUI " 继承而来，下面配一张简图说明：

![img](https://static.oschina.net/uploads/space/2018/0606/225856_0xqr_3443876.png)

​		最常用的容器有两个类，"CVerticalLayoutUI"和"CHorizontalLayoutUI"，即纵向布局和横向布局。

​		回过头来说绘制吧。windows 的绘制一般都放在 WM_PAINT 消息中进行，Duilib 也一样，进入 CPaintManagerUI 类的消息处理函数 MessageHandler ，然后看 WM_PAINT 分支，在这里会绘制整个窗口的内容。找到下面这一行代码:

![img](https://static.oschina.net/uploads/space/2018/0606/225910_dniJ_3443876.png)

m_pRoot 要么是一个 "CVerticalLayoutUI" 对象，要么就是一个 "CHorizontalLayoutUI" 对象，为什么呢，因为顶层肯定是一个容器，不然怎么包含子控件。跟进到 Paint 函数看一下：

```c++
bool CControlUI::Paint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
	if (pStopControl == this) return false;
	if( !::IntersectRect(&m_rcPaint, &rcPaint, &m_rcItem) ) return true;
	if( OnPaint ) {
		if( !OnPaint(this) ) return true;
	}
	if (!DoPaint(hDC, rcPaint, pStopControl))
		return false;
    if( m_pCover != NULL ) return m_pCover->Paint(hDC, rcPaint);
    return true;
}
```

​		看到一个函数 DoPaint()，有前面知道，此时的 this 就是上面的两种布局对象中的一个，所以这个 DoPaint 会调到 CContainerUI 的 DoPaint，截取部分代码：

```c++
bool CContainerUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
	{
		RECT rcTemp = { 0 };
		if( !::IntersectRect(&rcTemp, &rcPaint, &m_rcItem) ) return true;
 
		CRenderClip clip;
		CRenderClip::GenerateClip(hDC, rcTemp, clip);
		CControlUI::DoPaint(hDC, rcPaint, pStopControl);
 
		if( m_items.GetSize() > 0 ) {
			RECT rc = m_rcItem;
			rc.left += m_rcInset.left;
			rc.top += m_rcInset.top;
			rc.right -= m_rcInset.right;
			rc.bottom -= m_rcInset.bottom;
			if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) rc.right -= m_pVerticalScrollBar->GetFixedWidth();
			if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) rc.bottom -= m_pHorizontalScrollBar->GetFixedHeight();
 
			if( !::IntersectRect(&rcTemp, &rcPaint, &rc) ) {
				for( int it = 0; it < m_items.GetSize(); it++ ) {
					CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
					if( pControl == pStopControl ) return false;
					if( !pControl->IsVisible() ) continue;
					if( !::IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos()) ) continue;
					if( pControl->IsFloat() ) {
						if( !::IntersectRect(&rcTemp, &m_rcItem, &pControl->GetPos()) ) continue;
                        if( !pControl->Paint(hDC, rcPaint, pStopControl) ) return false;
					}
				}
			}
```

​        主要是做两件事。一是绘制自己，二是绘制子控件，如果子控件也是布局，那么重复此过程，这样就可以对所用控件进行绘制。

​        控件的具体绘制动作在 CControlUI::DoPaint() 中：

```cpp
bool CControlUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    // 绘制循序：背景颜色->背景图->状态图->文本->边框
    if( m_cxyBorderRound.cx > 0 || m_cxyBorderRound.cy > 0 ) {
        CRenderClip roundClip;
        CRenderClip::GenerateRoundClip(hDC, m_rcPaint,  m_rcItem, m_cxyBorderRound.cx, m_cxyBorderRound.cy, roundClip);
        PaintBkColor(hDC);
        PaintBkImage(hDC);
        PaintStatusImage(hDC);
        PaintText(hDC);
        PaintBorder(hDC);
    }
    else {
        PaintBkColor(hDC);
        PaintBkImage(hDC);
        PaintStatusImage(hDC);
        PaintText(hDC);
        PaintBorder(hDC);
    }
    return true;
}
```

​        这里是具体的绘制操作，再跟进到PaintBkColor，PaintBkImage这些函数，又会引出一个新类 " CRenderEngine " ，顾名思义就是渲染引擎。这个类封装了所有的绘图功能，像绘制矩形，绘制椭圆等。到这里整个渲染过程就已经完成了，写的比较简洁，对照着代码看相信还是很容易理清楚的。

​		最后还有一个小细节双缓冲绘图，在 windows 编程中经常会提到这个概念。双缓冲是用来解决闪烁问题的一种方式，当然仅仅只是众多方式中的一种，因为屏幕闪烁跟很多因素相关，双缓冲解决的是由屏幕刷新频率造成的闪烁，即绘制动作不是在同一个刷新周期内完成，给人的感觉有可能就是"闪烁"。**<u>双缓冲简单来说就是先将屏幕内容绘制到内存 DC 中，也就是先在内存中把要绘制的内容准备好，然后再一次全部贴到屏幕上，而不是每一个控件各自直接在屏幕上绘制。</u>**