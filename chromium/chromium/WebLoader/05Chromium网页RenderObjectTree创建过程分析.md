[TOC]



# Chromium 网页 Render Object Tree 创建过程分析

​		在前面一文中，我们分析了网页 DOM Tree 的创建过程。网页 DOM Tree 创建完成之后，WebKit 会根据它的内容创建一个 Render Object Tree。Render Object Tree 是和网页渲染有关的一个 Tree。这意味着只有在 DOM Tree 中需要渲染的节点才会在 Render Object Tree 中有对应节点。本文接下来就分析网页 Render Object Tree 的创建过程。

​		从前面Chromium DOM Tree创建过程分析一文可以知道，**<u>每一个 HTML 标签在 DOM Tree 中都有一个对应的 HTMLElement 节点。相应地，在 DOM Tree 中每一个需要渲染的 HTMLElement 节点在R ender Object Tree 中都有一个对应的 RenderObject 节点</u>**，如图1所示：

![img](markdownimage/20160131192843053)

​		从图1还可以看到，Render Object Tree 创建完成之后，WebKit 还会继续根据它的内容创建一个 **Render Layer Tree** 和一个 **Graphics Layer Tree**。本文主要关注 Render Object Tree 的创建过程。

​		从前面 Chromium DOM Tree 创建过程分析一文还可以知道，**DOM Tree 是在网页内容的下载过程中创建的**。一旦网页内容下载完成，DOM Tree 就创建完成了。网**页的 Render Object Tree 与 DOM Tree 不一样，它是在网页内容下载完成之后才开始创建的**。因此，接下来我们就从网页内容下载完成时开始分析网页的 Render Object Tree 的创建过程。

​		从前面 Chromium 网页 URL 加载过程分析一文可以知道，**<u>WebKit 是通过 Browser 进程下载网页内容的。Browser 进程一方面通过 Net 模块中的 URLRequest 类去 We b服务器请求网页内容，另一方面又通过Content 模块中的 ResourceLoader 类的成员函数 OnReadCompleted 不断地获得 URLRequest 类请求回来的网页内容</u>**，如下所示：

```c++
void ResourceLoader::OnReadCompleted(net::URLRequest* unused, int bytes_read) {  
  ......  
  
  CompleteRead(bytes_read);  
  
  ......  
  
  if (bytes_read > 0) {  
    StartReading(true);  // Read the next chunk.  
  } else {  
    // URLRequest reported an EOF. Call ResponseCompleted.  
    DCHECK_EQ(0, bytes_read);  
    ResponseCompleted();  
  }  
}
```

​		参数 bytes_read 表示当前这次从 URLRequest 类中读取回来的网页内容的长度。当这个长度值等于 0 的时候，就表示所有的网页内容已经读取完毕。这时候 ResourceLoader 类的成员函数 OnReadCompleted 就会调用另外一个成员函数 ResponseCompleted 进行下一步处理。

​		ResourceLoader 类的成员函数 ResponseCompleted 的实现如下所示：

```c++
void ResourceLoader::ResponseCompleted() {
  ......
 
  handler_->OnResponseCompleted(request_->status(), security_info, &defer);
  
  ......
}
```

​		在前面 Chromium 网页 URL 加载过程分析一文中，我们假设 ResourceLoader 类的成员变量 handler_指向的是一个 AsyncResourceHandler 对象。ResourceLoader 类的成员函数 ResponseCompleted 调用这个 AsyncResourceHandler 对象的成员函数 OnResponseCompleted 进行下一步处理。

​		AsyncResourceHandler 类的成员函数 OnResponseCompleted 的实现如下所示：

```c++
void AsyncResourceHandler::OnResponseCompleted(
    const net::URLRequestStatus& status,
    const std::string& security_info,
    bool* defer) {
  const ResourceRequestInfoImpl* info = GetRequestInfo();
  ......
 
  ResourceMsg_RequestCompleteData request_complete_data;
  request_complete_data.error_code = error_code;
  request_complete_data.was_ignored_by_handler = was_ignored_by_handler;
  request_complete_data.exists_in_cache = request()->response_info().was_cached;
  request_complete_data.security_info = security_info;
  request_complete_data.completion_time = TimeTicks::Now();
  request_complete_data.encoded_data_length =
      request()->GetTotalReceivedBytes();
  info->filter()->Send(
      new ResourceMsg_RequestComplete(GetRequestID(), request_complete_data));
}
```

​		AsyncResourceHandler 类的成员函数 OnResponseCompleted 所做的事情是向 Render 进程发送一个类型为 ResourceMsg_RequestComplete 的 IPC 消息，用来通知 Render 进程它所请求的网页内容已下载完毕。

