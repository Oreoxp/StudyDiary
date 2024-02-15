[TOC]



# Chromium 网页 URL 加载过程分析

​		Chromium 在 Browser 进程中为网页创建了一个 Frame Tree 之后，会将网页的 URL 发送给 Render 进程进行加载。Render 进程接收到网页 URL 加载请求之后，会做一些必要的初始化工作，然后请求 Browser 进程下载网页的内容。Browser 进程一边下载网页内容，一边又通过共享内存将网页内容传递给 Render 进程解析，也就是创建 DOM Tree。本文接下来就分析网页 URL 的加载过程。

### 重要问题：

#### 为什么 Render 进程之所以要请求 Browser 进程下载网页的内容？

​		Render 进程之所以要请求 Browser 进程下载网页的内容，是因为 Render 进程没有网络访问权限。出于安全考虑，Chromium 将 Render 进程启动在一个受限环境中，使得 Render 进程没有网络访问权限。

#### 为什么不是 Browser 进程主动下载好网页内容再交给 Render 进程解析呢？

​		这是<u>因为 Render 进程是通过 WebKit 加载网页 URL 的，WebKit 不关心自己所在的进程是否有网络访问权限，它通过特定的接口访问网络</u>。这个特定接口由 WebKit 的使用者，也就是 Render 进程中的 Content 模块实现。Content 模块在实现这个接口的时候，会通过 IPC 请求 Browser 进程下载网络的内容。这种设计方式使得 WebKit 可以灵活地使用：既可以在有网络访问权限的进程中使用，也可以在没有网络访问权限的进程中使用，并且使用方式是统一的。





​		从前面 Chromium Frame Tree 创建过程分析一文可以知道，Browser 进程中为要加载的网页创建了一个Frame Tree 之后，会向 Render 进程发送一个类型为 **FrameMsg_Navigate** 的 IPC 消息。Render 进程接收到这个 IPC 消息之后，处理流程如图 1 所示：

![img](markdownimage/20160116130748903)

​		**<u>Render 进程执行了一些初始化工作之后，就向 Browser 进程发送一个类型为ResourceHostMsg\_RequestResource 的 IPC 消息。Browser 进程收到这个 IPC 消息之后，就会通过 HTTP 协议请求 Web 服务器将网页的内容返回来。请求得到响应后，Browser 进程就会创建一块共享内存，并且通过一个类型为 ResourceMsg\_SetDataBuffer 的 IPC 消息将这块共享内存传递给 Render 进程的</u>**。

​		以后每当下载到新的网页内容，Browser 进程就会将它们写入到前面创建的共享内存中去，并且发送Render 进程发送一个类型为 ResourceMsg_DataReceived 的 IPC 消息。Render 进程接收到这个 IPC 消息之后，就会从共享内存中读出 Browser 进程写入的内容，并且进行解析，也就是创建一个 DOM Tree 。这个过程一直持续到网页内容下载完成为止。





​		接下来，我们就从 Render 进程接收类型为 FrameMsg_Navigate 的 IPC 消息开始分析网页 URL 的加载过程。Render 进程是通过 RenderFrameImpl 类的成员函数 OnMessageReceived 接收类型为FrameMsg_Navigate 的 IPC 消息的，如下所示：

```c++
bool RenderFrameImpl::OnMessageReceived(const IPC::Message& msg) {
  ......
 
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderFrameImpl, msg)
    IPC_MESSAGE_HANDLER(FrameMsg_Navigate, OnNavigate)
    ......
  IPC_END_MESSAGE_MAP()
 
  return handled;
}
```

​		RenderFrameImpl 类的成员函数 OnMessageReceived 将类型为 **FrameMsg_Navigate** 的 IPC 消息分发给另外一个成员函数 OnNavigate 处理，后者的实现如下所示：

```c++
void RenderFrameImpl::OnNavigate(const FrameMsg_Navigate_Params& params) {
  ......
 
  bool is_reload = RenderViewImpl::IsReload(params);
  ......
 
  WebFrame* frame = frame_;
  ......
 
  if (is_reload) {
    ......
  } else if (params.page_state.IsValid()) {
    ......
  } else if (!params.base_url_for_data_url.is_empty()) {
    ......
  } else {
    // Navigate to the given URL.
    WebURLRequest request(params.url);
    ......
 
    frame->loadRequest(request);
 
    ......
  }
 
  ......
}
```

​		从前面 Chromium Frame Tree 创建过程分析一文可以知道，RenderFrameImpl 类的成员变量 frame\_ 指向的是一个 WebLocalFrameImpl 对象。如果当前正在处理的 RenderFrameImpl 对象还没有加载过URL，并且当前要加载的 URL 不为空，RenderFrameImpl 类的成员函数 OnNavigate 会调用成员变量frame\_ 指向的 WebLocalFrameImpl 对象的成员函数 loadRequest 加载指定的 URL 。

​		WebLocalFrameImpl 类的成员函数 loadRequest 的实现如下所示：

```c++
void WebLocalFrameImpl::loadRequest(const WebURLRequest& request)
{
    ......
    const ResourceRequest& resourceRequest = request.toResourceRequest();
 
    if (resourceRequest.url().protocolIs("javascript")) {
        loadJavaScriptURL(resourceRequest.url());
        return;
    }
 
    frame()->loader().load(FrameLoadRequest(0, resourceRequest));
}
```

