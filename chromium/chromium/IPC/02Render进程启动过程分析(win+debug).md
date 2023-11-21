[TOC]

# 概览

![02](./markdownimage/02renderipc.jpg)



## Browser 进程

​		RenderProcessHost、RenderViewHost、RenderProcess 和 RenderView 仅仅是定义了一个抽象接口，真正用来执行 IPC 通信的对象，是实现了上述抽象接口的一个实现者对象，这些实现者对象的类型以 Impl 结尾，因此，RenderProcessHost、RenderViewHost、RenderProcess 和 RenderView 对应的实现者对象的类型就分别为 RenderProcessHostImpl、RenderViewHostImpl、RenderProcessImpl 和 RenderViewImpl。

​		为了更好地理解 Render 进程的启动过程，我们有必要了解上述 Impl 对象的类关系图。

​		RenderViewHostImpl 对象的类关系图如下所示：

![02](./markdownimage/02renderprocesshost.drawio.png)

​		RenderViewHostImpl 类多重继承了 **RenderViewHost** 类和 **RenderWidgetHostOwnerDelegate** 类，RenderWidgetHostOwnerDelegate 是由 RenderViewHostImpl 和 RenderWidgetHostImpl 解开耦合的中间类，所以可以看到一些解析里面使用的是 RenderWidgetHostImpl。一般是在 RenderWidgetHostImpl 进行一些IPC消息，但是这里没有进行继承，是使用了成员变量来进行这些操作。

​		RenderProcessHostImpl 类实现了 RenderProcessHost 接口，后者又多重继承了 Sender 和 Listener 类。

​		**<u>RenderProcessHostImpl 类有一个成员变量 channel_，它指向了一个 ChannelProxy 对象。ChannelProxy 类实现了 Sende r接口，RenderProcessHostImpl 类就是通过它来发送 IPC 消息的。</u>**

​		**<u>ChannelProxy 类有一个成员变量 context_，它指向了一个 ChannelProxy::Context 对象。ChannelProxy::Context 类实现了Listener 接口，因此它可以用来接收 IPC 消息。ChannelProxy 类就是通过 ChannelProxy::Context 类来发送和接收 IPC 消息的。</u>**

​		**<u>ChannelProxy::Context 类有一个类型为 Channel 的成员变量 channel_，它指向的实际上是一个 ChannelPosix 对象。ChannelPosix 类继承了 Channe l类，后者又实现了 Sender 接口。ChannelProxy::Context 类就是通过 ChannelPosix 类发送IPC消息的。</u>**

​		绕了一圈，总结来说，**<u>就是 RenderProcessHostImpl 类是分别通过 ChannelPosix 类和 ChannelProxy::Context 类来发送和接收IPC消息的。</u>**







## Render 进程

​		上面分析的 RenderViewHostImpl 对象和 RenderProcessHostImpl 对象都是运行在 Browser 进程的，接下来要分析的RenderViewImpl 类和 RenderProcessImpl 类是运行在 Render 进程的。

​		RenderViewImpl 对象的类关系图如下所示：

![02renderprocessdrawio](./markdownimage/02renderprocess.drawio.png)

​		RenderViewImpl 类多重继承了 **RenderView** 类和 **RenderWidget** 类。RenderView 类实现了 Sender 接口。RenderWidget 类也实现了 Sender 接口，同时也实现了 Listener 接口，因此它可以用来发送和接收 IPC 消息。

​		RenderWidget 类实现了接口 Sender 的成员函数 **`Send`**，RenderViewImpl 类就是通过它来发送 IPC 消息的。RenderWidget 类的成员函数 **`Send`** 又是通过一个用来描述 Render 线程的 **RenderThreadImpl** 对象来发送IPC 类的。这个 RenderThreadImpl 对象可以通过调用 RenderThread 类的静态成员函数**`Get`**获得。

​      **RenderThreadImpl** 对象的类关系图如下所示：

![02renderthreadimpl](./markdownimage/02renderthreadimpl.jpg)

​		RenderThreadImpl 类多重继承了 RenderThread 类和 ChildThread 类。RenderThread 类实现了 Sender 接口。ChildThread 类也实现 Sender 接口，同时也实现了 Listener 接口，因此它可以用来发送和接收 IPC 消息。

​		ChildThread 类有一个成员变量 channel_ ，它指向了一个 SyncChannel 对象。SyncChannel 类继承了上面提到的 ChannelProxy 类，因此，ChildThread 类通过其成员变量 channel_ 指向的 SyncChannel 对象可以发送IPC 消息。

​		从上面的分析又可以知道，ChannelProxy 类最终是通过 ChannelPosix 类发送 IPC 消息的，因此总结来说，就是 **<u>RenderThreadImpl 是通过 ChannelPosix 类发送 IPC 消息的</u>**。

​       接下来我们再来看 RenderProcessImpl 对象的类关系图，如下所示：

![02renderprocessimpl](./markdownimage/02renderprocessimpl.png)

​		RenderProcessImpl 类继承了 RenderProcess 类，RenderProcess 类又继承了 ChildProcess 类。ChildProcess 类有一个成员变量 **io_thread_**，它指向了一个 Thread 对象。该 Thread 对象描述的就是 Render 进程的 IO 线程。



## 启动

​		有了上面的基础知识之后，接下来我们开始分析 Render 进程的启动过程。我们将 Render 进程的启动过程划分为两部分：

- <u>第一部分是在 Browser 进程中执行的，它主要负责创建一个 UNIX Socket，并且将该 UNIX Socket 的 Client端描述符传递给接下来要创建的 Render 进程。</u>
- <u>第二部分是在 Render 进程中执行的，它负责执行一系列的初始化工作，其中之一就是将 Browser 进程传递过来的 UNIX Socket 的 Client 端描述符封装在一个 Channel 对象中，以便以后可以通过它来和 Browser 进程执行 IPC。</u>



### Browser 进程

​		Render 进程启动过程的第一部分子过程如下所示：

![02renderprocesslaunch](./markdownimage/02renderprocesslaunch.png)

​		上图列出的仅仅是一些核心过程，接下来我们通过代码来分析这些核心过程。

​		我们首先了解什么情况下 Browser 进程会启动一个 Render 进程。**<u>当我们在 Chromium的 地址栏输入一个网址，然后进行加载的时候，Browser 进程经过判断，发现需要在一个新的 Render 进程中渲染该网址的内容时，就会创建一个 RenderViewHostImpl 对象，并且调用它的成员函数 CreateRenderView 触发启动一个新的 Render 进程。后面我们分析 WebView 加载一个 URL 的时候，就会看到触发创建 RenderViewHostImpl 对象的流程。</u>**