​		Render 进程是通过 ResourceDispatcher 类的成员函数 DispatchMessage 接收类型为ResourceMsg_RequestComplete 的 IPC 消息的，如下所示：

```c++
void ResourceDispatcher::DispatchMessage(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(ResourceDispatcher, message)
    ......
    IPC_MESSAGE_HANDLER(ResourceMsg_RequestComplete, OnRequestComplete)
  IPC_END_MESSAGE_MAP()
}
```

​		从这里可以看到，ResourceDispatcher 类的成员函数 DispatchMessage 将类型为ResourceMsg_RequestComplete 的 IPC 消息分发给另外一个成员函数 OnRequestComplete 处理。

​		ResourceDispatcher 类的成员函数 OnRequestComplete 的实现如下所示：

```c++
void ResourceDispatcher::OnRequestComplete(
    int request_id,
    const ResourceMsg_RequestCompleteData& request_complete_data) {
  ......
 
  PendingRequestInfo* request_info = GetPendingRequestInfo(request_id);
  ......
 
  RequestPeer* peer = request_info->peer;
  ......
 
  peer->OnCompletedRequest(request_complete_data.error_code,
                           request_complete_data.was_ignored_by_handler,
                           request_complete_data.exists_in_cache,
                           request_complete_data.security_info,
                           renderer_completion_time,
                           request_complete_data.encoded_data_length);
}
```

​		从前面 Chromium 网页 URL 加载过程分析一文可以知道，Render 进程在请求 Browser 进程下载指定URL 对应的网页内容之前，会创建一个 PendingRequestInfo 对象。

​		这个 PendingRequestInfo 对象以一个Request ID为键值保存在 ResourceDispatcher 类的内部。这个Request ID 即为参数 request_id 描述的 Request ID。因此，ResourceDispatcher 类的成员函数OnRequestComplete 可以通过参数 request_id 获得一个 PendingRequestInfo 对象。有了这个PendingRequestInfo 对象之后，ResourceDispatcher 类的成员函数 OnSetDataBuffer 再通过它的成员变量 peer 获得一个 WebURLLoaderImpl::Context 对象，并且调用它的成员函数 OnCompletedRequest 通知它下载网页内容的请求已完成。

​		WebURLLoaderImpl::Context 类的成员函数 OnCompletedRequest 的实现如下所示：

```c++
void WebURLLoaderImpl::Context::OnCompletedRequest(
    int error_code,
    bool was_ignored_by_handler,
    bool stale_copy_in_cache,
    const std::string& security_info,
    const base::TimeTicks& completion_time,
    int64 total_transfer_size) {
  ......
 
  if (client_) {
    if (error_code != net::OK) {
      client_->didFail(loader_, CreateError(request_.url(),
                                            stale_copy_in_cache,
                                            error_code));
    } else {
      client_->didFinishLoading(
          loader_, (completion_time - TimeTicks()).InSecondsF(),
          total_transfer_size);
    }
  }
 
  ......
}
```

​		从前面 Chromium 网页 URL 加载过程分析一文可以知道，WebURLLoaderImpl::Context 类的成员变量 client_ 指向的是 WebKit 模块中的一个 ResourceLoader 对象。在成功下载完成网页内容的情况下，WebURLLoaderImpl::Context 类的成员函数 OnCompletedRequest 调用这个 ResourceLoader 对象的成员函数 didFinishLoading 通知 WebKit 结束解析网页内容。

​		ResourceLoader 类的成员函数 didFinishLoading 的实现如下所示：

```c++
void ResourceLoader::didFinishLoading(blink::WebURLLoader*, double finishTime, int64 encodedDataLength)
{
    ......
 
    m_resource->finish(finishTime);
 
    ......
}
```

​		ResourceLoader 类的成员变量 m_resource 描述的是一个 RawResource 对象。这个 RawResource 对象的创建过程可以参考前面 Chromium 网页 URL 加载过程分析一文。ResourceLoader 类的成员函数didFinishLoading 调用这个 RawResource 对象的成员函数 finish 结束加载网页内容。

​		RawResource 类的成员函数 finish 是从父类 Resource 继承下来的，它的实现如下所示：

```c++
void Resource::finish(double finishTime)
{
    ......
    finishOnePart();
    ......
}
```