​		如果参数 request 描述的 URL 指定的协议是 " javascript " ，那么表示要加载的是一段 JavaScript。这时候 WebLocalFrameImpl 类的成员函数 loadRequest 会调用另外一个成员函数 loadJavaScriptURL 加载这段 JavaScript。 

​		在其它情况下，WebLocalFrameImpl 类的成员函数 loadRequest 首先调用成员函数 frame 获得成员变量 m_frame 描述的一个 LocalFrame 对象，接着又调用这个 LocalFrame 对象的成员函数 loader 获得其成员变量 m_loader 描述的一个 FrameLoader 对象。有了这个 FrameLoader 对象之后，就调用它的成员函数load 加载参数 request 描述的 URL。

​		WebLocalFrameImpl 类的成员变量 m_frame 描述的 LocalFrame 对象和 LocalFrame 类的成员变量m_loader 描述的 FrameLoader 对象的创建过程，可以参考前面 Chromium Frame Tree 创建过程分析一文。接下来我们继续分析 FrameLoader 类的成员函数 load 的实现，如下所示：

```c++
void FrameLoader::load(const FrameLoadRequest& passedRequest)
{
    ......
 
    FrameLoadRequest request(passedRequest);
    ......
 
    FrameLoadType newLoadType = determineFrameLoadType(request);
    NavigationAction action(request.resourceRequest(), newLoadType, request.formState(), request.triggeringEvent());
    ......
 
    loadWithNavigationAction(action, newLoadType, request.formState(), request.substituteData(), request.clientRedirect());
 
    ......
}
```

​		FrameLoader 类的成员函数 load 主要是调用另外一个成员函数 loadWithNavigationAction 加载参数passedRequest 描述的 URL。

​		FrameLoader类的成员函数loadWithNavigationAction的实现如下所示：

```c++
void FrameLoader::loadWithNavigationAction(const NavigationAction& action, FrameLoadType type, PassRefPtrWillBeRawPtr<FormState> formState, const SubstituteData& substituteData, ClientRedirectPolicy clientRedirect, const AtomicString& overrideEncoding)
{
    ......
 
    const ResourceRequest& request = action.resourceRequest();
    ......
 
    m_policyDocumentLoader = client()->createDocumentLoader(m_frame, request, substituteData.isValid() ? substituteData : defaultSubstituteDataForURL(request.url()));
    ......
 
    m_provisionalDocumentLoader = m_policyDocumentLoader.release();
    ......
 
    m_provisionalDocumentLoader->startLoadingMainResource();
}
```

​		FrameLoader 类的成员函数 loadWithNavigationAction 首先调用成员函数 client 获得一个FrameLoaderClientImpl 对象，接着再调用这个 FrameLoaderClientImpl 对象的成员函数createDocumentLoader 为参数 action 描述的 URL 创建了一个 WebDataSourceImpl 对象，并且保存在成员变量 m_policyDocumentLoader 中。关于 FrameLoader 类的成员函数 client 和FrameLoaderClientImpl 类的成员函数 createDocumentLoader 的实现，可以参考前面Chromium Frame Tree创建过程分析一文。

​		FrameLoader 类的成员函数 loadWithNavigationAction 接下来又将成员变量m_policyDocumentLoader 描述的 WebDataSourceImpl 对象转移到另外一个成员变量m_provisionalDocumentLoader 中，最后调用这个 WebDataSourceImpl 对象的成员函数startLoadingMainResource 加载参数 action 描述的 URL。

​		WebDataSourceImpl类的成员函数startLoadingMainResource是从父类DocumentLoader继承下来的，它的实现如下所示：

```c++
void DocumentLoader::startLoadingMainResource() { 
    ......
    FetchRequest cachedResourceRequest(request, FetchInitiatorTypeNames::document, mainResourceLoadOptions);
    m_mainResource = m_fetcher->fetchMainResource(cachedResourceRequest, m_substituteData);
    ......
 
    m_mainResource->addClient(this); 
 
    ......
}
```

​		从前面 Chromium Frame Tree 创建过程分析一文可以知道，DocumentLoader 类的成员变量m_fetcher 描述的是一个 ResourceFetcher 对象，DocumentLoader 类的成员函数startLoadingMainResource 调用这个 ResourceFetcher 对象的成员函数 fetchMainResource 请求加载本地变量 cachedResourceRequest 描述的资源。这个资源描述的即为上一步指定要加载的 URL。

​		ResourceFetcher 类的成员函数 fetchMainResource 执行结束后，会返回一个 RawResource 对象。这个 RawResource 对象保存在 WebDataSourceImpl 类的成员变量 m_mainResource 中。这个RawResource 对象描述的是一个异步加载的资源，DocumentLoader 类的成员 startLoadingMainResource 调用它的成员函数 addClient 将当前正在处理的 DocumentLoader 对象添加到它的内部去，用来获得异步加载的资源数据，也就是本地变量 cachedResourceRequest 描述的 URL 对应的网页内容。

​		RawResource 类的成员函数 addClient 是从父类 Resource 继承下来的，它的实现如下所示：