```c++
// PlzNavigate
bool RenderFrameHostManager::CreateSpeculativeRenderFrameHost(
    SiteInstance* old_instance,
    SiteInstance* new_instance) {
  if (!new_instance->GetProcess()->Init())
    return false;

  CreateProxiesForNewRenderFrameHost(old_instance, new_instance);

  speculative_render_frame_host_ =
      CreateRenderFrame(new_instance, delegate_->IsHidden(), nullptr);

  if (speculative_render_frame_host_) {
    speculative_render_frame_host_->render_view_host()
        ->DispatchRenderViewCreated();
  }

  return !!speculative_render_frame_host_;
}
```

**`CreateSpeculativeRenderFrameHost`**这个函数的主要目的是在导航过程中预先创建一个“推测性”的 `RenderFrameHost` 实例。这一过程是浏览器预测性能优化的一部分，特别是在处理页面导航时。

​      这里我们主要关注RenderViewHostImpl 类的构造函数调用该 **RenderWidgetHostImpl** 对象的成员函数 **`get() `**获得一个 **RenderProcessHost** 对象，如下所示：

```c++
RenderProcessHost* SiteInstanceImpl::GetProcess() {
	......

  // Create a new process if ours went away or was reused.
  if (!process_) {
    BrowserContext* browser_context = browsing_instance_->browser_context();

    // Check if the ProcessReusePolicy should be updated.
    bool should_use_process_per_site =
        has_site_ &&
        RenderProcessHost::ShouldUseProcessPerSite(browser_context, site_);
    if (should_use_process_per_site) {
      process_reuse_policy_ = ProcessReusePolicy::PROCESS_PER_SITE;
    } else if (process_reuse_policy_ == ProcessReusePolicy::PROCESS_PER_SITE) {
      process_reuse_policy_ = ProcessReusePolicy::DEFAULT;
    }

    process_ = RenderProcessHostImpl::GetProcessHostForSiteInstance(
        browser_context, this);

    CHECK(process_);
    process_->AddObserver(this);

    // If we are using process-per-site, we need to register this process
    // for the current site so that we can find it again.  (If no site is set
    // at this time, we will register it in SetSite().)
    if (process_reuse_policy_ == ProcessReusePolicy::PROCESS_PER_SITE &&
        has_site_) {
      RenderProcessHostImpl::RegisterProcessHostForSite(browser_context,
                                                        process_, site_);
    }

    TRACE_EVENT2("navigation", "SiteInstanceImpl::GetProcess",
                 "site id", id_, "process id", process_->GetID());
    GetContentClient()->browser()->SiteInstanceGotProcess(this);

    if (has_site_)
      LockToOriginIfNeeded();
  }
  DCHECK(process_);

  return process_;
}
```

​       这个函数定义在文件 content/browser/site_instance_impl.cc 中。

​       SiteInstanceImpl 对象的成员变量 process_ 是一个 **RenderProcessHost** 指针，它从 **`RenderProcessHostImpl::GetProcessHostForSiteInstance`** 中获取的值，它的实现如下：

```c++
// static
RenderProcessHost* RenderProcessHostImpl::GetProcessHostForSiteInstance(
    BrowserContext* browser_context,
    SiteInstanceImpl* site_instance) {
  const GURL site_url = site_instance->GetSiteURL();
  SiteInstanceImpl::ProcessReusePolicy process_reuse_policy =
      site_instance->process_reuse_policy();
  bool is_for_guests_only = site_url.SchemeIs(kGuestScheme);
  RenderProcessHost* render_process_host = nullptr;

  bool is_unmatched_service_worker = site_instance->is_for_service_worker();

  // First, attempt to reuse an existing RenderProcessHost if necessary.
  switch (process_reuse_policy) {
    case SiteInstanceImpl::ProcessReusePolicy::PROCESS_PER_SITE:
      render_process_host = GetProcessHostForSite(browser_context, site_url);
      break;
    case SiteInstanceImpl::ProcessReusePolicy::USE_DEFAULT_SUBFRAME_PROCESS:
      DCHECK(SiteIsolationPolicy::IsTopDocumentIsolationEnabled());
      DCHECK(!site_instance->is_for_service_worker());
      render_process_host = GetDefaultSubframeProcessHost(
          browser_context, site_instance, is_for_guests_only);
      break;
    case SiteInstanceImpl::ProcessReusePolicy::REUSE_PENDING_OR_COMMITTED_SITE:
      render_process_host =
          FindReusableProcessHostForSite(browser_context, site_url);
      UMA_HISTOGRAM_BOOLEAN(
          "SiteIsolation.ReusePendingOrCommittedSite.CouldReuse",
          render_process_host != nullptr);
      if (render_process_host)
        is_unmatched_service_worker = false;
      break;
    default:
      break;
  }

  // If not, attempt to reuse an existing process with an unmatched service
  // worker for this site. Exclude cases where the policy is DEFAULT and the
  // site instance is for a service worker. We use DEFAULT when we have failed
  // to start the service worker before and want to use a new process.
  if (!render_process_host &&
      !(process_reuse_policy == SiteInstanceImpl::ProcessReusePolicy::DEFAULT &&
        site_instance->is_for_service_worker())) {
    render_process_host = UnmatchedServiceWorkerProcessTracker::MatchWithSite(
        browser_context, site_url);
  }

  // If not (or if none found), see if we should reuse an existing process.
  if (!render_process_host &&
      ShouldTryToUseExistingProcessHost(browser_context, site_url)) {
    render_process_host = GetExistingProcessHost(browser_context, site_url);
  }

  // Otherwise, use the spare RenderProcessHost or create a new one.
  if (!render_process_host) {
    // Pass a null StoragePartition. Tests with TestBrowserContext using a
    // RenderProcessHostFactory may not instantiate a StoragePartition, and
    // creating one here with GetStoragePartition() can run into cross-thread
    // issues as TestBrowserContext initialization is done on the main thread.
    render_process_host = CreateOrUseSpareRenderProcessHost(
        browser_context, nullptr, site_instance, is_for_guests_only);
  }

  if (is_unmatched_service_worker) {
    UnmatchedServiceWorkerProcessTracker::Register(
        browser_context, render_process_host, site_url);
  }

  // Make sure the chosen process is in the correct StoragePartition for the
  // SiteInstance.
  CHECK(render_process_host->InSameStoragePartition(
      BrowserContext::GetStoragePartition(browser_context, site_instance,
                                          false /* can_create */)));

  return render_process_host;
}
```