​		Resource 类的成员函数 finish 调用另外一个成员函数 finishOnePart 结束加载网页的内容。注意Resource 类的成员函数 finishOnePart 的命名。有前面 Chromium 网页 URL 加载过程分析一文中，我们提到，当网页内容的 MIME 类型为 “ multipart/x-mixed-replace ” 时，下载回来网页内容实际是包含多个部分的，每一个部分都有着自己的 MIME 类型。每一个部分下载完成时，都会调用 Resourc e类的成员函数finishOnePart 进行处理。为了统一接口，对于 MIME 类型不是 “ multipart/x-mixed-replace ” 的网页内容而言，下载回来的网页内容也是当作一个部分进行整体处理。

​		Resource 类的成员函数 finishOnePart 的实现如下所示：

```c++
void Resource::finishOnePart()
{
    ......
    checkNotify();
}
```

​		Resource 类的成员函数 finishOnePart 调用另外一个成员函数 checkNotify 通知当前正在前处理的Resource 对象的 Client，它们所关注的资源，也就是网页内容，已经下载完成了。

​		Resource 类的成员函数 checkNotify 的实现如下所示：

```c++
void Resource::checkNotify()
{
    ......
 
    ResourceClientWalker<ResourceClient> w(m_clients);
    while (ResourceClient* c = w.next())
        c->notifyFinished(this);  
}
```

​		从前面 Chromium 网页 URL 加载过程分析一文可以知道，在 Resource 类的成员变量 m_clients 中，保存有一个 DocumentLoader 对象。这个 DocumentLoader 对象是从 ResourceClient 类继承下来的，它负责创建和加载网页的文档对象。Resource 类的成员函数 checkNotify 会调用这个 DocumentLoader 对象的成员函数 notifyFinished 通知它要加载的网页的内容已经下载完成了。

​		DocumentLoader 类的成员函数 notifyFinished 的实现如下所示：

```c++
void DocumentLoader::notifyFinished(Resource* resource)
{
    ......
 
    if (!m_mainResource->errorOccurred() && !m_mainResource->wasCanceled()) {
        finishedLoading(m_mainResource->loadFinishTime());
        return;
    }
 
    ......
}
```

​		DocumentLoader 类的成员变量 m_mainResource 指向的是一个 RawResource 对象。这个RawResource 对象和前面分析的 ResourceLoader 类的成员变量 m_resource 指向的是同一个RawResource 对象。这个 RawResource 对象代表正在请求下载的网页内容。在网页内容成功下载完成的情况下，DocumentLoader 类的成员函数 notifyFinished 就会调用另外一个成员函数 finishedLoading 进行结束处理。

​		DocumentLoader 类的成员函数 finishedLoading 的实现如下所示：

```c++
void DocumentLoader::finishedLoading(double finishTime)
{
    ......
 
    endWriting(m_writer.get());
 
    ......
}
```

​		从前面Chromium网页DOM Tree创建过程分析一文可以知道，DocumentLoader 类的成员变量m_writer 指向的是一个 DocumentWriter 对象。DocumentLoader 类的成员函数 finishedLoading 调用另外一个成员函数 endWriting 告诉这个 DocumentWriter 对象结束对正在加载的网页内容的解析。

​		DocumentLoader 类的成员函数 endWriting 的实现如下所示：

```c++
void DocumentLoader::endWriting(DocumentWriter* writer)
{
    ......
    m_writer->end();
    .....
}
```

​		DocumentLoader 类的成员函数 endWriting 调用上述 DocumentWriter 对象的成员函数 end 结束对正在加载的网页内容的解析。

​		DocumentWriter 类的成员函数 end 的实现如下所示：

```c++
void DocumentWriter::end()
{
    ......
 
    m_parser->finish();
    
    ......
}
```

​		从前面Chromium网页DOM Tree创建过程分析一文可以知道，DocumentWriter 类的成员变量m_parser 指向的是一个 HTMLDocumentParser 对象。DocumentWriter 类的成员函数 end 调用这个HTMLDocumentParser 对象的成员函数 finish 结束对正在加载的网页内容的解析。

​		HTMLDocumentParser 类的成员函数 finish 的实现如下所示：

```c++
void HTMLDocumentParser::finish()
{
    ......
 
    attemptToEnd();
}
```

​		HTMLDocumentParser 类的成员函数 finish 调用另外一个成员函数 attemptToEnd 结束对正在加载的网页内容的解析。

​		HTMLDocumentParser 类的成员函数 attemptToEnd 的实现如下所示：

```c++
void HTMLDocumentParser::attemptToEnd()
{
    // finish() indicates we will not receive any more data. If we are waiting on
    // an external script to load, we can't finish parsing quite yet.
 
    if (shouldDelayEnd()) {
        m_endWasDelayed = true;
        return;
    }
    prepareToStopParsing();
}
```