```c++
void Resource::addClient(ResourceClient* client) {
    if (addClientToSet(client))
        didAddClient(client);
}
```

​		Resource 类的成员函数 addClient 调用另外一个成员函数 addClientToSet 将参数 client 描述的一个DocumentLoader 对象保存在内部，如下所示：

```c++
bool Resource::addClientToSet(ResourceClient* client) {
    ......
 
    m_clients.add(client);
    return true;
}
```

​		Resource 类的成员函数 addClientToSet 将参数 client 描述的一个 DocumentLoader 保存在成员变量m_clients 描述的一个 Hash Set 中，以便当前正在处理的 Resource 对象描述的网页内容从 Web 服务器下载回来的时候，可以交给它处理。



​		接下来我们继续分析 WebDataSourceImpl 类的成员函数 startLoadingMainResource 调用成员变量m_fetcher 描述的 ResourceFetcher 对象的成员函数 fetchMainResource 加载本地变量cachedResourceRequest 描述的 URL 的过程，如下所示：

```c++
ResourcePtr<RawResource> ResourceFetcher::fetchMainResource(FetchRequest& request, const SubstituteData& substituteData)
{
    ......
    return toRawResource(requestResource(Resource::MainResource, request));
}
```

​		ResourceFetcher 类的成员函数 fetchMainResource 调用另外一个成员函数 requestResource 加载参数request 描述的 URL。ResourceFetcher 类的成员函数 requestResource 会返回一个 RawResource 对象给调用者，即 ResourceFetcher 类的成员函数 fetchMainResource。后者又会将这个 RawResource 对象返回给它的调用者。

​		ResourceFetcher类的成员函数requestResource的实现如下所示：

```c++
ResourcePtr<Resource> ResourceFetcher::requestResource(Resource::Type type, FetchRequest& request)
{
    ......
 
    KURL url = request.resourceRequest().url();
    ......
 
    const RevalidationPolicy policy = determineRevalidationPolicy(type, request.mutableResourceRequest(), request.forPreload(), resource.get(), request.defer(), request.options());
    switch (policy) {
    ......
    case Load:
        resource = createResourceForLoading(type, request, request.charset());
        break;
    .....
    }
 
    ......
 
    if (resourceNeedsLoad(resource.get(), request, policy)) {
        ......
 
        if (!m_documentLoader || !m_documentLoader->scheduleArchiveLoad(resource.get(), request.resourceRequest()))
            resource->load(this, request.options());
     
        ......
    }
 
    ......
 
    return resource;
}
```

​		ResourceFetcher 类的成员函数 requestResource 首先调用成员函数 createResourceForLoading 为参数 request 描述的 URL 创建一个 RawResource 对象，如下所示：

```c++
ResourcePtr<Resource> ResourceFetcher::createResourceForLoading(Resource::Type type, FetchRequest& request, const String& charset)
{
    ......
 
    ResourcePtr<Resource> resource = createResource(type, request.resourceRequest(), charset);
 
    ......
    return resource;
}
```

​		ResourceFetcher 类的成员函数 createResourceForLoading 调用函数 createResource 根据参数 type 和 request 创建一个 RawResource 对象，如下所示：

```c++
static Resource* createResource(Resource::Type type, const ResourceRequest& request, const String& charset)
{
    switch (type) {
    ......
    case Resource::MainResource:
    case Resource::Raw:
    case Resource::TextTrack:
    case Resource::Media:
        return new RawResource(request, type);
    ......
    }
 
    ......
    return 0;
}
```

​		从前面的调用过程可以知道，参数 type 的值等于 Resource::MainResource，因此函数 createResource创建的是一个 RawResource 对象。

​		回到 ResourceFetcher 类的成员函数 requestResource 中，它调用成员函数createResourceForLoading 为参数 request 描述的 URL 创建了一个 RawResource 对象之后，接下来又调用成员函数 resourceNeedsLoad 判断该 URL 是否需要进行加载。如果需要进行加载，那么 ResourceFetcher 类的成员函数 requestResource 又会调用成员变量 m_documentLoader 描述的一个DocumentLoader 对象的成员函数 scheduleArchiveLoad 判断要加载的 URL 描述的是否是一个存档文件。如果不是，那么就会调用前面创建的 RawResource 对象的成员函数 load 从 Web 服务器下载对应的网页内容。

​		我们假设 request 描述的 URL 需要进行加载，并且不是一个存档文件，因此接下来我们继续分析RawResource 类的成员函数 load 的实现。RawResource 类的成员函数 load 是从父类 Resource 继承下来的，它的实现如下所示：

```c++
void Resource::load(ResourceFetcher* fetcher, const ResourceLoaderOptions& options)
{
    ......
 
    ResourceRequest request(m_resourceRequest);
    ......
 
    m_loader = ResourceLoader::create(fetcher, this, request, options);
    m_loader->start();
}
```

​		Resource 类的成员变量 m_resourceRequest 描述的是要加载的 URL，Resource 类的成员函数 load 首先调用 ResourceLoader 类的静态成员函数 create 为其创建一个 ResourceLoader 对象，如下所示：