​		首先，它从提供的 `site_instance` 获取站点URL，并确定进程复用策略。该函数尝试根据进程复用策略重用现有的 RenderProcessHost：

- `PROCESS_PER_SITE`：为每个站点分配一个进程，如果可能，重用相同站点的现有进程。
- `USE_DEFAULT_SUBFRAME_PROCESS`：对于顶层文档隔离启用的情况，使用默认的子框架进程。
- `REUSE_PENDING_OR_COMMITTED_SITE`：重用挂起或已提交站点的进程。

如果没有找到合适的进程重用，代码会尝试匹配一个现有的与服务工作者（`service worker`）不匹配的进程。

如果上述方法都未找到合适的进程，代码会检查是否应该尝试重用现有的进程。

如果还是没有找到，代码会使用空闲的 `RenderProcessHost` 或创建一个新的进程。

如果这个过程是为了一个不匹配的服务工作者，那么它会注册到 `UnmatchedServiceWorkerProcessTracker`。

最后，代码确保选择的进程位于正确的存储分区，并返回这个进程。

​		注意上述 RenderProcessHostImpl 对象的创建过程：

1. <u>如果 Chromium 启动时，指定了**同一个网站**的所有网页都在同一个 Render 进程中加载，即本地变量 use_ process_ per_site 的值等于 true，那么这时候 **SiteInstanceImpl** 类的成员函数 GetProcess 就会先调用RenderProcessHostImpl 类的静态函 数GetProcessHostForSite 检查之前是否已经为当前正在处理的SiteInstanceImpl 对象描述的网站创建过 Render 进程。如果已经创建过，那么就可以获得一个对应的RenderProcessHostImpl 对象。</u>
2. <u>如果按照上面的方法找不到一个相应的 RenderProcessHostImpl 对象，本来就应该要创建一个新的 Render 进程了，也就是要创建一个新的 RenderProcessHostImpl 对象了。但是由于当前创建的 Render 进程已经超出预设的最大数量了，这时候就要复用前面已经启动的 Rende r进程，即使这个 Render 进程加载的是另一个网站的内容。</u>
3. <u>如果通过前面两步仍然找不到一个对应的 RenderProcessHostImpl 对象，这时候就真的是需要创建一个RenderProcessHostImpl 对象了。取决于 SiteInstanceImpl 类的静态成员变量 g_render _ process _host _factory _  是否被设置，创建一个新的 RenderProcessHostImpl 对象的方式有所不同。如果该静态成员变量被设置了指向一个 RenderProcessHostFactory 对象，那么就调用该RenderProcessHostFactory 对象的成员函数 CreateRenderProcessHost 创建一个从 RenderProcessHost 类继承下来的子类对象。否则的话，就直接创建一个 RenderProcessHostImpl 对象。</u>

下面看看是如何创建新进程的：

```c++
RenderProcessHost* RenderProcessHostImpl::GetProcessHostForSiteInstance(
    BrowserContext* browser_context,
    SiteInstanceImpl* site_instance) {
    
......
render_process_host = CreateOrUseSpareRenderProcessHost(
		browser_context, nullptr, site_instance, is_for_guests_only);
......
    
}

// static
RenderProcessHost* RenderProcessHostImpl::CreateRenderProcessHost(
    BrowserContext* browser_context,
    StoragePartitionImpl* storage_partition_impl,
    SiteInstance* site_instance,
    bool is_for_guests_only) {
  if (g_render_process_host_factory_) {
    return g_render_process_host_factory_->CreateRenderProcessHost(
        browser_context);
  }

  if (!storage_partition_impl) {
    storage_partition_impl = static_cast<StoragePartitionImpl*>(
        BrowserContext::GetStoragePartition(browser_context, site_instance));
  }

  return new RenderProcessHostImpl(browser_context, storage_partition_impl,
                                   is_for_guests_only);
}

// static
RenderProcessHost* RenderProcessHostImpl::CreateOrUseSpareRenderProcessHost(
    BrowserContext* browser_context,
    StoragePartitionImpl* storage_partition_impl,
    SiteInstance* site_instance,
    bool is_for_guests_only) {
  RenderProcessHost* render_process_host =
      g_spare_render_process_host_manager.Get().MaybeTakeSpareRenderProcessHost(
          browser_context, storage_partition_impl, site_instance,
          is_for_guests_only);

  if (!render_process_host) {
    render_process_host =
        CreateRenderProcessHost(browser_context, storage_partition_impl,
                                site_instance, is_for_guests_only);
  }

  DCHECK(render_process_host);
  return render_process_host;
}
```

在 `RenderProcessHostImpl::GetProcessHostForSiteInstance` 函数中，`CreateOrUseSpareRenderProcessHost` 是用于创建或重用一个备用的 `RenderProcessHost`。我们可以基于您提供的代码进一步分析这一部分的功能和工作流程：

​		`CreateOrUseSpareRenderProcessHost` 函数首先尝试通过调用 `g_spare_render_process_host_manager.Get().MaybeTakeSpareRenderProcessHost` 获取一个备用的 `RenderProcessHost`。备用进程主机管理器（`g_spare_render_process_host_manager`）维护一个备用的 `RenderProcessHost` 池，可以快速分配给需要的 `SiteInstance`。`MaybeTakeSpareRenderProcessHost` 考虑了多个因素，包括浏览器上下文（`BrowserContext`）、存储分区（`StoragePartitionImpl`）、站点实例（`SiteInstance`）和是否为客人模式（`is_for_guests_only`）。这些因素确保选择的备用进程与请求的站点兼容。

​		如果没有合适的备用 `RenderProcessHost` 可用，`CreateOrUseSpareRenderProcessHost` 函数将调用 `CreateRenderProcessHost` 来创建一个新的进程实例。这确保了即使没有可用的备用进程，请求也能被满足。

**CreateRenderProcessHost 的实现**：