​		如果网页包含外部 JavaScript 脚本，并且这些外部 JavaScript 脚本还没有下载回来，那么这时候HTMLDocumentParser 类的成员函数 attemptToEnd 就还不能结束对正在加载的网页内容的解析，必须要等到外部 JavaScript 脚本下载回来之后才能进行结束。

​		另一方面，如果网页没有包含外部 JavaScript 脚本，那 么HTMLDocumentParser 类的成员函数attemptToEnd 就会马上调用另外一个成员函数 prepareToStopParsing 结束对正在加载的网页内容的解析。在网页包含外部 JavaScript 脚本的情况下，等到这些外部 JavaScript 脚本下载回来处理之后，HTMLDocumentParser 类的成员函数 prepareToStopParsing 也是同样会被调用的。因此，接下来我们就继续分析 HTMLDocumentParser 类的成员函数 prepareToStopParsing 的实现。

​		HTMLDocumentParser 类的成员函数 prepareToStopParsing 的实现如下所示：

```c++
void HTMLDocumentParser::prepareToStopParsing()
{
    ......
 
    attemptToRunDeferredScriptsAndEnd();
}
```

​		HTMLDocumentParser 类的成员函数 prepareToStopParsing 调用另外一个成员函数attemptToRunDeferredScriptsAndEnd **执行那些被延后执行的 JavaScript 脚本，以及结束对正在加载的网页内容的解析。**

​		HTMLDocumentParser 类的成员函数 attemptToRunDeferredScriptsAndEnd 的实现如下所示：

```c++
void HTMLDocumentParser::attemptToRunDeferredScriptsAndEnd()
{
    ......
 
    if (m_scriptRunner && !m_scriptRunner->executeScriptsWaitingForParsing())
        return;
    end();
}
```

​		HTMLDocumentParser 类的成员变量 m_scriptRunner 指向的是一个 HTMLScriptRunner 对象。HTMLDocumentParser 类的成员函数 attemptToRunDeferredScriptsAndEnd 调用这个HTMLScriptRunner 对象的成员函数 executeScriptsWaitingForParsing 执行那些被延后执行的 JavaScript 脚本之后，就会调用 HTMLDocumentParser 类的成员函数 end 结束对正在加载的网页内容的解析。

​		HTMLDocumentParser 类的成员函数 end 的实现如下所示：

```c++
void HTMLDocumentParser::end()
{
    ......
 
    // Informs the the rest of WebCore that parsing is really finished (and deletes this).
    m_treeBuilder->finished();
}
```

​		HTMLDocumentParser类的成员变量m_treeBuilder指向的是一个HTMLTreeBuilder对象。HTMLDocumentParser类的成员函数end调用这个HTMLTreeBuilder对象的成员函数finished告诉它结束对网页内容的解析。

​		HTMLTreeBuilder类的成员函数finished的实现如下所示：

```c++
void HTMLTreeBuilder::finished()
{
    ......
 
    m_tree.finishedParsing();
}
```

​		HTMLTreeBuilder 类的成员变量 m_tree 描述的是一个 HTMLConstructionSite 对象。从前面Chromium网页DOM Tree创建过程分析一文可以知道，这个 HTMLConstructionSite 对象就是用来构造网页的 DOM Tree 的，HTMLTreeBuilder 类的成员函数 finished 调用它的成员函数 finishedParsing告诉它结束构造网页的DOM Tree。

​		HTMLConstructionSite 类的成员函数 finishedParsing 的实现如下所示：

```c++
void HTMLConstructionSite::finishedParsing()
{
    ......
 
    m_document->finishedParsing();
}
```

​		HTMLConstructionSite 类的成员变量 m_document 指向的是一个 HTMLDocument 对象。这个HTMLDocument 对象描述的是网页的 DOM Tree 的根节点，HTMLConstructionSite 类的成员函数finishedParsing 调用它的成员函数 finishedParsing 通知它 DOM Tree 创建结束。

​		HTMLDocument 类的成员函数 finishedParsing 是从父类 Document 继承下来的，它的实现如下所示：

```c++
void Document::finishedParsing()
{
    ......
 
    if (RefPtr<LocalFrame> f = frame()) {
        ......
        const bool mainResourceWasAlreadyRequested =
            m_frame->loader().stateMachine()->committedFirstRealDocumentLoad();
 
        ......
        if (mainResourceWasAlreadyRequested)
            updateRenderTreeIfNeeded();
 
        ......
    }
 
    ......
}
```

