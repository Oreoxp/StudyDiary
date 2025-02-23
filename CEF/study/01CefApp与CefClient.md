[TOC]

​		CEF3 使用多个进程运行。处理窗口创建、绘制和网络访问的主要进程称为**浏览器进程**。这通常与宿主应用程序的进程相同，大多数应用程序的逻辑将在浏览器进程中运行。使用Blink引擎渲染HTML和JavaScript执行在单独的**渲染进程**中发生。一些应用程序逻辑（如JavaScript绑定和DOM访问）也将在渲染进程中运行。默认进程模型将为每个唯一源地址（scheme+domain）运行一个新的渲染进程。其他进程将根据需要生成，例如处理Flash等插件的**插件进程**和处理加速合成的**GPU进程**。综合上述文档，我们整理一下：

浏览器进程（Browser Process）：

- 窗口创建、绘制
- 网络访问
- ......

渲染进程（Renderer Process）：

- 通过Blink引擎渲染HTML
- JavaScript执行（V8引擎）
- ......

​        需要注意的是，**浏览器进程**中会进行窗口绘制，并不是指绘制HTML内容，而是承载网页内容的那个窗体壳，同样**渲染进程**也不是用来创建窗体的进程。接下来，将以官方CefSimple Demo源码入手，逐步介绍 CEF 的概念。

**<u>cefsimple_win.cc</u>**

```cpp
// ......
// Entry point function for all processes.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
// ......

  // CEF应用程序会创建多个子进程（渲染render，插件plugin，GPU处理，等等）但是会共用一个可执行程序。以下的函数会检查命令行并且，如果确认是一个子进程，那么会执行相关的逻辑。
  int exit_code = CefExecuteProcess(main_args, nullptr, sandbox_info);
  if (exit_code >= 0) {
    // The sub-process has completed so return here.
    return exit_code;
  }

// ......

  // SimpleApp implements application-level callbacks for the browser process.
  // It will create the first browser instance in OnContextInitialized() after
  // CEF has initialized.
  CefRefPtr<SimpleApp> app(new SimpleApp);

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get(), sandbox_info);

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  // called.
  CefRunMessageLoop();

  // Shut down CEF.
  CefShutdown();

  return 0;
}
```

```c++
///
// 该函数应当在应用程序的入口函数处被调用，用以执行一个子进程。它可以用于执行一个可执行程序来启动一个子进程，该可执行程序可以是当前的浏览器客户端可执行程序（默认行为）或是通过设置CefSettings.browser_subprocess_path指定路径的可执行程序。如果被调用用于浏览器进程（在启动命令行中没有"type"参数），该函数会立刻返回-1。如果被调用时识别为子进程，该函数将会阻塞直到子进程退出并且返回子进程退出的返回码。application参数可以为空（null）。windows_sandbox_info参数只能在Windows上使用或设置为NULL（详见cef_sandbox_win.h
///
/*--cef(api_hash_check,optional_param=application,
        optional_param=windows_sandbox_info)--*/
int CefExecuteProcess(const CefMainArgs& args,
                      CefRefPtr<CefApp> application,
                      void* windows_sandbox_info);
```

​		从 **CefExecuteProcess** 这个函数我们不难推断出，CEF 在以多进程架构下启动的时候，会多次启动自身可执行程序。启动的时候，会通过命令行参数传入某些标识，由 **CefExecuteProcess** 内部进行判断。如果是主进程，则该函数立刻返回 -1，程序会继续执行下去，那么后续继续运行的代码全部都运行在主进程中；如果是子进程（渲染进程等），那么该函数会阻塞住，直到子进程结束后，该函数会返回一个大于等于 0 的值，并在 main 函数直接返回，进而退出。