- `CreateRenderProcessHost` 首先检查是否有全局的 `RenderProcessHost` 工厂（`g_render_process_host_factory_`）。如果有，它将使用这个工厂来创建新的 `RenderProcessHost`。这通常用于测试或自定义进程创建逻辑。
- 如果没有设置自定义工厂，函数会获取或确认 `StoragePartitionImpl`。如果没有提供 `storage_partition_impl`，它将根据 `BrowserContext` 和 `SiteInstance` 查找合适的存储分区。
- 最后，它创建一个新的 `RenderProcessHostImpl` 实例并返回。这个新实例是根据提供的浏览器上下文、存储分区和是否为客人模式创建的。

**RenderProcessHostImpl** 在 Chromium 架构中扮演着重要的角色。它是 `RenderProcessHost` 接口的具体实现，负责管理浏览器进程和渲染进程之间的交互。我们主要讲解中的**`InitializeChannelProxy`**；

### RenderProcessHostImpl

```c++
RenderProcessHostImpl::RenderProcessHostImpl(
    BrowserContext* browser_context,
    StoragePartitionImpl* storage_partition_impl,
    bool is_for_guests_only)
    : ...... {
 ......
  InitializeChannelProxy();
 ......
}


void RenderProcessHostImpl::InitializeChannelProxy() {
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner =
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);

  service_manager::Connector* connector =
      BrowserContext::GetConnectorFor(browser_context_);
  if (!connector) {
    if (!ServiceManagerConnection::GetForProcess()) 
      ServiceManagerConnection::SetForProcess(ServiceManagerConnection::Create(
          mojo::MakeRequest(&test_service_), io_task_runner));
    }
    connector = ServiceManagerConnection::GetForProcess()->GetConnector();
  }

  broker_client_invitation_ =
      base::MakeUnique<mojo::edk::OutgoingBrokerClientInvitation>();
  service_manager::Identity child_identity(
      mojom::kRendererServiceName,
      BrowserContext::GetServiceUserIdFor(GetBrowserContext()),
      base::StringPrintf("%d_%d", id_, instance_id_++));
  child_connection_.reset(new ChildConnection(child_identity,
                                              broker_client_invitation_.get(),
                                              connector, io_task_runner));

  mojo::MessagePipe pipe;
  BindInterface(IPC::mojom::ChannelBootstrap::Name_, std::move(pipe.handle1));
  std::unique_ptr<IPC::ChannelFactory> channel_factory =
      IPC::ChannelMojo::CreateServerFactory(std::move(pipe.handle0),
                                            io_task_runner);

  ResetChannelProxy();

  if (!channel_)
    channel_.reset(new IPC::ChannelProxy(this, io_task_runner.get()));
  channel_->Init(std::move(channel_factory), true /* create_pipe_now */);

  channel_->GetRemoteAssociatedInterface(&remote_route_provider_);
  channel_->GetRemoteAssociatedInterface(&renderer_interface_);

  channel_->Pause();
}

```

​		<u>`RenderProcessHostImpl::InitializeChannelProxy` 函数在 Chromium 代码中负责设置与渲染进程的通信通道。这个函数的主要职责是创建和初始化 IPC（Inter-Process Communication）通道，该通道用于浏览器进程和渲染进程之间的通信。</u>下面是对这个函数的关键部分的分析：

1. **获取 I/O 线程的任务运行器**：
   - `io_task_runner` 获取用于 I/O 操作的线程的任务运行器。这是因为大部分与渲染进程通信的操作都在 I/O 线程上执行。
2. **获取 Service Manager 连接器**：
   - 获取 `service_manager::Connector` 实例，用于与 Service Manager 建立连接。这个连接器用于路由到新的渲染服务实例。
3. **处理连接器的备用情况**：
   - 如果没有针对每个 `BrowserContext` 初始化连接器，代码会回退使用浏览器全局的连接器。在一些测试环境中，可能需要初始化一个虚拟的连接器。
4. **建立 Service Manager 连接**：
   - 使用 `broker_client_invitation_` 和 `child_connection_` 创建新的渲染服务实例的 Service Manager 连接。这包括设置服务身份（`service_manager::Identity`）和相关的连接逻辑。
5. **初始化 IPC 通道**：
   - 创建一个新的消息管道（`mojo::MessagePipe`），并使用它来初始化一个 `IPC::ChannelFactory`。
   - 根据编译条件和运行时设置，决定是创建一个标准的 `IPC::ChannelProxy` 还是一个同步通道（`IPC::SyncChannel`，通常用于 Android 的同步合成）。
6. **配置通道代理**：
   - 调用 `ResetChannelProxy` 清理旧的通道代理（如果有）。
   - 使用之前创建的通道工厂初始化新的通道代理，并立即创建管道。
7. **获取关联接口代理**：
   - 通过 `channel_->GetRemoteAssociatedInterface` 方法获取 `remote_route_provider_` 和 `renderer_interface_` 的代理。这些接口用于后续的通信。
8. **暂停通道**：
   - 初始化时，通道被设置为暂停状态。这样做是为了在进程启动和初期配置期间控制消息流。

调用完**RenderProcessHostImpl::InitializeChannelProxy**之后，就可以返回了，直到

```c++
CreateSpeculativeRenderFrameHost(
    SiteInstance* old_instance,
    SiteInstance* new_instance) {
  if (!new_instance->GetProcess()->Init())
      .......
}
```

​		**RenderProcessHostImpl**类的成员函数Init的实现如下所示：