​		**HTMLDocument 类的成员函数 finishedParsing 首先判断网页的主资源是否已经请求回来了。在请求回来的情况下，才会调用另外一个成员函数 updateRenderTreeIfNeeded 创建一个 Render Object Tree。网页的主资源，指的就是网页文本类型的内容，不包括 Image、CSS和Script 等资源。**

​		HTMLDocument 类的成员函数 updateRenderTreeIfNeeded 也是从父类 Document 继承下来的，它的实现如下所示：

```c++
class Document : public ContainerNode, public TreeScope, public SecurityContext, public ExecutionContext, public ExecutionContextClient
    , public DocumentSupplementable, public LifecycleContext<Document> {
    ......
public:
    ......
 
    void updateRenderTreeIfNeeded() { updateRenderTree(NoChange); }
 
    ......
};
```

​		HTMLDocument类的成员函数updateRenderTreeIfNeeded调用另外一个成员函数updateRenderTree创建一个Render Object Tree。

​		HTMLDocument类的成员函数updateRenderTree是从父类Document继承下来的，它的实现如下所示：

```c++
void Document::updateRenderTree(StyleRecalcChange change)
{
    ......
 
    updateStyle(change);
 
    ......
}
```

​		**<u>Document 类的成员函数 updateRenderTree 会调用另外一个成员函数 updateStyle 更新网页各个元素的 CSS 属性。Document 类的成员函数 updateStyle 在更新网页各个元素的 CSS 属性的过程中，会分别为它们创建一个对应的R ender Object。这些 Render Object 最终就会形成一个 Render Object Tree。</u>**

​		Document 类的成员函数 updateStyle 的实现如下所示：

```c++
void Document::updateStyle(StyleRecalcChange change)
{
    ......
 
    if (styleChangeType() >= SubtreeStyleChange)
        change = Force;
 
    ......
 
    if (change == Force) {
        ......
        RefPtr<RenderStyle> documentStyle = StyleResolver::styleForDocument(*this);
        StyleRecalcChange localChange = RenderStyle::stylePropagationDiff(documentStyle.get(), renderView()->style());
        if (localChange != NoChange)
            renderView()->setStyle(documentStyle.release());
    }
 
    ......
 
    if (Element* documentElement = this->documentElement()) {
        ......
        if (documentElement->shouldCallRecalcStyle(change))
            documentElement->recalcStyle(change);
        ......
    }
 
    ......
}
```

​		从前面Chromium网页DOM Tree创建过程分析一文可以知道，**<u>当前正在处理的 Document 对象实际上是一个 HTMLDocument 对象。这个 HTMLDocument 对象即为网页 DOM Tree 的根节点，它的子孙节点就是网页中的各个 HTML 标签。在 DOM Tree 创建之初，这些 HTML 标签的 CSS 属性还没有进行计算，因此这时候 DOM Tree 的根节点就会被标记为子树 CSS 属性需要进行计算，也就是调用当前正在处理的 Document对象的成员函数 styleChangeType 获得的值会等于 SubtreeStyleChange。在这种情况下，参数 change 的值也会被修改为 Force，表示要对每一个 HTML 标签的 CSS 属性进行一次计算和设置。</u>**

​		接下来，Document 类的成员函数 updateStyle 首先是计算根节点的 CSS 属性，这是通过调用StyleResolver 类的静态成员函数 styleForDocument 实现的，接着又比较根节点新的 CSS 属性与旧的 CSS 属性是否有不同的地方。**如果有不同的地方，那么就会将新的 CSS 属性值保存在与根节点对应的 Render Object中。**

​		从前面Chromium网页DOM Tree创建过程分析一文可以知道，DOM Tree 的根节点，也就是一个HTMLDocument 对象，是在解析网页内容之前就已经创建好了的，并且在创建这个 HTMLDocument 对象的时候，会给它关联一个 Render Object。这个 Render Object 实际上是一个 RenderView 对象。这个RenderView 对象就作为网页 Render Object Tree 的根节点。

​		Document 类的成员函数 updateStyle 调用另外一个成员函数 renderView() 可以获得上面描述的RenderView 对象。有了这个 RenderView 对象之后，调用它的成员函数 style 就可以获得它原来设置的 CSS属性，同时调用它的成员函数 setStyle 可以给它设置新的 CSS。