![img](https://pic3.zhimg.com/80/v2-a4263dd145a1fd6596037776feed12da_720w.webp)

对 CefExecuteProcess 分析就到这里，细节可以阅读[官方文档](https://link.zhihu.com/?target=https%3A//bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage.md%23markdown-header-entry-point-function)，我们继续后续的代码分析：

```cpp
// SimpleApp implements application-level callbacks for the browser process.
  // It will create the first browser instance in OnContextInitialized() after
  // CEF has initialized.
  CefRefPtr<SimpleApp> app(new SimpleApp);
```

注释翻译如下

> SimpleApp实现了对于浏览器进程在应用级别的回调。该实例CEF初始化后（initialized），在OnContextInitialized中会创建第一个browser实例

查看 SimpleApp 的声明，发现该类继承了 CefApp：

```text
class SimpleApp : public CefApp, public CefBrowserProcessHandler {
 public:
  SimpleApp();
  ......
}
```

于是，我们迎来了第一个重要的类：CefApp。

## CefApp

​		CefApp 在官方文档中，就写了一句话介绍：

> The [CefApp](https://link.zhihu.com/?target=http%3A//magpcss.org/ceforum/apidocs3/projects/(default)/CefApp.html) interface provides access to process-specific callbacks.
> CefApp 接口提供了指定进程的回调访问。

​		一开始看到 CefApp 时，想到上面提到的CEF的多进程架构，结合后文还会提到的 CefClient，以为所谓CefApp 就是指浏览器进程，CefClient 就对应其他的进程（一个 App 对应多个 Client，多么的自然的理解），**然而这样错误的理解**，在阅读代码的时候走了很大的弯路。

​		首先，我们看一下 CefApp 的头文件声明：

```cpp
class CefApp : public virtual CefBaseRefCounted {
 public:
  virtual void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) {}

  virtual void OnRegisterCustomSchemes(
      CefRawPtr<CefSchemeRegistrar> registrar) {}

  virtual CefRefPtr<CefResourceBundleHandler> GetResourceBundleHandler() {
    return nullptr;
  }

  virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() {
    return nullptr;
  }

  virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() {
    return nullptr;
  }
};
```

​		先看其中有两个本文讨论的重点方法：`GetBrowserProcessHandler`、`GetRenderProcessHandler`。它们的文档注释如下：

```cpp
///
// Return the handler for functionality specific to the browser process. This
// method is called on multiple threads in the browser process.
// 返回浏览器进程特定功能的处理程序。在浏览器进程中的多个线程上调用此方法。
///
virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
///
// Return the handler for functionality specific to the render process. This
// method is called on the render process main thread.
// 返回渲染进程特定功能的处理程序。在渲染进程中的主线程上调用此方法。
///
virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler()
```

​		看到这些注释可能会疑问：为什么注释中一会儿说在浏览器进程中一会儿又说在渲染进程中？难道这个类的实例还会在多个进程中使用吗？对也不对。这个类的实例确实会在浏览器进程和渲染进程中使用，但是我们又知道，两个进程之间的资源是不共享的，包括类实例，所以在浏览器进程运行的过程中，会使用到 CefApp 的某个实例化对象，而在渲染进程的运行过程中，又会使用到 CefApp 另一个实例化对象，它们都是 CefApp 子类的实例，但一定不是同一个实例对象。

​		我们可以这样理解：一个 CefApp 对应了一个进程，而一个进程可以是浏览器进程（Browser Process），可以是渲染进程（Renderer Process）。因此，CefApp 提供了 GetBrowserProcessHandler 和GetRendererProcessHandler 来分别在相关进程中获取对应的handler。

​		这两个方法的实现由我们来决定，即我们可以通过编程方式来返回 handler，**但这两个方法不会由我们客户端代码进行调用**，而是 CEF 在运行过程中，由 CEF 在某个时刻来回调这两个方法。所以，这里虽然写了两个 GetXXXProcessHandler，但在**浏览器进程**和**渲染进程**中只会**分别**调用GetBrowserProcessHandler 和 GetRendererProcessHandler。

​		按照程序运行的角度讲，当浏览器进程运行的时候，CEF 框架就会在某个时候调用CefApp::GetBrowserProcessHandler 获得由我们定义的 BrowserProcessHandler 实例，这个实例会在适当的时候调用它提供的一些方法（后文介绍有哪些方法）；当渲染进程运行的时候，CEF 框架就会在某个时候调用 CefApp::GetRendererProcessHandler 得到我们定义的RendererProcessHandle r实例，然后在适当的时候调用 RenererProcessHandler 中的一些方法（后文介绍有哪些方法）。

​		<u>在 cefsimple 的示例代码中只有一个 SimpleApp 是继承的 CefApp，这个类还继承了CefBrowserHandler，表明自身是同时也是CefBrowserHandler，这样实现的`GetBrowserProcessHandler`就返回自身</u>。那么CEF是如何将我们的 CefApp 实例关联到CEF运行中的呢？

```cpp
// SimpleApp implements application-level callbacks for the browser process.
  // It will create the first browser instance in OnContextInitialized() after
  // CEF has initialized.
  CefRefPtr<SimpleApp> app(new SimpleApp);

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get(), sandbox_info);
```

​		注意 CefInitialize 中的 `app.get()` 参数，就是将我们的 CefApp 关联到 CEF 的运行中的。那么，有些读者会有疑问，在示例代码中，只看到我们创建的 SimpleApp 类继承了 CefApp，并通过`GetBrowserProcessHandler` 返回自身来表明是一个浏览器进程的回调实例，并没有看到体现渲染进程的代码呢？确实，cefsimple 作为 helloworld 级别的代码，没有体现这一点。在 cefclient 示例代码中（更高阶的CEF示例，也更复杂），你会看到：

![img](https://pic3.zhimg.com/80/v2-07e6cc7a76c713db7369d68e5095b23e_720w.webp)

​		上图是浏览器进程 CefApp 子类 ClientAppBrowser（这里的 ” Client “ 是 cefclient 示例代码的 “ client ”，请勿和下文的 CefClient 类混淆）。

​		同时你还能找到一个 CefApp 子类 ClientAppRenderer：

![img](https://pic2.zhimg.com/80/v2-5cdd7c57dfc7301850c60a3f7d0cbd99_720w.webp)

​		你甚至还能找到一个名为 ClientAppOther 的 CefApp 子类：

![img](https://pic3.zhimg.com/80/v2-6cd5c31c964170761e59e8c619810ede_720w.webp)

​		那么它们在哪儿被使用到呢？

![img](https://pic2.zhimg.com/80/v2-5c8e9634e5fd065b958739ef1be8dd51_720w.webp)

​		看到这里，相信绝大多数的读者应该能够理解所说的 CefApp 代表的是一个进程的抽象了。这块的大体流程是，通过一个工具函数 `GetProcessType` 从命令行中解析 `--type=xxx`（浏览器进程没有这个命令参数）来判断进程的类型，然后实例化对应的 CefApp 子类，最后通过`CefExecuteProcess`来运行进程。



​		在介绍了 CefApp 的基本概念以后，我们可以继续分析 `SimpleApp`。

​		通过上文，我们知道 SimpleApp 是 CefApp 子类，并且通过只会在浏览器进程中，会使用到该类的实例，因为实现了接口 `CefBrowserProcessHandler`，并且有如下代码：

```cpp
// CefApp methods:
  virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
      OVERRIDE {
    return this;
  }
```

​		那么在CEF中，作为 `CefBrowserProcessHandler`，有哪些回调可以供我们定制呢？下面是头文件声明，并且我也写了下概要注释：

```cpp
class CefBrowserProcessHandler : public virtual CefBaseRefCounted {
 public:
    // Cookie处理定制化
  virtual void GetCookieableSchemes(std::vector<CefString>& schemes,
                                    bool& include_defaults) {}
    // 在 CEF 上下文初始化后，在浏览器进程 UI 线程中进行调用。
  virtual void OnContextInitialized() {}
    // 可定制化处理子进程启动时的命令行参数
  virtual void OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line) {}
    // 打印处理
  virtual CefRefPtr<CefPrintHandler> GetPrintHandler() { return nullptr; }
    // 自定义消息循环的时候，消息循环的频率
  virtual void OnScheduleMessagePumpWork(int64 delay_ms) {}
    // 获取默认的 CefClient
  virtual CefRefPtr<CefClient> GetDefaultClient() { return nullptr; }
};
```

​		通过阅读该Handler的头文件以及每个函数的调用说明，我们继续阅读在`SimpleApp::OnContextInitialized` 这个函数：

```cpp
void SimpleApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

  const bool enable_chrome_runtime =
      command_line->HasSwitch("enable-chrome-runtime"); // 是否启用chrome运行时

#if defined(OS_WIN) || defined(OS_LINUX)
    // Create the browser using the Views framework if "--use-views" is specified
    // via the command-line. Otherwise, create the browser using the native
    // platform framework. The Views framework is currently only supported on
    // Windows and Linux.
    // 如果命令行中指定了"--use-views"，那么使用CEF自己的视图框架（Views framework）
    // 否则使用操作系统原生API。视图框架目前只支持Windows和Linux。
  const bool use_views = command_line->HasSwitch("use-views");
#else
  const bool use_views = false;
#endif

  // SimpleHandler implements browser-level callbacks.
  CefRefPtr<SimpleHandler> handler(new SimpleHandler(use_views));

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;

  std::string url;

  // Check if a "--url=" value was provided via the command-line. If so, use
  // that instead of the default URL.
  url = command_line->GetSwitchValue("url");
  if (url.empty())
    url = "http://www.google.com";

  if (use_views && !enable_chrome_runtime) {
    // Create the BrowserView.
    CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
        handler, url, browser_settings, nullptr, nullptr,
        new SimpleBrowserViewDelegate());

    // Create the Window. It will show itself after creation.
    CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
  } else {
    // Information used when creating the native window.
    CefWindowInfo window_info;

#if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    window_info.SetAsPopup(NULL, "cefsimple");
#endif

    // Create the first browser window.
    CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings,
                                  nullptr, nullptr);
  }
}
```

对于这段代码，整理了如下流程，方便对照阅读：

![img](https://pic2.zhimg.com/80/v2-2815c0a8149096f0c031d33c77957ec5_720w.webp)

在这个流程中，最关键的3个部分被红色标记出来：

1. SimpleHandler（CefClient子类）；
2. 使用 CEF 的窗体视图框架创建 CefBrowserView 和 CefWindow；
3. 使用操作系统原生API构建窗体。

整个过程中会创建 CefClient 的子类实例，然后通过 CEF 提供的 API 来将 CefClient 和窗体结合在一起。



**对于使用 CEF 自己的视图框架，有如下的步骤：**

1. 首先是调用 CefBrowserView::CreateBrowserView 得到 CefBrowserView 实例，这个过程会把 CefClient 实例和 View 对象通过 API 绑定。
2. 调用 CefWindow::CreateTopLevelWindow，传入 CefBrowserView 实例来创建窗体。

![img](https://pic2.zhimg.com/80/v2-1369fa7e900dcd2f5933f8a5a5bf3f29_720w.webp)

**对于使用操作系统原生API创建浏览器窗体，主要是如下步骤：**

1. 使用CefWindowInfo设置窗体句柄
2. 调用CefBrowserHost::CreateBrowser将对应窗体句柄的窗体和CefClient绑定起来

![img](https://pic2.zhimg.com/80/v2-496fc64d16f6bd05d20583004581b951_720w.webp)

​		当然，上述两个窗体的创建过程涉及到 CEF 的窗体模块，我们不在这里细说，但是两个流程都离不开一个重要的类：CefClient，它具体是什么呢？接下来，我们将对 CefClient 进行介绍，并对 SimpleHandler 这个类（CefClient 子类）进行一定的源码分析。



## CefClient

​		在官方的文档，描述了 CefClien 的概念：

> The [CefClient](https://link.zhihu.com/?target=http%3A//magpcss.org/ceforum/apidocs3/projects/(default)/CefClient.html) interface provides access to browser-instance-specific callbacks. A single CefClient instance can be shared among any number of browsers. Important callbacks include:
> CefClient 接口提供对特定于浏览器实例的回调的访问。一个 CefClient 实例可以在任意数量的浏览器之间共享。重要的回调包括：

- Handlers for things like browser life span, context menus, dialogs, display notifications, drag events, focus events, keyboard events and more. The majority of handlers are optional. See the documentation in cef_client.h for the side effects, if any, of not implementing a specific handler.
- 所有的 Handler，例如浏览器的生命周期，上下文菜单，对话框，显示通知，拖动事件，焦点事件，键盘事件等。大多数处理程序是可选的。请参阅cef_client.h 中的文档，以了解不实施特定处理程序的副作用（如果有）。
- **OnProcessMessageReceived** which is called when an IPC message is received from the render process. See the “Inter-Process Communication” section for more information.
- 从渲染过程中接收到 IPC 消息时调用的 **OnProcessMessageReceived**。有关更多信息，请参见“进程间通信”部分。

​        首先需要解释一下什么什么是特定浏览器实例，实际上，指的是以下过程产生的浏览器实例：

```cpp
CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
        handler, url, browser_settings, nullptr, nullptr,
        new SimpleBrowserViewDelegate());
// 或
    CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings,
                                  nullptr, nullptr);
```

​		通过上述两种方式创建的浏览器实例，是一个概念上的实例，并不是指你能看得到的浏览器的窗口，窗口只是浏览器实例的宿主而已。而浏览器中发生的事件，例如：生命周期的变化，对话框等，都只会通过 CefClient 中返回的各种类型 Handler 以及这些 Handler 接口实例提供的方法回调。

​		下面时 CefClient 的声明：

```cpp
class CefClient : public virtual CefBaseRefCounted {
 public:
  virtual CefRefPtr<CefAudioHandler> GetAudioHandler() { return nullptr; }

  virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() {
    return nullptr;
  }

  virtual CefRefPtr<CefDialogHandler> GetDialogHandler() { return nullptr; }

  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() { return nullptr; }

  virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() { return nullptr; }

  virtual CefRefPtr<CefDragHandler> GetDragHandler() { return nullptr; }
    // ...... 还有很多的Handler

}
```

​		在这个 CefClient 提供了很多`GetXXXHandler`方法，这些方法会在合适的时候，被 CEF 调用以得到对应的 Handler，然后再调用返回的 Handler 中的方法。例如，HTML 页面中的 Title 发生变化的时候，就会调用 `CefClient::CefDisplayHandler()` 得到一个 CefDisplayHandler 实例，然后再调用其中的`CefDisplayHandler::OnTitleChange`，而这些过程不是我们调用的，而是 CEF 框架完成的。只是具体的实现有我们客户端代码编写。

​		那么现在思考一下，为什么会有这个 CefClient 呢？可能是如下的理由：

​		<u>在 CefClient 中各种回调的事件，本质上发生的地方是渲染进程。因为每当一个浏览器实例（不是浏览器进程）创建的时候，会有一个对应的渲染进程创建（也可能由于配置，而共用一个，这里先认为默认多个一对一）。渲染进程中发生的各种 V8 事件、下载事件，显示事件等触发后，会通过**进程间通讯**给到浏览器进程，然后在浏览器进程中找到与之相关的 CefClient，然后从 CefClient 中找到对应的Handler，回调Handler对应的方法。</u>

​		<u>也就是说，将在渲染进程发生的事件，用在浏览器进程中的 CefClient 一定的抽象映射，而不是直接在浏览器进程处理器中进行，因为一个浏览器进程可能会创建多个渲染进程，让 CefClient 作为中间层避免耦合。</u>

![img](https://pic2.zhimg.com/80/v2-7babcc54058253aeb3563661100746c9_720w.webp)

​		当然，文档也为我们指出，CefClient 实例与浏览器实例可以不是一一对应的，多个浏览器实例可以共享一个 CefClient，如此一来我们也可以总结关于CefClient的一点：**非必要情况，不要编写具有状态的 CefClient**。

至此，我们通过对Demo源码入手，对 CefApp 和 CefClient 已经有了一个整体的认识，读者可以阅读官方文档来更加深入的了解：[官方文档](https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage.md#markdown-header-application-structure)。
