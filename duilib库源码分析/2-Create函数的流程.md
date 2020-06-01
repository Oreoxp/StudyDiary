## 窗体构建流程

下面分析第五行代码pMainWndFrame->Create()做了什么

```C++
pMainWndFrame->Create(nullptr, MainWndFrame::kClassName, UI_WNDSTYLE_DIALOG, 0);//创建窗体
```

此处下断点后F11跟进可以看到Create函数是基类CWindowWnd的成员函数。这个函数并只有6行代码。

```c++
HWND CWindowWnd::Create(HWND hwndParent, LPCTSTR pstrName, DWORD dwStyle, DWORD dwExStyle, int x, int y, int cx, int cy, HMENU hMenu)
{
	//GetSuperClassName()这个虚函数在CWindowWnd类中暂时返回的是NULL值
	//RegisterSuperclass()函数是针对控件子类化使用的
    if( GetSuperClassName() != NULL && !RegisterSuperclass() ) 
		return NULL;
	//RegisterWindowClass()成员函数只做了一件事情，注册窗口类。
	//其中需要知道的是窗口类的消息循环为CWindowWnd::__WndProc静态函数。
    if( GetSuperClassName() == NULL && !RegisterWindowClass() ) 
		return NULL;
	//第五行代码就是调用来创建窗口了。
	//但是Windows会在CreateWindowEx函数中产生一个条WM_CREATE的消息事件。所以我们可以再第六行下一个断点和CWindowWnd::__WndProc中函数头下一个断点。
    m_hWnd = ::CreateWindowEx(
		dwExStyle, 
		GetWindowClassName(), pstrName, dwStyle, x, y,
		cx, cy, hwndParent, hMenu, CPaintManagerUI::GetInstance(), this);
    ASSERT(m_hWnd!=NULL);
    return m_hWnd;
}
```

CWindowWnd::__WndProc函数：

```C++
LRESULT CALLBACK CWindowWnd::__WndProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWindowWnd* pThis = NULL;
    if( uMsg == WM_NCCREATE )
    {
        //........
    }
    else
    {
        //........
        if( uMsg == WM_NCDESTROY && pThis != NULL )
        {
            //........
        }
    }
    if( pThis != NULL )
    {
        return pThis->HandleMessage(uMsg, wParam, lParam);////////这里
    }
    //........
}
```

在函数中并没有找到需要的WM_CREATE消息而且和窗体消息相关的只有两个*NCCREATE*非客户去窗体创建和*WM_NCDESTROY*非客户区窗体销毁。

但是在17行可以看到pThis->HandleMessage(uMsg, wParam, lParam);函数中找到WM_CREATE消息事件的处理。因为HandleMessage()是虚函数且CFrameWindowWnd中实现了的HandleMessage()覆盖所以你需要单步进去或者直接找到CFrameWindowWnd的HandleMessage实现WM_CREATE的过程处理中下断点！

可以看到WindowImplBase实现的消息处理函数，具体处理过程见别的章节，此处先单步进入WM_CREATE

```C++
LRESULT WindowImplBase::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	switch (uMsg)
	{//WM_CREAT在这里
	case WM_CREATE:			lRes = OnCreate(uMsg, wParam, lParam, bHandled); break;
	case WM_CLOSE:			lRes = OnClose(uMsg, wParam, lParam, bHandled); break;
	case WM_DESTROY:		lRes = OnDestroy(uMsg, wParam, lParam, bHandled); break;
#if defined(WIN32) && !defined(UNDER_CE)
	case WM_NCACTIVATE:		lRes = OnNcActivate(uMsg, wParam, lParam, bHandled); break;
	case WM_NCCALCSIZE:		lRes = OnNcCalcSize(uMsg, wParam, lParam, bHandled); break;
	case WM_NCPAINT:		lRes = OnNcPaint(uMsg, wParam, lParam, bHandled); break;
	case WM_NCHITTEST:		lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
	case WM_GETMINMAXINFO:	lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); break;
	case WM_MOUSEWHEEL:		lRes = OnMouseWheel(uMsg, wParam, lParam, bHandled); break;
#endif
	case WM_SIZE:			lRes = OnSize(uMsg, wParam, lParam, bHandled); break;
	case WM_CHAR:		lRes = OnChar(uMsg, wParam, lParam, bHandled); break;
	case WM_SYSCOMMAND:		lRes = OnSysCommand(uMsg, wParam, lParam, bHandled); break;
	case WM_KEYDOWN:		lRes = OnKeyDown(uMsg, wParam, lParam, bHandled); break;
	case WM_KILLFOCUS:		lRes = OnKillFocus(uMsg, wParam, lParam, bHandled); break;
	case WM_SETFOCUS:		lRes = OnSetFocus(uMsg, wParam, lParam, bHandled); break;
	case WM_LBUTTONUP:		lRes = OnLButtonUp(uMsg, wParam, lParam, bHandled); break;
	case WM_LBUTTONDOWN:	lRes = OnLButtonDown(uMsg, wParam, lParam, bHandled); break;
	case WM_MOUSEMOVE:		lRes = OnMouseMove(uMsg, wParam, lParam, bHandled); break;
	case WM_MOUSEHOVER:	lRes = OnMouseHover(uMsg, wParam, lParam, bHandled); break;
	default:				bHandled = FALSE; break;
	}
	if (bHandled) return lRes;

	lRes = HandleCustomMessage(uMsg, wParam, lParam, bHandled);
	if (bHandled) return lRes;

	if (m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes))
		return lRes;
	return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}
```