```c++
PassRefPtr<ResourceLoader> ResourceLoader::create(ResourceLoaderHost* host, Resource* resource, const ResourceRequest& request, const ResourceLoaderOptions& options)
{
    RefPtr<ResourceLoader> loader(adoptRef(new ResourceLoader(host, resource, options)));
    loader->init(request);
    return loader.release();
}
```

​		从这里可以看到，ResourceLoader 类的静态成员函数 create 创建的是一个 ResourceLoader 对象。这个ResourceLoader 对象经过初始化之后，会返回给调用者。

​		回到 Resource 类的成员函数 load 中，它为要加载的 URL 创建了一个 ResourceLoader 对象之后，会调用这个 ResourceLoader 对象的成员函数 start 开始加载要加载的 URL，如下所示：

```c++
void ResourceLoader::start()
{
    ......
 
    m_loader = adoptPtr(blink::Platform::current()->createURLLoader());
    ......
    blink::WrappedResourceRequest wrappedRequest(m_request);
    m_loader->loadAsynchronously(wrappedRequest, this);
}
```

​		ResourceLoader 类的成员函数 start 首先调用由 Chromium 的 Content 模块实现的一个blink::Platform 接口的成员函数 createURLLoader 创建一个 WebURLLoaderImpl 对象，接着再调用这个WebURLLoaderImpl 对象的成员函数 loadAsynchronously 对象成员变量 m_request 描述的 URL 进行异步加载。

​		Chromium 的 Content 模块的 BlinkPlatformImpl 类实现了 blink::Platform 接口，它的成员函数createURLLoader 的实现如下所示：

```c++
WebURLLoader* BlinkPlatformImpl::createURLLoader() {
  return new WebURLLoaderImpl;
}
```

​		从这里可以看到，BlinkPlatformImpl 类的成员函数 createURLLoader 创建的是一个WebURLLoaderImpl 对象。这个 WebURLLoaderImpl 对象会返回给调用者。

​		接下来我们继续分析 WebURLLoaderImpl 类的成员函数 loadAsynchronously 异步加载一个 URL 的过程，如下所示：

```c++
void WebURLLoaderImpl::loadAsynchronously(const WebURLRequest& request,
                                          WebURLLoaderClient* client) {
  ......
 
  context_->set_client(client);
  context_->Start(request, NULL);
}
```

​		从前面的调用过程可以知道，参数 client 描述的是一个 ResourceLoader 对象。这个ResourceLoader 对象会保存在 WebURLLoaderImpl 类的成员变量 content_ 描述的一个 WebURLLoaderImpl::Context 对象的内部。这是通过调用 WebURLLoaderImpl::Context 类的成员函数 set_client 实现的，如下所示：

```c++
class WebURLLoaderImpl::Context : public base::RefCounted<Context>,
                                  public RequestPeer {
 public:
  ......
 
  void set_client(WebURLLoaderClient* client) { client_ = client; }
 
 private:
  ......
 
  WebURLLoaderClient* client_;
  
  ......
};
```

​		WebURLLoaderImpl::Context 类的成员函数 set_client 将参数 client 描述的 ResourceLoader 对象保存在成员变量 client_ 中。

​		回到 WebURLLoaderImpl 类的成员函数 loadAsynchronously 中，它接下来会继续调用成员变量content_ 描述的一个 WebURLLoaderImpl::Context 对象的成员函数 Start 加载参数 request 描述的 URL，如下所示：

```c++
void WebURLLoaderImpl::Context::Start(const WebURLRequest& request,
                                      SyncLoadResponse* sync_load_response) {
  ......
 
  GURL url = request.url();
  ......
 
  RequestInfo request_info;
  ......
  request_info.url = url;
  ......
  bridge_.reset(ChildThread::current()->resource_dispatcher()->CreateBridge(
      request_info));
 
  ......
 
  if (bridge_->Start(this)) {
    AddRef();  // Balanced in OnCompletedRequest
  } else {
    bridge_.reset();
  }
}
```

​		WebURLLoaderImpl::Context 类的成员函数 Start 首先调用当前 Render 进程的一个 ChildThread 单例的成员函数 resource_dispatcher 获得一个 ResourceDispatcher 对象，如下所示：

```c++
class CONTENT_EXPORT ChildThread
    : public IPC::Listener,
      public IPC::Sender,
      public NON_EXPORTED_BASE(mojo::ServiceProvider) {
 public:
  ......
 
  ResourceDispatcher* resource_dispatcher() const {
    return resource_dispatcher_.get();
  }
 
  ......
 
 private:
  ......
 
  // Handles resource loads for this process.
  scoped_ptr<ResourceDispatcher> resource_dispatcher_;
 
  ......
};
```

​		ChildThread 类的成员函数 resource_dispatcher 返回的是成员变量 resource_dispatcher_ 描述的一个 ResourceDispatcher 对象。

​		回到 WebURLLoaderImpl::Context 类的成员函数 Start 中，它获得了一个 ResourceDispatcher 对象之后，接着调用这个 ResourceDispatcher 对象的成员函数 CreateBridge 创建一个IPCResourceLoaderBridge 对象，如下所示：

```c++
ResourceLoaderBridge* ResourceDispatcher::CreateBridge(
    const RequestInfo& request_info) {
  return new IPCResourceLoaderBridge(this, request_info);
}
```

