先从一个最简单的DEMO开始，删除掉不必要的就可以开始F10大法了。下面是main函数分析，遇到需要详细分析的会另起文档

下面是MAIN函数：

```c++
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。
	CPaintManagerUI::SetInstance(hInstance);
	CPaintManagerUI::SetCurrentPath(CPaintManagerUI::GetInstancePath());
	CPaintManagerUI::SetResourcePath(_T("theme"));


	MainWndFrame* pMainWndFrame = new MainWndFrame;
	pMainWndFrame->Create(nullptr, MainWndFrame::kClassName, UI_WNDSTYLE_DIALOG, 0);
	pMainWndFrame->CenterWindow();
	pMainWndFrame->ShowWindow();
	CPaintManagerUI::MessageLoop();

	if (nullptr != pMainWndFrame)
	{
		delete pMainWndFrame;
	}

    return 0;
}
```

从Main 1~3行调用的是的两个静态成员变量：

```c++
//全局静态，设置窗口实例句柄
CPaintManagerUI::SetInstance(hInstance);
//设置资源路径，且CPaintManagerUI::GetInstancePath()返回的是当前EXE程序所在的完整路径
CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());
//设置资源文件夹
CPaintManagerUI::SetResourcePath(_T("theme"));
```

接下来看到开始new程序的界面类以及对界面的创建、居中、显示（详见 2-Create函数的流程.md）：

```c++
	MainWndFrame* pMainWndFrame = new MainWndFrame;//UI窗体类
	pMainWndFrame->Create(nullptr, MainWndFrame::kClassName, UI_WNDSTYLE_DIALOG, 0);//创建窗体
	pMainWndFrame->CenterWindow();//窗体居中
	pMainWndFrame->ShowWindow();//显示窗体
```

接下来是消息循环（详见 3-按键消息与Notify消息过程.md    4-控件消息响应处理.md）：

```C++
CPaintManagerUI::MessageLoop();
```

消息循环会一直用GetMessage()获取消息当跳出来后就可以delete UI了