```c++
bool RenderProcessHostImpl::Init() {
  if (HasConnection())
    return true;

  is_dead_ = false;

  base::CommandLine::StringType renderer_prefix;
  // A command prefix is something prepended to the command line of the spawned
  // process.
  const base::CommandLine& browser_command_line =
      *base::CommandLine::ForCurrentProcess();
  renderer_prefix =
      browser_command_line.GetSwitchValueNative(switches::kRendererCmdPrefix);

  int flags = ChildProcessHost::CHILD_NORMAL;

  // Find the renderer before creating the channel so if this fails early we
  // return without creating the channel.
  base::FilePath renderer_path = ChildProcessHost::GetChildPath(flags);
  if (renderer_path.empty())
    return false;

  sent_render_process_ready_ = false;

  // We may reach Init() during process death notification (e.g.
  // RenderProcessExited on some observer). In this case the Channel may be
  // null, so we re-initialize it here.
  if (!channel_)
    InitializeChannelProxy();

  // Unpause the Channel briefly. This will be paused again below if we launch a
  // real child process. Note that messages may be sent in the short window
  // between now and then (e.g. in response to RenderProcessWillLaunch) and we
  // depend on those messages being sent right away.
  //
  // |channel_| must always be non-null here: either it was initialized in
  // the constructor, or in the most recent call to ProcessDied().
  channel_->Unpause(false /* flush */);

  // Call the embedder first so that their IPC filters have priority.
  GetContentClient()->browser()->RenderProcessWillLaunch(this);

  // Intentionally delay the hang monitor creation after the first renderer
  // is created. On Mac audio thread is the UI thread, a hang monitor is not
  // necessary or recommended.
  media::AudioManager::StartHangMonitorIfNeeded(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));

  CreateMessageFilters();
  RegisterMojoInterfaces();

  if (run_renderer_in_process()) {
    // Crank up a thread and run the initialization there.  With the way that
    // messages flow between the browser and renderer, this thread is required
    // to prevent a deadlock in single-process mode.  Since the primordial
    // thread in the renderer process runs the WebKit code and can sometimes
    // make blocking calls to the UI thread (i.e. this thread), they need to run
    // on separate threads.
    in_process_renderer_.reset(
        g_renderer_main_thread_factory(InProcessChildThreadParams(
            BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
            broker_client_invitation_.get(),
            child_connection_->service_token())));

    base::Thread::Options options;
    // In-process plugins require this to be a UI message loop.
    options.message_loop_type = base::MessageLoop::TYPE_UI;
    // As for execution sequence, this callback should have no any dependency
    // on starting in-process-render-thread.
    // So put it here to trigger ChannelMojo initialization earlier to enable
    // in-process-render-thread using ChannelMojo there.
    OnProcessLaunched();  // Fake a callback that the process is ready.

    in_process_renderer_->StartWithOptions(options);

    g_in_process_thread = in_process_renderer_->message_loop();

    // Make sure any queued messages on the channel are flushed in the case
    // where we aren't launching a child process.
    channel_->Flush();
  } else {
    // Build command line for renderer.  We call AppendRendererCommandLine()
    // first so the process type argument will appear first.
    std::unique_ptr<base::CommandLine> cmd_line =
        base::MakeUnique<base::CommandLine>(renderer_path);
    if (!renderer_prefix.empty())
      cmd_line->PrependWrapper(renderer_prefix);
    AppendRendererCommandLine(cmd_line.get());

    // Spawn the child process asynchronously to avoid blocking the UI thread.
    // As long as there's no renderer prefix, we can use the zygote process
    // at this stage.
    child_process_launcher_.reset(new ChildProcessLauncher(
        base::MakeUnique<RendererSandboxedProcessLauncherDelegate>(),
        std::move(cmd_line), GetID(), this,
        std::move(broker_client_invitation_),
        base::Bind(&RenderProcessHostImpl::OnMojoError, id_)));
    channel_->Pause();

    fast_shutdown_started_ = false;
  }
 
  if (!gpu_observer_registered_) {
    gpu_observer_registered_ = true;
    ui::GpuSwitchingManager::GetInstance()->AddObserver(this);
  }

  is_initialized_ = true;
  init_time_ = base::TimeTicks::Now();
  return true;
}
```

​		这个函数定义在文件 content/browser/renderer_host/render_process_host_impl.cc 中。

​		RenderProcessHostImpl 类有一个类型为scoped_ptr< IPC::ChannelProxy > 成员变量channel_，当它引用了一个IPC::ChannelProxy 对象的时候，就表明已经为当前要加载的网而创建过 Render 进程了，因此在这种情况下，就无需要往前执行了。

​		 我们假设到目前为止，还没有为当前要加载的网页创建过 Render 进程。接下来RenderProcessHostImpl类的成员函数 Init就会做以下四件事情：

  1. 如果通道 (`channel_`) 尚未初始化，则调用 **`InitializeChannelProxy`** 来设置通信通道。
  2. 通道在初始化期间被暂停，现在临时解除暂停，以便可以立即发送一些初始化消息。
  3. 调用 **GetContentClient()->browser()->RenderProcessWillLaunch(this)** 通知浏览器客户端即将启动渲染进程。
  4. 调用RenderProcessHostImpl类的成员函数 CreateMessageFilters 创建一系列的 Message Filter，用来过滤 IPC 消息。
  5. **<u>如果所有网页都在 Browser 进程中加载，即不单独创建 Render 进程来加载网页，那么这时候调用父类 RenderProcessHost 的静态成员函数 run_ renderer_ in_ process 的返回值就等于 true。在这种情况下，就会通过在本进程（即Browser进程）创建一个新的线程来渲染网页。这个线程由 RenderProcessHostImpl 类的静态成员变量g_ renderer_ main _thread _ factory 描述的一个函数创建，它的类型为InProcessRendererThread。InProcessRendererThread类继承了base::Thread类，从前面 Chromium 多线程模型设计和实现分析一文可以知道，当调用它的成员函数 StartWithOptions 的时候，新的线程就会运行起来。这时候如果我们再调用它的成员函数  message _ loop ，就可以获得它的 Message Loop。有了这个 Message Loop 之后，以后就可以向它发送消息了。</u>**
  6. 如果网页要单独的 Render 进程中加载，那么调用创建一个命令行，并且以该命令行以及前面创建的 IPC::ChannelProxy 对象为参数，创建一个 ChildProcessLauncher 对象，而该 ChildProcessLauncher 对象在创建的过程，就会启动一个新的 Render进程。

   接下来，我们主要分析第 1、5 和 6 件事情，第 2、3 件事情在接下来的一篇文章中分析 IPC 消息分发机制时再分析。



第一件事情 **`InitializeChannelProxy`** 的实现如下：

```c++
void RenderProcessHostImpl::InitializeChannelProxy() {
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner =
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);

  service_manager::Connector* connector =
      BrowserContext::GetConnectorFor(browser_context_);
  if (!connector) {
    if (!ServiceManagerConnection::GetForProcess()) {
      ServiceManagerConnection::SetForProcess(ServiceManagerConnection::Create(
          mojo::MakeRequest(&test_service_), io_task_runner));
    }
    connector = ServiceManagerConnection::GetForProcess()->GetConnector();
  }

  broker_client_invitation_ =
      base::MakeUnique<mojo::edk::OutgoingBrokerClientInvitation>();
  service_manager::Identity child_identity(
      mojom::kRendererServiceName,
      BrowserContext::GetServiceUserIdFor(GetBrowserContext()),
      base::StringPrintf("%d_%d", id_, instance_id_++));
  child_connection_.reset(new ChildConnection(child_identity,
                                              broker_client_invitation_.get(),
                                              connector, io_task_runner));

  mojo::MessagePipe pipe;
  BindInterface(IPC::mojom::ChannelBootstrap::Name_, std::move(pipe.handle1));
  std::unique_ptr<IPC::ChannelFactory> channel_factory =
      IPC::ChannelMojo::CreateServerFactory(std::move(pipe.handle0),
                                            io_task_runner);

  ResetChannelProxy();

  if (!channel_)
    channel_.reset(new IPC::ChannelProxy(this, io_task_runner.get()));
  channel_->Init(std::move(channel_factory), true /* create_pipe_now */);

  channel_->GetRemoteAssociatedInterface(&remote_route_provider_);
  channel_->GetRemoteAssociatedInterface(&renderer_interface_);

  channel_->Pause();
}
```

