# Qt消息机制



QEventDispatcherWin32：
注册窗口类别，并创建一个隐藏窗口 （QEventDispatcherWin32_Internal_WidgetXXXX）
窗口的回调函数 qt_internal_proc()
安装WH_GETMESSAGE类型的钩子函数 qt_GetMessageHook()

```c++
bool QEventDispatcherWin32::processEvents(
  QEventLoop::ProcessEventsFlags flags);
if(!filterNativeEvent(
  			QByteArrayLiteral("windows_generic_MSG"), 
  												&msg, 
  												0))  
//与上面的消息循环：的while一样 、得到过滤所有消息
{
	TranslateMessage(&msg);//转换消息
	DispatchMessage(&msg); //分发消息
}
```

DispatchMessage(&msg); //分发消息
分发消息的或回调函数、这个与 windows 程序的 CALLBACK WndProc 一样
`LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)`





## 消息全局通知事件

另外我们都知道我们开发的系统、所有的消息都会到这个类QApplication::notify事件过滤所有的事件：
其实QApplication继承QGuiApplication类；

```c++
bool QGuiApplication::notify(QObject *object, QEvent *event) {
    if (object->isWindowType())
       QGuiApplicationPrivate::
       sendQWindowEventToQPlatformWindow(
       					static_cast<QWindow *>(object), 
      					event);
    return QCoreApplication::notify(object, event);
}
```



## 消息的组装

​		下面是所有的消息类型处理：在我们开发的系统中别人使用processEvent发送消息是比较高效的。
​		原因：这个消息直达经过的处理的少。

`void QGuiApplicationPrivate::processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e)`



## 常用的消息过滤事件

​		通过上面组装的消息最终到达、此处。经过这些之后 : 之前的windows消息现在都已映射为 QT 类型的消息、进入 Qt 消息处理机制中来、才有了我们的消息过滤等等消息事件键盘鼠标等等；