​		更新好 DOM Tree 的根节点的 CSS 属性之后，Document 类的成员函数 updateStyle 接下来继续更新它的子节点的CSS 属性，也就是网页的 \<html\> 标签的 CSS 属性。从前面Chromium网页DOM Tree创建过程分析一文可以知道，网页的\<html\>标签在 DOM Tree 中通过一个 HTMLHtmlElement 对象描述。这个HTMLHtmlElement 对象可以通过调用当前正在处理的 Document 对象的成员函数 documentElement 获得。有了这个 HTMLHtmlElement 对象之后，就可以调用它的成员函数 recalcStyle 更新它以及它的子节点的 CSS 属性了。

​		HTMLHtmlElement 类的成员函数 recalcStyle 是从父类 Element 继承下来的，它的实现如下所示：

```c++
void Element::recalcStyle(StyleRecalcChange change, Text* nextTextSibling)
{
    ......
 
    if (change >= Inherit || needsStyleRecalc()) {
        ......
        if (parentRenderStyle())
            change = recalcOwnStyle(change);
        ......
    }
 
    ......
}
```

​		从前面的调用过程可以知道，参数 change 的值等于 Force，它的值是大于 Inherit 的。在这种情况下，如果当前正在处理的 DOM 节点的父节点的 CSS 属性已经计算好，也就是调用成员函数 parentRenderStyle 的返回值不等于 NULL，那么 Element 类的成员函数 recalcStyle 就会重新计算当前正在处理的 DOM 节点的CSS 属性。这是通过调用 Element 类的成员函数 recalcOwnStyle 实现的。

​		另一方面，如果参数 change 的值小 于Inherit，但是当前正在处理的 DOM 节点记录了它的 CSS 属性确实发生了变化需要重新计算，也就是调用成员函数 needsStyleRecalc 获得的返值为 true。那么 Element 类的成员函数 recalcStyle 也会调用另外一个成员函数 recalcOwnStyle 重新计算当前正在处理的 DOM 节点的 CSS 属性。

​		Elemen t类的成员函数 recalcOwnStyle 的实现如下所示：

```c++
StyleRecalcChange Element::recalcOwnStyle(StyleRecalcChange change)
{
    ......
 
    RefPtr<RenderStyle> oldStyle = renderStyle();
    RefPtr<RenderStyle> newStyle = styleForRenderer();
    StyleRecalcChange localChange = RenderStyle::stylePropagationDiff(oldStyle.get(), newStyle.get());
 
    ......
 
    if (localChange == Reattach) {
        AttachContext reattachContext;
        reattachContext.resolvedStyle = newStyle.get();
        ......
        reattach(reattachContext);
        ......
    }
 
    ......
}
```

​		Element 类的成员函数 recalcOwnStyle 首先调用成员函数 renderStyle 获得当前正处理的 DOM 节点的原来设置的 CSS 属性。由于当前正在处理的 DOM 节点还没有计算过 CSS 属性，因此前面获得的 CSS 属性就为空。

​		**Element 类的成员函数 recalcOwnStyle 接下来又调用成员函数 styleForRenderer 计算当前正在处理的DOM 节点的 CSS 属性，这是通过解析网页内容得到的。在这种情况下调用 RenderStyle 类的静态成员函数stylePropagationDiff 比较前面获得的两个 CSS 属性，会得一个值为 Reattach 的返回值，表示要为当前正在处理的 DOM 节点创建一个Render Object，并且将这个 Render Object 加入到网页的 Render Object Tree 中去。这是通过调用 Element 类的成员函数 reattach 实现的。**

​		Element 类的成员函数 reattach 是从父类 Node 继承下来的，它的实现如下所示：

```c++
void Node::reattach(const AttachContext& context)
{
    AttachContext reattachContext(context);
    reattachContext.performingReattach = true;
 
    ......
 
    attach(reattachContext);
}
```

​		Node 类的成员函数 reattach 主要是调用另外一个成员函数 attach 为当前正在处理的 DOM 节点创建一个Render Object。从前面的分析可以知道，当前正在处理的 DOM 节点是网页的 HTML 标签，也就是一个HTMLHtmlElement 对象。HTMLHtmlElement 类是从 Element 类继承下来的，Element 类又是从 Node类继承下来的，并且它重写了 Node 类的成员函数 attach。因此，在我们这个情景中，Node 类的成员函数reattach 实际上调用的是 Element 类的成员函数 attach。

​		Element 类的成员函数 attach 的实现如下所示：

```c++
void Element::attach(const AttachContext& context)
{
    ......
 
    RenderTreeBuilder(this, context.resolvedStyle).createRendererForElementIfNeeded();
 
    ......
 
    ContainerNode::attach(context);
 
    ......
}
```