​		从这里可以看到，ResourceDispatcher 类的成员函数 CreateBridge 创建的是一个IPCResourceLoaderBridge 对象，并且会将这个 IPCResourceLoaderBridge 对象返回给调用者。

​		回到 WebURLLoaderImpl::Context 类的成员函数 Start 中，它获得了一个 IPCResourceLoaderBridge对象之后，接着调用这个 IPCResourceLoaderBridg e对象的成员函数 Start 加载参数 request 描述的 URL，如下所示：

```c++
bool IPCResourceLoaderBridge::Start(RequestPeer* peer) {
  ......
 
  // generate the request ID, and append it to the message
  request_id_ = dispatcher_->AddPendingRequest(peer,
                                               request_.resource_type,
                                               request_.origin_pid,
                                               frame_origin_,
                                               request_.url,
                                               request_.download_to_file);
 
  return dispatcher_->message_sender()->Send(
      new ResourceHostMsg_RequestResource(routing_id_, request_id_, request_));
}
```

​		IPCResourceLoaderBridge 类的成员变量 dispatcher_ 描述的是一个 ResourceDispatcher 对象，IPCResourceLoaderBridge 类的成员函数 Start 首先调用这个 ResourceDispatcher 对象的成员函数AddPendingRequest 将参数 peer 描述的一个 WebURLLoaderImpl::Context 对象保存在内部，如下所示：

```c++
int ResourceDispatcher::AddPendingRequest(RequestPeer* callback,
                                          ResourceType::Type resource_type,
                                          int origin_pid,
                                          const GURL& frame_origin,
                                          const GURL& request_url,
                                          bool download_to_file) {
  // Compute a unique request_id for this renderer process.
  int id = MakeRequestID();
  pending_requests_[id] = PendingRequestInfo(callback,
                                             resource_type,
                                             origin_pid,
                                             frame_origin,
                                             request_url,
                                             download_to_file);
  return id;
}
```

​		ResourceDispatcher 类的成员函数 AddPendingRequest 首先调用成员函数 MakeRequestID 生成一个Request ID，接着将参数 callback 描述的一个 WebURLLoaderImpl::Context 对象封装在一个PendingRequestInfo 对象中，并且以上述 Request ID 为键值，将这个 PendingRequestInfo 对象保存在成员变量 pending_requests_ 描述的一个 Hash Map 中。

​		回到 IPCResourceLoaderBridge 类的成员函数 Start 中，它接下来调用成员变量 dispatcher_ 描述的ResourceDispatcher 对象的成员函数 message_sender 获得一个 IPC::Sender 对象，并且通过这个IPC::Sender 对象向 Browser 进程发送一个类型为 ResourceHostMsg_RequestResource 的 IPC 消息，用来请求 Browser 进程下载成员变量 request_ 描述的 URL 对应的网页的内容。



### Browser 进程

​		在 Browser 进程中，类型为 ResourceHostMsg_RequestResource 的 IPC 消息是由ResourceDispatcherHostImpl 类的成员函数 OnMessageReceived 进行接收的，如下所示：

```c++
bool ResourceDispatcherHostImpl::OnMessageReceived(
    const IPC::Message& message,
    ResourceMessageFilter* filter) {
  ......
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ResourceDispatcherHostImpl, message)
    IPC_MESSAGE_HANDLER(ResourceHostMsg_RequestResource, OnRequestResource)
    ......
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
 
  ......
}
```

​		ResourceDispatcherHostImpl 类的成员函数 OnMessageReceived 将类型为ResourceHostMsg_RequestResource 的 IPC 消息分发给另外一个成员函数 OnRequestResource 处理，后者的实现如下所示：

```c++
void ResourceDispatcherHostImpl::OnRequestResource(
    int routing_id,
    int request_id,
    const ResourceHostMsg_Request& request_data) {
  BeginRequest(request_id, request_data, NULL, routing_id);
}
```

​		ResourceDispatcherHostImpl 类的成员函数 OnRequestResource 调用另外一个成员函数BeginRequest 开始下载参数 request_data 描述的 URL 对应的网页内容，后者的实现如下所示：

```c++
void ResourceDispatcherHostImpl::BeginRequest(
    int request_id,
    const ResourceHostMsg_Request& request_data,
    IPC::Message* sync_result,  // only valid for sync
    int route_id) {
  ......
 
  // Construct the request.
  net::CookieStore* cookie_store =
      GetContentClient()->browser()->OverrideCookieStoreForRenderProcess(
          child_id);
  scoped_ptr<net::URLRequest> new_request;
  new_request = request_context->CreateRequest(
      request_data.url, request_data.priority, NULL, cookie_store);
  ......
 
  scoped_ptr<ResourceHandler> handler(
       CreateResourceHandler(
           new_request.get(),
           request_data, sync_result, route_id, process_type, child_id,
           resource_context));
 
  if (handler)
    BeginRequestInternal(new_request.Pass(), handler.Pass());
}
```

​		ResourceDispatcherHostImpl 类的成员函数 BeginRequest 首先从参数 request_data 取出要下载网页内容的 URL，接着又将该 URL 封装在一个 URLRequest 对象中。