**`RenderProcessHostImpl::InitializeChannelProxy`** 函数主要职责是创建 IPC 通道，这是浏览器进程和渲染进程之间通信的关键组件。以下是对这个函数关键部分的分析：

1. **获取 I/O 线程的任务运行器**：
   - `io_task_runner` 获取用于 I/O 操作的线程的任务运行器。这是因为大部分与渲染进程通信的操作都在 I/O 线程上执行。
2. **获取 Service Manager 连接器**：
   - 获取 `service_manager::Connector` 实例，用于与 Service Manager 建立连接。这个连接器用于路由到新的渲染服务实例。
3. **处理连接器的备用情况**：
   - 如果没有针对每个 `BrowserContext` 初始化连接器，代码会回退使用浏览器全局的连接器。在一些测试环境中，可能需要初始化一个虚拟的连接器。
4. **建立 Service Manager 连接**：
   - 使用 `broker_client_invitation_` 和 `child_connection_` 创建新的渲染服务实例的 Service Manager 连接。这包括设置服务身份（`service_manager::Identity`）和相关的连接逻辑。
5. **初始化 IPC 通道**：
   - 创建一个新的消息管道（`mojo::MessagePipe`），并使用它来初始化一个 `IPC::ChannelFactory`。
   - 根据编译条件和运行时设置，决定是创建一个标准的 `IPC::ChannelProxy` 还是一个同步通道（`IPC::SyncChannel`，通常用于 Android 的同步合成）。
6. **配置通道代理**：
   - 调用 `ResetChannelProxy` 清理旧的通道代理（如果有）。
   - 使用之前创建的通道工厂初始化新的通道代理，并立即创建管道。
7. **获取关联接口代理**：
   - 通过 `channel_->GetRemoteAssociatedInterface` 方法获取 `remote_route_provider_` 和 `renderer_interface_` 的代理。这些接口用于后续的通信。
8. **暂停通道**：
   - 初始化时，通道被设置为暂停状态。这样做是为了在进程启动和初期配置期间控制消息流。

注意 chenel 使用 service_manager::Identity 来创建一个new对象， **`service_manager::Identity`** 的实现如下：

```c++
...
      service_manager::Identity child_identity(
      mojom::kRendererServiceName,
      BrowserContext::GetServiceUserIdFor(GetBrowserContext()),
      base::StringPrintf("%d_%d", id_, instance_id_++));
  child_connection_.reset(new ChildConnection(child_identity,
                                              broker_client_invitation_.get(),
                                              connector, io_task_runner));
...


class Identity {
 public:
    .......
  Identity(const std::string& name,
           const std::string& user_id,
           const std::string& instance);
.......
 private:
  std::string name_;
  std::string user_id_;
  std::string instance_;
};
```

​		从这里就可以看出，这是一个类似名称的对象，用于对各个通道进行区分。里面使用通道名称、上下文id、一个全局id+实例id。

### ChildConnection 

​		回到R enderProcessHostImpl 类的成员函数**`InitializeChannelProxy`**中，有了用来创建 UNIX Socket 的名字之后，就可以生成一个 ChildConnection 了，如下所示：

```c++
ChildConnection::ChildConnection(
    const service_manager::Identity& child_identity,
    mojo::edk::OutgoingBrokerClientInvitation* invitation,
    service_manager::Connector* connector,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : context_(new IOThreadContext),
      child_identity_(child_identity),
      weak_factory_(this) {
  // TODO(rockot): Use a constant name for this pipe attachment rather than a
  // randomly generated token.
  service_token_ = mojo::edk::GenerateRandomToken();
  context_->Initialize(child_identity_, connector,
                       invitation->AttachMessagePipe(service_token_),
                       io_task_runner);
}

class ChildConnection::IOThreadContext
    : public base::RefCountedThreadSafe<IOThreadContext> {
 public:
  IOThreadContext() {}

  void Initialize(const service_manager::Identity& child_identity,
                  service_manager::Connector* connector,
                  mojo::ScopedMessagePipeHandle service_pipe,
                  scoped_refptr<base::SequencedTaskRunner> io_task_runner) {
    DCHECK(!io_task_runner_);
    io_task_runner_ = io_task_runner;
    std::unique_ptr<service_manager::Connector> io_thread_connector;
    if (connector)
      connector_ = connector->Clone();
    child_identity_ = child_identity;
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&IOThreadContext::InitializeOnIOThread, this,
                                  child_identity, base::Passed(&service_pipe)));
  }
};
```

1. **`ChildConnection::ChildConnection` 构造函数**：
   - **设置身份和邀请**：构造函数接收一个 `service_manager::Identity` 对象（`child_identity`），它代表了子进程的身份。还有一个 `mojo::edk::OutgoingBrokerClientInvitation`（`invitation`），这是一个 Mojo 的 IPC 机制，用于创建到子进程的通信通道。
   - **生成服务令牌**：使用  `mojo::edk::GenerateRandomToken`  生成一个随机的服务令牌（`service_token_`），这个令牌用于唯一标识连接的消息管道。
   - **初始化 IO 线程上下文**：调用 `context_->Initialize`，将消息管道、子进程身份、连接器和 I/O 线程的任务运行器传递给 `IOThreadContext` 对象进行初始化。
2. **`ChildConnection::IOThreadContext::Initialize` 方法**：
   - **线程和连接器的设置**：这个方法首先确认 I/O 线程的任务运行器还未初始化，然后将其设置为传入的 `io_task_runner`。如果存在一个有效的 `service_manager::Connector`，则创建其副本用于 I/O 线程。
   - **子进程身份设置**：将子进程的身份信息设置到 `IOThreadContext` 对象中。
   - **在 I/O 线程上初始化**：通过 `io_task_runner_->PostTask`，将初始化操作（`InitializeOnIOThread`）调度到 I/O 线程上执行。这包括使用传递的 `service_pipe` 来建立与子进程的实际通信。