​		Element 类的成员函数 attach 首先是根据当前正在处理的 DOM 节点的 CSS 属性创建一个RenderTreeBuilder 对象，接着调用这个 RenderTreeBuilder 对象的成员函数createRendererForElementIfNeeded 判断是否需要为当前正在处理的 DOM 节点创建的一个Render Object，并且在需要的情况下进行创建。

​		Element 类的成员函数 attach 最后还会调用父类 ContainerNode 的成员函数 attach 递归为当前正在处理的 DOM 节点的所有子孙节点分别创建一个 Render Object，从而就得到一个 Render Object Tree。

​		接下来，我们首先分析 RenderTreeBuilder 类的成员函数 createRendererForElementIfNeeded 的实现，接着再分析 ContainerNode 类的成员函数 attach 的实现。

​		RenderTreeBuilder 类的成员函数 createRendererForElementIfNeeded 的实现如下所示：

```c++
void RenderTreeBuilder::createRendererForElementIfNeeded()
{
    ......
 
    Element* element = toElement(m_node);
    RenderStyle& style = this->style();
 
    if (!element->rendererIsNeeded(style))
        return;
 
    RenderObject* newRenderer = element->createRenderer(&style);
    ......
 
    RenderObject* parentRenderer = this->parentRenderer();
    ......
 
    element->setRenderer(newRenderer);
    newRenderer->setStyle(&style); // setStyle() can depend on renderer() already being set.
 
    parentRenderer->addChild(newRenderer, nextRenderer);
}
```

​		RenderTreeBuilder 类的成员变量 m_node 描述的是当前正在处理的 DOM 节点。这个 DOM 节点对象类型一定是从 Element 类继承下来的，因此 RenderTreeBuilder 类的成员函数createRendererForElementIfNeeded 可以通过调用另外一个成员函数 toElmen t将其转换为一个 Element对象。

​		接下来还会通过调用另外一个成员函数 style 获得当前正在处理的 DOM 节点的 CSS 属性对象，然后再以这个 CSS 属性对象为参数，调用上面获得的 Element 对象的成员函数 rendererIsNeeded 判断是否需要为当前正在处理的 DOM 节点创建一个R ender Object。如果不需要，那么 RenderTreeBuilder 类的成员函数createRendererForElementIfNeeded 就直接返回了。

​		Element 类的成员函数 rendererIsNeeded 的实现如下所示：

```c++
bool Element::rendererIsNeeded(const RenderStyle& style)
{
    return style.display() != NONE;
}
```

​		从这里可以看到，当一个 DOM 节点的 display 属性被设置为 none 时，WebKit 就不会为它创建一个Render Object，也就是当一个 DOM 节点不需要渲染或者不可见时，就不需要为它创建一个 Render Object。

​		回到 RenderTreeBuilder 类的成员函数 createRendererForElementIfNeeded 中，假设需要为前正在处理的 DOM 节点创建 Render Object，那么 RenderTreeBuilder 类的成员函数createRendererForElementIfNeeded 接下来就会调用上面获得的 Element 对象的成员函数createRenderer 为其创建一个 Render Object。

​		为当前正在处理的 DOM 节点创建了 Render Object 之后，RenderTreeBuilder 类的成员函数createRendererForElementIfNeeded 接下来还做了三件事情：

   1. 将新创建的 Render Object 与当前正在处理的 DOM 节点关联起来。这是通过调用 Element 类的成员函数setRenderer实现的。

   2. 将用来描述当前正在处理的 DOM 节点的 CSS 属性对象设置给新创建的 Render Object，以便新创建的Render Object 后面可以根据这个 CSS 属性对象绘制自己。这是通过调用 RenderObjec t类的成员函数setStyle 实现的。

   3. 获得与当前正在处理的 DOM 节点对应的 Render Object，并且将新创建的 Render Object 作为这个Render Object 的子节点，从而形成一个 Render Object Tree。这是通过调用 RenderObject 类的成员函数 addChild 实现的。

​        接下来，我们主要分析 Element 类的成员函数 createRendere r为一个 DOM 节点创建一个 Render Object 的过程，如下所示：

```c++
RenderObject* Element::createRenderer(RenderStyle* style)
{
    return RenderObject::createObject(this, style);
}
```

​		Element类的成员函数createRenderer是通过调用RenderObject类的静态成员函数createObject为当前正在处理的DOM节点创建一个Render Object的。

​		RenderObject类的静态成员函数createObject的实现如下所示：