​		ResourceDispatcherHostImpl 类的成员函数 BeginRequest 接下来又调用另外一个成员函数CreateResourceHandler 创建了一个 AsyncResourceHandler 对象。这个 AsyncResourceHandler 对象用来异步接收和处理从 Web 服务器下载回来的网页内容。

​		ResourceDispatcherHostImpl 类的成员函数 CreateResourceHandler 的实现如下所示：

```c++
scoped_ptr<ResourceHandler> ResourceDispatcherHostImpl::CreateResourceHandler(
    net::URLRequest* request,
    const ResourceHostMsg_Request& request_data,
    IPC::Message* sync_result,
    int route_id,
    int process_type,
    int child_id,
    ResourceContext* resource_context) {
  // Construct the IPC resource handler.
  scoped_ptr<ResourceHandler> handler;
  if (sync_result) {
    ......
 
    handler.reset(new SyncResourceHandler(request, sync_result, this));
  } else {
    handler.reset(new AsyncResourceHandler(request, this));
 
    // The RedirectToFileResourceHandler depends on being next in the chain.
    if (request_data.download_to_file) {
      handler.reset(
          new RedirectToFileResourceHandler(handler.Pass(), request));
    }
  }
 
  ......
 
  // Install a CrossSiteResourceHandler for all main frame requests.  This will
  // let us check whether a transfer is required and pause for the unload
  // handler either if so or if a cross-process navigation is already under way.
  bool is_swappable_navigation =
      request_data.resource_type == ResourceType::MAIN_FRAME;
  // If we are using --site-per-process, install it for subframes as well.
  if (!is_swappable_navigation &&
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kSitePerProcess)) {
    is_swappable_navigation =
        request_data.resource_type == ResourceType::SUB_FRAME;
  }
  if (is_swappable_navigation && process_type == PROCESS_TYPE_RENDERER)
    handler.reset(new CrossSiteResourceHandler(handler.Pass(), request));
 
  // Insert a buffered event handler before the actual one.
  handler.reset(
      new BufferedResourceHandler(handler.Pass(), this, request));
 
  ......
 
  handler.reset(
      new ThrottlingResourceHandler(handler.Pass(), request, throttles.Pass()));
 
  return handler.Pass();
}
```

​		从前面的调用过程可以知道，参数 sync_result 的值等于 NULL，因此 ResourceDispatcherHostImpl类的成员函数 CreateResourceHandle r首先创建了一个 AsyncResourceHandler 对象，保存在本地变量handler 中，表示要通过异步方式下载参数 request 描述的 URL。

​		接下来 ResourceDispatcherHostImpl 类的成员函数 CreateResourceHandler 又会根据情况创建其它的 Handler 对象。这些 Handler 对象会依次连接在一起。其中，后面创建的 Handler 对象位于前面创建的Handler 对象的前面。下载回来的网页内容将依次被这些 Handler 对象处理。这意味着下载回来的网页内容最后会被最先创建的 AsyncResourceHandler 对象进行处理。为了简单起见，后面我们只分析这个AsyncResourceHandler 对象处理下载回来的网页内容的过程，也就是假设 ResourceDispatcherHostImpl类的成员函数 CreateResourceHandler 返回给调用者的是一个 AsyncResourceHandler 对象。

​		回到 ResourceDispatcherHostImpl 类的成员函数 BeginRequest 中，它最后调用另外一个成员函数BeginRequestInternal 下载本地变量 new_request 描述的 URL 对应的网页内容，如下所示：

```c++
void ResourceDispatcherHostImpl::BeginRequestInternal(
    scoped_ptr<net::URLRequest> request,
    scoped_ptr<ResourceHandler> handler) {
  ......
 
  ResourceRequestInfoImpl* info =
      ResourceRequestInfoImpl::ForRequest(request.get());
  ......
 
  linked_ptr<ResourceLoader> loader(
      new ResourceLoader(request.Pass(), handler.Pass(), this));
 
  .....
 
  StartLoading(info, loader);
}
```

​		ResourceDispatcherHostImpl 类的成员函数 BeginRequestInternal 将参数 request 描述的 URL 和参数 handler 描述的 AsyncResourceHandler 对象封装在一个 ResourceLoader 对象后，调用另外一个成员函数 StartLoading 开始加载参数 request 描述的 URL。

​		ResourceDispatcherHostImpl 类的成员函数 StartLoading 的实现如下所示：

```c++
void ResourceDispatcherHostImpl::StartLoading(
    ResourceRequestInfoImpl* info,
    const linked_ptr<ResourceLoader>& loader) {
  ......
 
  loader->StartRequest();
}
```

​		ResourceDispatcherHostImpl 类的成员函数 StartLoading 主要是调用参数 loader 描述的ResourceLoader 对象的成员函数 StartRequest 开始加载其内部封装的 URL。

​		ResourceLoader 类的成员函数 StartRequest 的实现如下所示：

```c++
void ResourceLoader::StartRequest() {
  ......
 
  // Give the handler a chance to delay the URLRequest from being started.
  bool defer_start = false;
  if (!handler_->OnWillStart(request_->url(), &defer_start)) {
    Cancel();
    return;
  }
 
  if (defer_start) {
    deferred_stage_ = DEFERRED_START;
  } else {
    StartRequestInternal();
  }
}
```