总结而言，`ChildConnection` 和它的 `IOThreadContext` 提供了一种机制来管理和维护与子进程（如渲染进程）的连接。



​		之后创建并配置 Mojo 消息管道，用于建立和管理与渲染进程的 IPC 通信，`BindInterface(IPC::mojom::ChannelBootstrap::Name_, std::move(pipe.handle1));` 这行代码将消息管道的一个端点（`pipe.handle1`）绑定到 `IPC::mojom::ChannelBootstrap` 接口。`ChannelBootstrap` 接口是一个 Mojo 接口，用于初始化 IPC 通道。

`std::unique_ptr<IPC::ChannelFactory> channel_factory = IPC::ChannelMojo::CreateServerFactory(std::move(pipe.handle0), io_task_runner);` 这行代码使用消息管道的另一个端点（`pipe.handle0`）来创建一个 IPC 通道工厂。`CreateServerFactory` 创建一个用于服务器端的工厂，它将在 I/O 线程上运行。这个工厂负责生成 IPC 通道，该通道将用于浏览器进程与新创建的渲染进程之间的通信。

### ChannelProxy

之后把以前的通道处置后，创建新的 **ChannelProxy**：

```c++
ChannelProxy::ChannelProxy(
    Listener* listener,
    const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner)
    : context_(new Context(listener, ipc_task_runner)), did_init_(false) {
#if defined(ENABLE_IPC_FUZZER)
  outgoing_message_filter_ = NULL;
#endif
}
```

ChannelProxy 类的构造函数主要是创建一个**ChannelProxy::Context**对象，并且将该ChannelProxy::Context对象保存在成员变量context_中。

ChannelProxy::Context 对象的创建过程如下所示：

```c++
ChannelProxy::Context::Context(
    Listener* listener,
    const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner)
    : listener_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      listener_(listener),
      ipc_task_runner_(ipc_task_runner),
      channel_connected_called_(false),
      message_filter_router_(new MessageFilterRouter()),
      peer_pid_(base::kNullProcessId) {
  ......
}
```

这个函数定义在文件external/chromium_org/ipc/ipc_channel_proxy.cc中。

   ChannelProxy::Context类有三个成员变量是需要特别关注的，它们分别是：

   1. **listenter_task_runner_**。这个成员变量的类型为scoped_refptr< base::SingleThreadTaskRunner >，它指向的是一个 SingleThreadTaskRunner 对象。这个 SingleThreadTaskRunner 对象通过调用 ThreadTaskRunnerHandle 类的静态成员函数 Get 获得。从前面 Chromium 多线程模型设计和实现分析一文可以知道，ThreadTaskRunnerHandle 类的静态成员<u>函数 Get 返回的 SingleThreadTaskRunner 对象实际上是当前线程的一个 MessageLoopProxy 对象，通过该 MessageLoopProxy 对象可以向当前线程的消息队列发送消息</u>。当前线程即为 Browser 进程的主线程。

   2. **listener**_ 。这是一个 IPC::Listener 指针，它的值设置为参数 listener 的值。从前面的图可以知道，RenderProcessHostImpl 类实现了 IPC::Listener 接口，而且从前面的调用过程过程可以知道，**<u>参数 listener 指向的就是一个 RenderProcessHostImpl 对象。以后正在创建的 ChannelProxy::Context 对象在 IO线 程中接收到 Render 进程发送过来的 IPC 消息之后，就会转发给成员变量 listener_ 指向的 RenderProcessHostImpl 对象处理，但是并不是让后者直接在 IO线程处理，而是让后者在成员变量 listener_task_runner_ 描述的线程中处理，即 Browser 进程的主线程处理。</u>**也就是说，ChannelProxy::Context 类的成员变量 listener_task_runner_ 和 listener_ 是配合在一起使用的，后面我们分析 IPC 消息的分发机制时就可以看到这一点。

   3. **ipc_task_runner_**。这个成员变量与前面分析的成员变量 listener_task_runner 一样，类型都为 scoped_refptr< base::SingleThreadTaskRunner >，指向的者是一个 SingleThreadTaskRunner 对象。不过，这个 SingleThreadTaskRunner 对象由参数 ipc_task_runner 指定。从前面的调用过程可以知道，这个SingleThreadTaskRunner 对象实际上是与 Browser 进程的 IO 线程关联的一个 MessageLoopProxy 对象。这个 MessageLoopProxy 对象用来接收 Render进程发送过来的 IPC 消息。也就是说，Browser 进程在 IO 线程中接收 IPC 消息。

​        ChannelProxy::Context 类还有一个重要的成员变量 message_filter_router_，它指向一个 MessageFilterRouter 对象，用来过滤 IPC 消息，后面我们分析 IPC 消息的分发机制时再详细分析。



 回到 **`InitializeChannelProxy`** 中，创建了一个 ChannelProxy 对象之后，接下来就调用它的成员函数 **`Init`** 进行初始化，如下所示：

```c++
void ChannelProxy::Init(std::unique_ptr<ChannelFactory> factory,
                        bool create_pipe_now) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!did_init_);

  if (create_pipe_now) {
    // Create the channel immediately.  This effectively sets up the
    // low-level pipe so that the client can connect.  Without creating
    // the pipe immediately, it is possible for a listener to attempt
    // to connect and get an error since the pipe doesn't exist yet.
    context_->CreateChannel(std::move(factory));
  } else {
    context_->ipc_task_runner()->PostTask(
        FROM_HERE, base::Bind(&Context::CreateChannel, context_,
                              base::Passed(&factory)));
  }

  // complete initialization on the background thread
  context_->ipc_task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&Context::OnChannelOpened, context_));

  did_init_ = true;
  OnChannelInit();
}
```

​		这个函数定义在文件 ipc/ipc_channel_proxy.cc 中。

​		从前面的调用过程知道，参数 channel_factory 描述的是一个服务端 ChannelFactory，参数 create_pipe_now 的值为 true。这样，ChannelProxy 类的成员函数 Init 就会马上调用前面创建的 ChannelProxy::Context 对象的成员函数 CreateChannel 创建一个 IPC 通信通道，也就是在当前线程中创建一个 IPC 通信通道 。