以下就是为处理WM_CREATE消息的处理函数了：

```C++
LRESULT WindowImplBase::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
	styleValue &= ~WS_CAPTION;
	::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	RECT rcClient;
	::GetClientRect(*this, &rcClient);
	::SetWindowPos(*this, NULL, rcClient.left, rcClient.top, rcClient.right - rcClient.left, \
		rcClient.bottom - rcClient.top, SWP_FRAMECHANGED);
	m_PaintManager.Init(m_hWnd);
    //上面这行，调用的CPaintManagerUI初始化功能，这个函数负责Remove一些内部东西和设置参数传递进去的窗口句柄，以及创建一个DC。移除什么东西看名字大概猜为：字体、图片、默认属性列表、窗口自定义属性、选项组、定时器。这些东西怎么定义的先不管。先过，用到了在仔细琢磨。
	m_PaintManager.AddPreMessageFilter(this);
	//消息筛选？

	CDialogBuilder builder;
    //这行是构造了一个对话框生成器（CDialogBuilder）类。这个类并没有继承任何基类。这个类看头文件挺简单的，只有二个Create()、GetMarkup()、GetLastErrorMessage()、GetLastErrorLocation()、_Parse()。属性有CMarkup m_xml、IDialogBuilderCallback m_pCallback、LPCTSTR m_pstrtype。
    //此处由对XML进行解析，详情看其他章节（XML解析引擎.md）
    //总结： CDialogBuilder类负责解析XML和按照XML文档所定义的组装控件需要使用的属性和组装控件。组装后的信息全部存储在CPaintManagerUI中。
	CDuiString strResourcePath=m_PaintManager.GetResourcePath();
	if (strResourcePath.IsEmpty())
	{
		strResourcePath=m_PaintManager.GetInstancePath();
		strResourcePath+=GetSkinFolder().GetData();
	}
	m_PaintManager.SetResourcePath(strResourcePath.GetData());
	switch(GetResourceType())//类型选择  很长 筛选了一些
	{
	case UILIB_ZIP://--
		break;
	case UILIB_ZIPRESOURCE://--
		break;
	}

	CControlUI* pRoot=NULL;//加载XML资源
	if (GetResourceType()==UILIB_RESOURCE)
	{
		STRINGorID xml(_ttoi(GetSkinFile().GetData()));
		pRoot = builder.Create(xml, _T("xml"), this, &m_PaintManager);
	}
	else
		pRoot = builder.Create(GetSkinFile().GetData(), (UINT)0, this, &m_PaintManager);
	ASSERT(pRoot);
	if (pRoot==NULL)
	{
		MessageBox(NULL,_T("加载资源文件失败"),_T("Duilib"),MB_OK|MB_ICONERROR);
		ExitProcess(1);
		return 0;
	}
    //下面几行做的是m_PaintManager.AttachDialog()返回的Root节点，由成员函数CControlUI* m_pRoot;保存，除了保存Root节点，还对Root节点内所有的控件进行初始化操作。
    //m_PaintManager.AddNotifier(this);将当前窗体添加到消息通知中。由CDuiPtrArray m_aNotifiers;保存通知窗体列表。需要接受通知需要继承INotifyUI接口并且实现纯虚函数Notify()。
	m_PaintManager.AttachDialog(pRoot);
	m_PaintManager.AddNotifier(this);//添加通知

	InitWindow();//这是个空函数，，
	return 0;
}
```

之后就生成了窗体啦，，，，，

下面分两步再详细讲解

1--------------------由XML到控件的过程----->5-从XML读取属性.md

2--------------------消息部分--------------------->3-按键消息与Notify消息过程.md   4-控件消息响应处理.md