​		ResourceLoader 类的成员变量 handler_ 描述的便是前面我们假设 ResourceDispatcherHostImpl 类的成员函数 CreateResourceHandler 返回的 AsyncResourceHandler 对象。ResourceLoader 类的成员函数 StartRequest 调用这个 AsyncResourceHandler 对象的成员函数 OnWillStart 询问是要取消、延迟、还是马上下载当前正在处理的 ResourceLoader 对象封装的 URL 对应的网页内容。

​		我们假设是第三种情况，这时候 ResourceLoader 类的成员函数 StartRequest 就会马上调用另外一个成员函数 StartRequestInternal 下载当前正在处理的 ResourceLoader 对象封装的 URL 对应的网页内容。

​			ResourceLoader 类的成员函数 StartRequestInternal 的实现如下所示：

```c++
void ResourceLoader::StartRequestInternal() {
  ......
 
  request_->Start();
 
  ......
}
```

​		ResourceLoader 类的成员变量 request_ 描述的是前面在 ResourceDispatcherHostImpl 类的成员函数 BeginRequest 中创建的一个 URLRequest 对象。这个 URLRequest 对象封装了要下载的 URL。ResourceLoader 类的成员函数 StartRequestInternal 通过调用这个 URLRequest 对象的成员函数 Start 就可以启动下载网页的过程了。

​		URLRequest 类是 Chromium 在 Net 模块中提供的一个类，用来执行具体的网络操作，也就是根据约定的协议请求 Web 服务器返回指定 URL 对应的网页的内容。这个过程我们留给读者自行分析。

​		Web 服务器响应了请求之后，Chromium 的 Ne t模块会调用 ResourceLoader 类的成员函数OnResponseStarted，它的实现如下所示：

```c++
void ResourceLoader::OnResponseStarted(net::URLRequest* unused) {
  ......
 
  if (request_->status().is_success()) {
    StartReading(false);  // Read the first chunk.
  } 
 
  ......
}
```

​		ResourceLoader 类的成员函数 OnResponseStarted 检查 Web 服务器的响应是否成功，例如 Web 服务器是否根据 HTTP 协议返回了 200 响应。如果成功的话，那么接下来就会调用另外一个成员函数StartReading 读出第一块数据。

​		ResourceLoader 类的成员函数 StartReading 的实现如下所示：

```c++
void ResourceLoader::StartReading(bool is_continuation) {
  int bytes_read = 0;
  ReadMore(&bytes_read);
 
  ......
 
  if (!is_continuation || bytes_read <= 0) {
    OnReadCompleted(request_.get(), bytes_read);
  } else {
    // Else, trigger OnReadCompleted asynchronously to avoid starving the IO
    // thread in case the URLRequest can provide data synchronously.
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ResourceLoader::OnReadCompleted,
                   weak_ptr_factory_.GetWeakPtr(),
                   request_.get(),
                   bytes_read));
  }
}
```

​		ResourceLoader 类的成员函数 StartReading 调用成员函数 ReadMore 读取 Web 服务器返回来的数据，读出来的数据大小保存在本地变量 bytes_read 中。

​		ResourceLoader 类的成员函数 ReadMore 的实现如下所示：

```c++
void ResourceLoader::ReadMore(int* bytes_read) {
  ......
 
  scoped_refptr<net::IOBuffer> buf;
  int buf_size;
  if (!handler_->OnWillRead(&buf, &buf_size, -1)) {
    Cancel();
    return;
  }
 
  ......
 
  request_->Read(buf.get(), buf_size, bytes_read);
 
  ......
}
```

​		ResourceLoader 类的成员函数 ReadMore 首先调用成员变量 handler_ 描述的一个AsyncResourceHandler 对象的成员函数 OnWillRead 获取一个 Buffer。这个 Buffer 用来保存从 Web 服务器返回来的数据。这些数据可以通过调用 ResourceLoader 类的成员变量 reqeust_ 描述的一个 URLRequest对象的成员函数 Read 获得。  

​		AsyncResourceHandler 对象的成员函数 OnWillRead 的实现如下所示：

```c++
bool AsyncResourceHandler::OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                                      int* buf_size,
                                      int min_size) {
  ......
 
  if (!EnsureResourceBufferIsInitialized())
    return false;
 
  ......
  char* memory = buffer_->Allocate(&allocation_size_);
  .....
 
  *buf = new DependentIOBuffer(buffer_.get(), memory);
  *buf_size = allocation_size_;
 
  ......
 
  return true;
}
```

​		AsyncResourceHandler 对象的成员函数 OnWillRead 首先调用成员函数EnsureResourceBufferIsInitialized 确保成员变量 buffer_ 指向了一块共享内存，然后再从这块共享内存中分配一块大小等于成员变量 allocation_size_ 的值的缓冲区，用来返回给调用者保存从 Web 服务器返回来的数据。 

​		AsyncResourceHandler 类的成员函数 EnsureResourceBufferIsInitialized 的实现如下所示：

```c++
bool AsyncResourceHandler::EnsureResourceBufferIsInitialized() {
  if (buffer_.get() && buffer_->IsInitialized())
    return true;
 
  ......
 
  buffer_ = new ResourceBuffer();
  return buffer_->Initialize(kBufferSize,
                             kMinAllocationSize,
                             kMaxAllocationSize);
}
```