​		另一个方面，如果参数 create_pipe_now 的值等于 false，那么 ChannelProxy 类的成员函数 Init 就不是在当前线程创建 IPC 通信通道，而是在 IO 线程中创建。因为它先通过前面创建的 ChannelProxy::Context 对象的成员函数 ipc_task_runner 获得其成员变量 ipc_task_runner_ 描述的 SingleThreadTaskRunner 对象，然后再将创建 IPC 通信通道的任务发送到该 SingleThreadTaskRunner 对象描述的 IO 线程的消息队列去。当该任务被处理时，就会调用ChannelProxy::Context 类的成员函数 CreateChannel。

​		当调用 ChannelProxy::Context 类的成员函数 CreateChannel 创建好一个 IPC 通信通道之后，ChannelProxy 类的成员函数 Init 还会向当前进程的 IO 线程的消息队列发送一个消息，该消息绑定的是 ChannelProxy::Context 类的成员函数 OnChannelOpened。因此，接下来我们就分别分析 ChannelProxy::Context 类的成员函数 CreateChannel 和 OnChannelOpened。

​		ChannelProxy::Context类的成员函数CreateChannel的实现如下所示：

```c++
void ChannelProxy::Context::CreateChannel(
    std::unique_ptr<ChannelFactory> factory) {
  base::AutoLock l(channel_lifetime_lock_);
  DCHECK(!channel_);
  DCHECK_EQ(factory->GetIPCTaskRunner(), ipc_task_runner_);
  channel_ = factory->BuildChannel(this)；//___________________________________________.
																					  |
  Channel::AssociatedInterfaceSupport* support =									  |
      channel_->GetAssociatedInterfaceSupport();									  |
  if (support) {						     	    	 						      |
    thread_safe_channel_ = support->CreateThreadSafeChannel();						  |
																	     	    	  |
    base::AutoLock l(pending_filters_lock_);		     							  |
    for (auto& entry : pending_io_thread_interfaces_)								  |
      support->AddGenericAssociatedInterface(entry.first, entry.second);			  |
    pending_io_thread_interfaces_.clear();	    	     							  |
  }																	     	    	  |
}																	     	    	  |
//____________________________________________________________________________________|
//↓
std::unique_ptr<Channel> MojoChannelFactory::BuildChannel(Listener* listener) override {
  return ChannelMojo::Create(
      std::move(handle_), mode_, listener, ipc_task_runner_);//_______________________
}																					  |
//____________________________________________________________________________________|
//↓
// static
std::unique_ptr<ChannelMojo> ChannelMojo::Create(
    mojo::ScopedMessagePipeHandle handle,
    Mode mode,
    Listener* listener,
    const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner) {
  return base::WrapUnique(
      new ChannelMojo(std::move(handle), mode, listener, ipc_task_runner));//_________
}																					  |
//____________________________________________________________________________________|
//↓
ChannelMojo::ChannelMojo(
    mojo::ScopedMessagePipeHandle handle,
    Mode mode,
    Listener* listener,
    const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner)
    : task_runner_(ipc_task_runner),
      pipe_(handle.get()),
      listener_(listener),
      weak_factory_(this) {
  bootstrap_ = MojoBootstrap::Create(std::move(handle), mode, ipc_task_runner);
}
```

​		从前面的调用过程知道，参数 channel_factory 描述的是一个服务端 ChannelFactory，进入函数后直接调用 `factory->BuildChannel(this)`，在最后直接创建了一个**`ChannelMojo`**，**`ChannelMojo`** 通过 **Mojo** 消息管道实现了 `IPC::Channel` 接口。下面我们分析 `ChannelMojo` 的作用和其成员的角色：

1. **作用与继承关系**:
   - `ChannelMojo` 继承自 `Channel` 和 `Channel::AssociatedInterfaceSupport`，并实现了 `internal::MessagePipeReader::Delegate` 接口。这表明它既是一个通信通道，也支持关联接口，并能处理消息管道的读取事件。
2. **静态工厂方法**:
   - `ChannelMojo` 提供了静态方法来创建 `ChannelMojo` 实例。这些方法接收一个 `mojo::ScopedMessagePipeHandle`（Mojo 消息管道句柄）和其他配置参数，并返回 `ChannelMojo` 的智能指针。
3. **通道实现（Channel Implementation）**:
   - 方法如 `Connect`, `Pause`, `Unpause`, `Flush`, `Close`, `Send` 实现了通道的基本操作，如**连接、暂停、恢复、刷新、关闭和发送消息。**
   - `GetAssociatedInterfaceSupport` 方法提供了对关联接口支持的访问，这是用于处理通道上的高级接口交互。
4. **消息处理与错误处理**:
   - 实现了 `MessagePipeReader::Delegate` 接口的方法，例如 `OnPeerPidReceived`, `OnMessageReceived`, `OnPipeError` 和 `OnAssociatedInterfaceRequest`，用于处理接收到的消息和管道错误，以及关联接口请求。
5. **关联接口支持**:
   - `ChannelMojo` 可以通过 `AddGenericAssociatedInterface` 和 `GetGenericRemoteAssociatedInterface` 管理关联接口。这允许通道处理特定接口的请求和响应。
6. **线程与消息处理**:
   - `task_runner_` 成员变量是一个指向单线程任务运行器的智能指针，用于在 ChannelMojo 所属的线程上运行任务。
   - `pipe_` 是 Mojo 消息管道的句柄，用于实际的消息传输。
   - `bootstrap_` 和 `message_reader_` 用于消息的初始化和读取。
7. **监听器与关联接口映射**:
   - `listener_` 指向一个监听器，用于接收通道事件。
   - `associated_interfaces_` 是一个映射，存储了关联接口和它们的工厂函数，用于动态创建接口实例。
8. **资源管理与线程安全**:
   - `weak_factory_` 用于创建 `ChannelMojo` 的弱指针，防止在异步操作中的潜在资源管理问题。
   - `associated_interface_lock_` 和 `DISALLOW_COPY_AND_ASSIGN` 保证了类实例的线程安全和不可复制性。

总的来说，`ChannelMojo` 是 Chromium 中用于处理进程间通信的关键组件，它通过 Mojo 消息管道实现了 IPC::Channel 接口，并提供了对关联接口的支持。通过它的方法和成员变量，`ChannelMojo` 管理了消息的发送、接收和处理，确保了通信的高效和安全。



















