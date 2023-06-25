​		[CefBrowser](https://cef-builds.spotifycdn.com/docs/stable.html?classCefBrowser.html) 和 [CefFrame](https://cef-builds.spotifycdn.com/docs/stable.html?classCefFrame.html) 对象用于向浏览器发送命令以及在回调方法中检索状态信息。每个 CefBrowser 对象都有一个表示顶级帧的主 CefFrame 对象和表示子帧的零个或多个 CefFrame 对象。例如，加载两个 iframe 的浏览器将具有三个 CefFrame 对象（顶级框架和两个 iframe）。

​		要在浏览器主框架中加载 URL，请执行以下操作：

```
browser->GetMainFrame()->LoadURL(some_url);
```

​		向后导航浏览器：

```
browser->GoBack();
```

检索主框架 HTML 内容：

```
// Implementation of the CefStringVisitor interface.
class Visitor : public CefStringVisitor {
 public:
  Visitor() {}

  // Called asynchronously when the HTML contents are available.
  virtual void Visit(const CefString& string) override {
    // Do something with |string|...
  }

  IMPLEMENT_REFCOUNTING(Visitor);
};

browser->GetMainFrame()->GetSource(new Visitor());
```

​		CefBrowser 和 CefFrame 对象同时存在于浏览器进程和呈现进程中。主机行为可以通过 CefBrowser：：GetHost（） 方法在浏览器进程中控制。例如，可以按如下方式检索窗口浏览器的本机句柄：

```
// CefWindowHandle is defined as HWND on Windows, NSView* on MacOS
// and (usually) X11 Window on Linux.
CefWindowHandle window_handle = browser->GetHost()->GetWindowHandle();
```

​		其他方法可用于历史记录导航、加载字符串和请求、发送编辑命令、检索文本/html 内容等。有关受支持方法的完整列表，请参阅类文档。