​		AsyncResourceHandler 类的成员函数 EnsureResourceBufferIsInitialized 首先检查成员变量 buffer_ 是否指向了一个 ResourceBuffer 对象，并且这个 ResourceBuffer 对象描述的共享内存是否已经创建。

​		如果 AsyncResourceHandler 类的成员变量 buffer_ 还没有指向一个 ResourceBuffer 对象，或者指向了一个 ResourceBuffer 对象，但是这个 ResourceBuffer 对象描述的共享内存还没有创建，那么AsyncResourceHandler 类的成员函数 EnsureResourceBufferIsInitialized 就会创建一个 ResourceBuffer对象保存在成员变量 buffer_ 中，并且调用这个 ResourceBuffer 对象的成员函数 Initialize 创建一块大小为kBufferSize 的共享内存。这块共享内存每次可以分配出来的缓冲区最小值为 kMinAllocationSize ，最大值为 kMaxAllocationSize。

​		这一步执行完成后，回到 ResourceLoader 类的成员函数 StartReading 中，如果没有读出数据（表明数据已经下载完毕），或者参数 is_continuation 的值等于 false（表示读出来的是第一个数据块），那么ResourceLoader 类的成员函数 StartReading 就会调用成员函数 OnReadCompleted 马上进行下一步处理。其余情况下，为了避免当前（网络）线程被阻塞，ResourceLoader 类的成员函数 StartReading 并不会马上调用成员函数 OnReadCompleted 处理读出来的数据，而是延后一个消息处理，也就是等 ResourceLoader 类的成员函数 StartReading 返回到 Chromium 的 Net 模块之后再作处理。

​		接下来我们继续分析 ResourceLoader 类的成员函数 OnReadCompleted 的实现，如下所示：

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

​		ResourceLoader 类的成员函数 OnReadCompleted 首先调用成员函数 CompleteRead 处理当前读出来的数据，数据的大小由参数 bytes_read 描述。如果当前读出来的数据的大小大于 0，那么就表示数据还没读完，这时候就需要调用前面分析的成员函数 StartReading 继续进行读取。注意，这时候传递成员函数StartReading 的参数为 true，表示不是第一次读取 Web 服务器返回来的数据。

​		另一方面，如果当前读出来的数据的大小小于等于 0，那么就说明 Web 服务器已经把所有的数据都返回来了，这时候 ResourceLoader 类的成员函数 OnReadCompleted 就调用另外一个成员函数ResponseCompleted 结束读取数据。

​		接下来我们继续分析 ResourceLoader 类的成员函数 CompleteRead 的实现，以便了解 Browser 进程将下载回来的网页内容返回给 Render 进程处理的过程，如下所示：

```c++
void ResourceLoader::CompleteRead(int bytes_read) {
  ......
 
  bool defer = false;
  if (!handler_->OnReadCompleted(bytes_read, &defer)) {
    Cancel();
  } 
 
  ......
}
```

​		ResourceLoader 类的成员函数 CompleteRead 将读取出来的数据交给成员变量 handler_ 描述的一个AsyncResourceHandler 对象处理，这是通过调用它的成员函数 OnReadCompleted 实现的。

​		AsyncResourceHandler 类的成员函数 OnReadCompleted 的实现如下所示：

```c++
bool AsyncResourceHandler::OnReadCompleted(int bytes_read, bool* defer) {
  ......
 
  if (!sent_first_data_msg_) {
    base::SharedMemoryHandle handle;
    int size;
    if (!buffer_->ShareToProcess(filter->PeerHandle(), &handle, &size))
      return false;
    filter->Send(new ResourceMsg_SetDataBuffer(
        GetRequestID(), handle, size, filter->peer_pid()));
    sent_first_data_msg_ = true;
  }
 
  int data_offset = buffer_->GetLastAllocationOffset();
 
  int64_t current_transfer_size = request()->GetTotalReceivedBytes();
  int encoded_data_length = current_transfer_size - reported_transfer_size_;
  reported_transfer_size_ = current_transfer_size;
 
  filter->Send(new ResourceMsg_DataReceived(
      GetRequestID(), data_offset, bytes_read, encoded_data_length));
 
  ......
}
```

​		当 AsyncResourceHandler 类的成员变量 sent_first_data_msg_ 的值等于 false 的时候，表示当前正在处理的 AsyncResourceHandler 对象还没有向 Render 进程返回过从 We b服务器下载回来的网页内容。这时候 AsyncResourceHandler 类的成员函数 OnReadCompleted 首先要向 Render 进程发送一个类型为**ResourceMsg_SetDataBuffer** 的 IPC 消息。这个 IPC 消息会将 AsyncResourceHandler 类的成员变量buffer_ 描述的共享内存传递给 Render 进程，以便 Render 进程接下来可以通过这块共享内存读取从 Web 服务器下载回来的网页内容。

​		最后，AsyncResourceHandler 类的成员函数 OnReadCompleted 再向 Render 进程发送一个类型为ResourceMsg_DataReceived 的 IPC 消息。这个 IPC 消息告诉 Render 进程从前面所描述的共享内存的什么位置开始读取多少数据。有了这些数据之后，Render 进程就可以构建网页的 DOM Tree 了。

### DOM Tree



