```c++
RenderObject* RenderObject::createObject(Element* element, RenderStyle* style)
{
    ......
 
    switch (style->display()) {
    case NONE:
        return 0;
    case INLINE:
        return new RenderInline(element);
    case BLOCK:
    case INLINE_BLOCK:
        return new RenderBlockFlow(element);
    case LIST_ITEM:
        return new RenderListItem(element);
    case TABLE:
    case INLINE_TABLE:
        return new RenderTable(element);
    case TABLE_ROW_GROUP:
    case TABLE_HEADER_GROUP:
    case TABLE_FOOTER_GROUP:
        return new RenderTableSection(element);
    case TABLE_ROW:
        return new RenderTableRow(element);
    case TABLE_COLUMN_GROUP:
    case TABLE_COLUMN:
        return new RenderTableCol(element);
    case TABLE_CELL:
        return new RenderTableCell(element);
    case TABLE_CAPTION:
        return new RenderTableCaption(element);
    case BOX:
    case INLINE_BOX:
        return new RenderDeprecatedFlexibleBox(element);
    case FLEX:
    case INLINE_FLEX:
        return new RenderFlexibleBox(element);
    case GRID:
    case INLINE_GRID:
        return new RenderGrid(element);
    }
 
    return 0;
}
```

​		RenderObject 类的静态成员函数 createObject 主要是根据参数 element 描述的一个 DOM 节点的display 属性值创建一个具体的 Render Object。例如，如果参数 element 描述的 DOM 节点的 display 属性值为 BLOCK 或者 INLINE_BLOCK，那么 RenderObject 类的静态成员函数 createObject 为它创建的就是一个类型为 RenderBlockFlow 的 Render Object。

​		不管是哪一种类型的 Render Object，它们都是间接从 RenderBoxModelObject 类继承下来的。RenderBoxModelObject 类又是间接从 RenderObject 类继承下来的，它描述的是一个 CSS Box Model，如图2所示：

![img](markdownimage/20160203015858510)

​		关于 CSS Box Model 的详细描述，可以参考这篇文章：[CSS Box Model and Display Positioning | by Tairat Aderonke Fadare | Medium](https://medium.com/@tairataderonkefadare/css-box-model-and-display-positioning-debfd0e0a730)。简单来说，就是一个 CSS Box Model 由 margin、border、padding和 content 四部分组成。其中，margin、border 和 padding 又分为 top、bottom、left 和 right 四个值。一个 Render Object 在绘制之前，会先进行 Layout。Layout 的目的就是确定一个 Render Object 的 CSS Box Model 的 margin、border 和 padding 值。一旦这些值确定之后，再结合 content 值，就可以对一个Render Object 进行绘制了。

​		这一步执行完成后，回到 Element 类的成员函数 attach 中，**接下来它会调用父类 ContainerNode 的成员函数 attach 递归为当前正在处理的 DOM 节点的所有子孙节点分别创建一个 Render Object，从而形成一个Render Object Tree。**

​		ContainerNode类的成员函数attach的实现如下所示：

```c++
void ContainerNode::attach(const AttachContext& context)
{
    attachChildren(context);
    ......
}
```

​		ContainerNode类的成员函数attach主要是调用另外一个成员函数attachChildren递归为当前正在处理的DOM节点的所有子孙节点分别创建一个Render Object，如下所示：

```c++
inline void ContainerNode::attachChildren(const AttachContext& context)
{
    AttachContext childrenContext(context);
    childrenContext.resolvedStyle = 0;
 
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        ASSERT(child->needsAttach() || childAttachedAllowedWhenAttachingChildren(this));
        if (child->needsAttach())
            child->attach(childrenContext);
    }
}
```

​		从这里就可以看到，ContainerNode 类的成员函数 attachChildren 会依次遍历当前正在处理的 DOM 节点的每一个子节点，并且对于需要创建 Render Object 的子节点，会调用它的成员函数 attach 进行创建，也就是调用我们前面分析过的 Element 类的成员函数attach进行创建。这个过程会一直重复下去，直到遍历至DOM 树的叶子节点为止。这时候就会得到图1所示的 Render Object Tree。

​		**至此，网页的 Render Object Tree 的创建过程就分析完成了，网页的 Render Object Tree 是在 DOM Tree 构造完成时开始创建的，并且 Render Object Tree 的每一个节点与 DOM Tree 的某一个节点对应，但是又不是每一个在 DOM Tree 的节点在 Render Object Tree 中都有对应的节点，只有那些需要进行渲染的 DOM节点才会在 Render Object Tree 中有对应的节点。**

​		事实上，WebKit在为网页创建Render Object Tree的过程中，也会为网页创建图1所示的Render Layer Tree。关于网页Render Layer Tree的创建过程，我们将在接下来的一篇文章进行分析.

















