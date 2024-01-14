[TOC]



# Chromium 网页 Frame Tree 创建过程分析

​		Chromium 在加载一个网页之前，需要在 Browser 进程创建一个 Frame Tree。Browser 进程为网页创建了 Frame Tree 之后，再请求 Render 进程加载其内容。Frame Tree 将网页抽象为 Render Frame。Render Frame 是为实现 Out-of-Process iframes 设计的。本文接下来就分析 Frame Tree 的创建过程，为后面分析网页加载过程打基础。

​		关于 Chromium 的 Out-of-Process iframes 的详细介绍，可以参考前面Chromium网页加载过程简要介绍和学习计划一文。我们通过图1所示的例子简单介绍，如下所示：

![img](markdownimage/20160104002245789)

​		在图 1 中，网页 A 在一个 Tab 显示，并且它通过 iframe 标签包含了网页 B 和网页 C。网页 C 通过window.open 在另外一个 Tab 中打开了另外一个网页 C 实例。新打开的网页 C 通过 iframe 标签包含了网页D，网页 D 又通过 iframe 标签包含了网页 A 的另外一个实例。

这时候 Browser 进程会分别为图 1 的两个 Tab 创建一个 WebContents 对象，如图 2 所示：

![img](markdownimage/20160104002508894)

​		<u>在 Browser 进程中，一个网页使用一个 WebContents 对象描述</u>。**<u>每一个 WebContents 对象都又关联有一个 Frame Tree。Frame Tree 的每一个 Node 代表一个网页。</u>**其中，Frame Tree 的**根 Node** 描述的是**主网页**，**子 Node** 描述的是**通过 iframe 标签嵌入的子网页**。

​		例如，第一个 Tab 的 Frame Tree 包含有三个 Node，分别代表网页 A、B 和 C，第二个 Tab 的 Frame Tree 也包含有三个 Node，分别代表网页 C、D 和 A。

​		网页 A、B、C 和 D 在 Chromium 中形成了一个 Browsing Instance。关于 Chromium 的 Browsing Instance，可以参考官方文档：[Chromium Docs - Process Model and Site Isolation (googlesource.com)](https://chromium.googlesource.com/chromium/src/+/main/docs/process_model_and_site_isolation.md)，它对应于 HTML5 规范中的 Browsing Context。

​		我们观察代表网页 B 的子 Node，它关联有一个 RenderFrameHost 对象和三个RenderFrameProxyHost 对象。其中，RenderFrameHost 对象描述的是网页 B 本身，另外三个RenderFrameProxyHosts 对象描述的是网页 A、C 和 D。表示网页 B 可以通过 Javascript 接口window.postMessage 向网页 A、C 和 D 发送消息。

​		假设图 1 所示的网页 A、B、C 和 D 分别在不同的 Render 进程中渲染，如图3所示：

![img](markdownimage/20160104004308368)

​		在负责加载和渲染网页 B 的 Render 进程中，有一个 RenderFrame 对象，代表图 1 所示的网页 B。负责加载和渲染网页 B 的 Render 进程还包含有五个 RenderFrameProxy 对象，分别代表图 1 所示的两个网页 A 实例、两个网页 C 实例和一个网页 D 实例。在负责渲染网页 A、C 和 D 的 Render 进程中，也有类似的RenderFrame 对象和 RenderFrameProxy 对象。

​		Browser 进程的 RenderFrameHost 对象和 Render 进程的 RenderFrame 对象是一一对应的。同样，Browser 进程的 RenderFrameHostProxy 对象和 Render 进程的 RenderFrameProxy 对象也是一一对应的。RenderFrame 对象和 RenderFrameProxy 对象的存在，使得在同一个 Browsing Instance 或者Browsing Context 中的网页可以通过J avascript 接口 window.postMessage 相互发送消息。

​		**<u>例如，当网页 B 要向网页 A 发送消息时，负责加载和渲染网页 B 的 Render 进程就会在当前进程中找到与网页 B 对应的 RenderFrame 对象，以及与网页 A 对应的 RenderFrameProxy 对象，然后将要发送的消息从找到的 RenderFrame 对象传递给找到的 RenderFrameProxy 对象即可。与网页 A 对应 RenderFrameProxy 对象接收到消息之后，会进一步将该消息发送给负责加载和渲染网页 A 的 Render 进程处理，从而完成消息的发送和处理过程。</u>**



## 源码分析

​		接下来，我们就结合源码分析 Chromium 为网页创建 Frame Tree 的过程，这个过程涉及到WebContents、RenderFrameHost 和 RenderFrame 等重要对象的创建。

​		前面提到，Frame Tree 是在 Browser 进程中创建的，并且与一个 WebContents 对象关联。从前面Chromium 网页加载过程简要介绍和学习计划一文可以知道，**<u>WebContents 是 Chromium 的 Content 模块提供的一个高级接口，通过这个接口可以将一个网页渲染在一个可视区域中。我们可以认为，一个 WebContents 对象就是用来实现一个网页的加载、解析、渲染和导航等功能的。</u>**

​		Frame Tree 是在与其关联的 WebContents 对象的创建过程中创建的，因此接下来我们就从WebContents 对象的创建过程开始分析 Frame Tree 的创建过程。以点击浏览器 Tab 为例，会在 Native 层创建一个 WebContents， Natigave 实现如下：

```c++
void Navigate(NavigateParams* params) {
  ......
  if (!params->target_contents && singleton_index < 0) {
    DCHECK(!params->url.is_empty());
    if (params->disposition != WindowOpenDisposition::CURRENT_TAB) {
      params->target_contents = CreateTargetContents(*params, params->url);

      // This function takes ownership of |params->target_contents| until it
      // is added to a TabStripModel.
      target_contents_owner.TakeOwnership();
    } else {
      DCHECK(params->source_contents);
        
      params->target_contents = params->source_contents;

      // Prerender can only swap in CURRENT_TAB navigations; others have
      // different sessionStorage namespaces.
      swapped_in_prerender = SwapInPrerender(params->url, params);
    }

    if (user_initiated)
      params->target_contents->UserGestureDone();

    if (!swapped_in_prerender) {
      // Try to handle non-navigational URLs that popup dialogs and such, these
      // should not actually navigate.
      if (!HandleNonNavigationAboutURL(params->url)) {
        // Perform the actual navigation, tracking whether it came from the
        // renderer.

        LoadURLInContents(params->target_contents, params->url, params);
      }
    }
  } else {
    // |target_contents| was specified non-NULL, and so we assume it has already
    // been navigated appropriately. We need to do nothing more other than
    // add it to the appropriate tabstrip.
  }

  ......
}
```

​		可以看出，当在 Navigate 时，如果没有目标 contents 时会调用  **`CreateTargetContents`**(*params, params->url) 创建一个目标 contents，

### WebContents 

​		当点击浏览器的 Tab 添加按钮时，会新建一个 WebContents ，它的实现如下：

```c++
WebContents* WebContents::Create(const WebContents::CreateParams& params) {
  return WebContentsImpl::CreateWithOpener(params, FindOpener(params));
}
```

​		WebContents 类的静态成员函数 Create 调用另外一个静态成员函数 CreateWithOpener 创建一个WebContentsImpl 对象，并且返回给调用者。

​		WebContents 类的静态成员函数 CreateWithOpener 的实现如下所示：

```c++
WebContentsImpl* WebContentsImpl::CreateWithOpener(
    const WebContents::CreateParams& params,
    FrameTreeNode* opener) {
  TRACE_EVENT0("browser", "WebContentsImpl::CreateWithOpener");
  WebContentsImpl* new_contents = new WebContentsImpl(params.browser_context);
  new_contents->SetOpenerForNewContents(opener, params.opener_suppressed);

  // If the opener is sandboxed, a new popup must inherit the opener's sandbox
  // flags, and these flags take effect immediately.  An exception is if the
  // opener's sandbox flags lack the PropagatesToAuxiliaryBrowsingContexts
  // bit (which is controlled by the "allow-popups-to-escape-sandbox" token).
  // See https://html.spec.whatwg.org/#attr-iframe-sandbox.
  FrameTreeNode* new_root = new_contents->GetFrameTree()->root();
  if (opener) {
    blink::WebSandboxFlags opener_flags = opener->effective_sandbox_flags();
    const blink::WebSandboxFlags inherit_flag =
        blink::WebSandboxFlags::kPropagatesToAuxiliaryBrowsingContexts;
    if ((opener_flags & inherit_flag) == inherit_flag) {
      new_root->SetPendingSandboxFlags(opener_flags);
      new_root->CommitPendingFramePolicy();
    }
  }

  // This may be true even when opener is null, such as when opening blocked
  // popups.
  if (params.created_with_opener)
    new_contents->created_with_opener_ = true;

  if (params.guest_delegate) {
    // This makes |new_contents| act as a guest.
    // For more info, see comment above class BrowserPluginGuest.
    BrowserPluginGuest::Create(new_contents, params.guest_delegate);
    // We are instantiating a WebContents for browser plugin. Set its subframe
    // bit to true.
    new_contents->is_subframe_ = true;
  }
  new_contents->Init(params);
  return new_contents;
}
```

​		WebContents 类的静态成员函数 CreateWithOpener 首先是创建一个 WebContentsImpl 对象。在将这个 WebContentsImpl 对象返回给调用者之前，会先调用它的成员函数 Init 对它进行初始化。

#### WebContentsImpl

​		接下来，我们先分析 WebContentsImpl 对象的创建过程，即WebContentsImpl类的构造函数的实现，接下来再分析 WebContentsImpl 对象的初始化过程，即 WebContentsImpl 类的成员函数 Init 的实现。

​		WebContentsImpl 类的构造函数的实现如下所示：

```c++
WebContentsImpl::WebContentsImpl(BrowserContext* browser_context)
    : ......
      controller_(this, browser_context),
      ......
      frame_tree_(new NavigatorImpl(&controller_, this),
                  this,
                  this,
                  this,
                  this),
      ...... {
  ......
}
```

​		WebContentsImpl 类的成员变量 controller_ 描述的是一个 NavigationControllerImpl 对象。后面我们可以看到，这个 NavigationControllerImpl 对象负责执行加载 URL 的操作。现在我们分析它的创建过程，如下所示：

```c++
NavigationControllerImpl::NavigationControllerImpl(
    NavigationControllerDelegate* delegate,
    BrowserContext* browser_context)
    : ......,
      delegate_(delegate),
      ...... {
  ......
}
```

​		从前面的调用过程可以知道，参数 delegate 指向的是一个 WebContentsImpl 对象。这个WebContentsImpl 对象保存在 NavigationControllerImpl 类的成员变量 delegate_ 中。

#### NavigatorImpl 

​		回到 WebContentsImpl 类的构造函数中，它构造了一个 NavigationControllerImpl 对象之后，接下来又根据该 NavigationControllerImpl 对象创建一个 NavigatorImpl 对象。这个 NavigatorImpl 对象也是在加载 URL 时使用到的，它的创建过程如下所示：

```c++
NavigatorImpl::NavigatorImpl(NavigationControllerImpl* navigation_controller,
                             NavigatorDelegate* delegate)
    : controller_(navigation_controller), delegate_(delegate) {}
```

​		从前面的调用过程可以知道，参数 navigation_controller 和 delegate 指向的分别是一个NavigationControllerImpl 对象和一个 WebContentsImpl 对象，它们分别保存在 NavigatorImpl 类的成员变量 controller_ 和 delegate_ 中。

​		再回到 WebContentsImpl 类的构造函数中，它创建了一个 NavigatorImpl 对象之后，又根据该NavigatorImpl 对象创建一个 FrameTree 对象，并且保存在成员变量 frame_tree_ 中。这个 FrameTree 对象描述的就是一个 Frame Tree。

### FrameTree

​		接下来我们继续分析 FrameTree 对象的创建过程，也就是 FrameTree 类的构造函数的实现，如下所示：

```c++
FrameTree::FrameTree(Navigator* navigator,
                     RenderFrameHostDelegate* render_frame_delegate,
                     RenderViewHostDelegate* render_view_delegate,
                     RenderWidgetHostDelegate* render_widget_delegate,
                     RenderFrameHostManager::Delegate* manager_delegate)
    : render_frame_delegate_(render_frame_delegate),
      render_view_delegate_(render_view_delegate),
      render_widget_delegate_(render_widget_delegate),
      manager_delegate_(manager_delegate),
      root_(new FrameTreeNode(this,
                              navigator,
                              render_frame_delegate,
                              render_widget_delegate,
                              manager_delegate,
                              nullptr,
                              // The top-level frame must always be in a
                              // document scope.
                              blink::WebTreeScopeType::kDocument,
                              std::string(),
                              std::string(),
                              base::UnguessableToken::Create(),
                              FrameOwnerProperties())),
      focused_frame_tree_node_id_(FrameTreeNode::kFrameTreeNodeInvalidId),
      load_progress_(0.0) {}
```

​		从前面的调用过程可以知道，参数 navigator 指向的是一个 NavigatorImpl 对象，其余四个参数指向的是同一个 WebContentsImpl 对象，分别被保存在 FrameTree 类的成员变量 render_frame_delegate_ 、 render_view_delegate_ 、render_widget_delegate_ 和 manager_delegate_ 中。

​		FrameTree 类的构造函数接下来做的一件事情是创建一个 FrameTreeNode 对象，并且保存在成员变量root_ 中，作为当前正在创建的 Frame Tree 的**根节点**。这个根节点描述的就是后面要加载的网页。

​		FrameTreeNode 对象的创建过程，即 FrameTreeNode 类的构造函数的实现，如下所示：

```c++
FrameTreeNode::FrameTreeNode(FrameTree* frame_tree,
                             Navigator* navigator,
                             RenderFrameHostDelegate* render_frame_delegate,
                             RenderWidgetHostDelegate* render_widget_delegate,
                             RenderFrameHostManager::Delegate* manager_delegate,
                             FrameTreeNode* parent,
                             blink::WebTreeScopeType scope,
                             const std::string& name,
                             const std::string& unique_name,
                             const base::UnguessableToken& devtools_frame_token,
                             const FrameOwnerProperties& frame_owner_properties)
    : frame_tree_(frame_tree),
      navigator_(navigator),
      render_manager_(this,
                      render_frame_delegate,
                      render_widget_delegate,
                      manager_delegate),
      ....... {
  std::pair<FrameTreeNodeIdMap::iterator, bool> result =
      g_frame_tree_node_id_map.Get().insert(
          std::make_pair(frame_tree_node_id_, this));
  CHECK(result.second);

  RecordUniqueNameSize(this);

  // Note: this should always be done last in the constructor.
  blame_context_.Initialize();
}
```

​		从前面的调用过程可以知道，参数 frame_tree 和 navigator 指向的分别是一个 FrameTree 对象和一个NavigatorImpl 对象，它们分别被保存在 FrameTreeNode 类的成员变量 frame_tree_ 和 navigator_ 中。

​		FrameTreeNode 类的构造函数接下来做的一件事情是构造一个 RenderFrameHostManager 对象，并且保存在成员变量 render_manager_ 中。这个 RenderFrameHostManager 对象负责管理与当前正在创建的FrameTreeNode 对象关联的一个 RenderFrameHost 对象，它的构造过程如下所示：

```c++
RenderFrameHostManager::RenderFrameHostManager(
    FrameTreeNode* frame_tree_node,
    RenderFrameHostDelegate* render_frame_delegate,
    RenderWidgetHostDelegate* render_widget_delegate,
    Delegate* delegate)
    : frame_tree_node_(frame_tree_node),
      delegate_(delegate),
      render_frame_delegate_(render_frame_delegate),
      render_widget_delegate_(render_widget_delegate),
      weak_factory_(this) {
  DCHECK(frame_tree_node_);
}
```

​		从前面的调用过程可以知道，参数 frame_tree_node 指向的是一个 FrameTreeNode 对象，它被保存在RenderFrameHostManager 类的成员变量 frame_tree_node_ 中，其余四个参数指向的是同一个WebContentsImpl 对象，分别被保存在 RenderFrameHostManager 类的成员变量 delegate_ 、render_frame_delegate_ 、render_view_delegate_ 和 render_widget_delegate_ 中。

​		这一步执行完成之后，一个 Frame Tree 就创建出来了。这是一个只有根节点的 Frame Tree。根节点描述的网页就是接下来要进行加载的。根节点描述的网页加载完成之后，就会进行解析。在解析的过程中，<u>如果碰到 iframe 标签，那么就会创建另外一个子节点，并且添加到当前正在创建的 Frame Tree 中去。</u>







​		返回到 WebContents 类的静态成员函数 CreateWithOpener 中，它创建了一个 WebContentsImpl 对象之后，接下来会调用这个 WebContentsImpl 对象的成员函数 Init 执行初始化工作，如下所示：

```c++
void WebContentsImpl::Init(const WebContents::CreateParams& params) {
  ......

  GetRenderManager()->Init(
      site_instance.get(), view_routing_id, params.main_frame_routing_id,
      main_frame_widget_routing_id, params.renderer_initiated_creation);

  ......

  if (GuestMode::IsCrossProcessFrameGuest(this)) {
    view_.reset(new WebContentsViewChildFrame(
        this, delegate, &render_view_host_delegate_view_));
  } else {
    view_.reset(CreateWebContentsView(this, delegate,
                                      &render_view_host_delegate_view_));
  }

  if (browser_plugin_guest_ && !GuestMode::IsCrossProcessFrameGuest(this)) {
    view_.reset(new WebContentsViewGuest(this, browser_plugin_guest_.get(),
                                         std::move(view_),
                                         &render_view_host_delegate_view_));
  }
    
  ......
}
```

​		WebContentsImpl 类的成员函数 Init 首先是调用成员函数 GetRenderManager 获得一个RenderFrameHostManager 对象，接下来再调用这个 RenderFrameHostManager 对象的成员函数 Init 对其进行初始化。

​		WebContentsImpl 类的成员函数 GetRenderManager 的实现如下所示：

```c++
RenderFrameHostManager* WebContentsImpl::GetRenderManager() const {
  return frame_tree_.root()->render_manager();
}
```

​		从这里可以看到，FrameTreeNode类的成员函数render_manager返回的是成员变量render_manager_描述的一个RenderFrameHostManager对象。这个RenderFrameHostManager对象是在前面分析的FrameTreeNode类的构造函数创建的。

​		回到 WebContentsImpl 类的成员函数 Init 中，它获得了一个 RenderFrameHostManager 对象之后，就调用它的成员函数 Init 对它进行初始化，如下所示：

```c++
void RenderFrameHostManager::Init(SiteInstance* site_instance,
                                  int32_t view_routing_id,
                                  int32_t frame_routing_id,
                                  int32_t widget_routing_id,
                                  bool renderer_initiated_creation) {
  ......
      
  SetRenderFrameHost(CreateRenderFrameHost(site_instance, view_routing_id,
                                           frame_routing_id, widget_routing_id,
                                           delegate_->IsHidden(),
                                           renderer_initiated_creation));

  ......
}
```

​		RenderFrameHostManager 类的成员函数 Init 主要做了两件事情。第一件事情是调用成员函数CreateRenderFrameHost 创建了一个 **`RenderFrameHostImpl`** 对象。第二件事情是调用成员函数**`SetRenderFrameHost`** 将该 RenderFrameHostImpl 对象保存在内部，如下所示：

```c++
scoped_ptr<RenderFrameHostImpl> RenderFrameHostManager::SetRenderFrameHost(
    scoped_ptr<RenderFrameHostImpl> render_frame_host) {
  // Swap the two.
  scoped_ptr<RenderFrameHostImpl> old_render_frame_host =
      render_frame_host_.Pass();
  render_frame_host_ = render_frame_host.Pass();
 
  ......
 
  return old_render_frame_host.Pass();
}
```

​		RenderFrameHostManager 类的成员函数 SetRenderFrameHost 主要是将参数 render_frame_host描述的一个 RenderFrameHostImpl 对象保存在成员变量render_frame_host_ 中。

​		回到 RenderFrameHostManager 类的成员函数 Init 中，接下来我们主要分析它调用成员函数CreateRenderFrameHost 创建 RenderFrameHostImpl 对象的过程，如下所示：

```c++
std::unique_ptr<RenderFrameHostImpl>
RenderFrameHostManager::CreateRenderFrameHost(
    SiteInstance* site_instance,
    int32_t view_routing_id,
    int32_t frame_routing_id,
    int32_t widget_routing_id,
    bool hidden,
    bool renderer_initiated_creation) {
 ......

  // Create a RVH for main frames, or find the existing one for subframes.
  FrameTree* frame_tree = frame_tree_node_->frame_tree();
  RenderViewHostImpl* render_view_host = nullptr;
  if (frame_tree_node_->IsMainFrame()) {
    render_view_host = frame_tree->CreateRenderViewHost(
        site_instance, view_routing_id, frame_routing_id, false, hidden);
    if (view_routing_id == MSG_ROUTING_NONE) {
      widget_routing_id = render_view_host->GetRoutingID();
    } else {
      DCHECK_EQ(view_routing_id, render_view_host->GetRoutingID());
    }
  } else {
    render_view_host = frame_tree->GetRenderViewHost(site_instance);
    CHECK(render_view_host);
  }

  return RenderFrameHostFactory::Create(
      site_instance, render_view_host, render_frame_delegate_,
      render_widget_delegate_, frame_tree, frame_tree_node_, frame_routing_id,
      widget_routing_id, hidden, renderer_initiated_creation);
}
```

​		从前面的分析可以知道，当前正在处理的 RenderFrameHostManager 对象的成员变量frame_tree_node_ 描述的是一个 Frame Tree 的根节点。一个 Frame Tree 的根节点描述的网页是一个Main Frame（网页中的 iframe 标签描述的网页称为 Sub Frame ），因此 RenderFrameHostManager 类的成员函数 CreateRenderFrameHost 是调用 FrameTree 类的成员函数 CreateRenderViewHost 创建一个 RenderViewHostImpl 对象，并且保存在本地变量 render_view_host 中。

​		FrameTree 类的成员函数 CreateRenderViewHost 的实现如下所示：

```c++
RenderViewHostImpl* FrameTree::CreateRenderViewHost(
    SiteInstance* site_instance,
    int32_t routing_id,
    int32_t main_frame_routing_id,
    bool swapped_out,
    bool hidden) {
  RenderViewHostMap::iterator iter =
      render_view_host_map_.find(site_instance->GetId());
  if (iter != render_view_host_map_.end())
    return iter->second;

  RenderViewHostImpl* rvh =
      static_cast<RenderViewHostImpl*>(RenderViewHostFactory::Create(
          site_instance, render_view_delegate_, render_widget_delegate_,
          routing_id, main_frame_routing_id, swapped_out, hidden));

  render_view_host_map_[site_instance->GetId()] = rvh;
  return rvh;
}
```

​		FrameTree 类的成员函数 CreateRenderViewHost  调用 RenderViewHostFactory 类的静态成员函数Create 创建了一个 RenderViewHostImpl 对象。在将这个 RenderViewHostImpl 对象返回给调用者之前，会以参数 site_instance 描述的一个 Site Instance 的 ID 为键值，将该 RenderViewHostImpl 对象保存在成员变量 render_view_host_map_ 描述的一个 Hash Map 中，以便以后可以进行查找。

​		RenderViewHostFactory 类的静态成员函数 Create 的实现如下所示：

```c++
// static
RenderViewHost* RenderViewHostFactory::Create(
    SiteInstance* instance,
    RenderViewHostDelegate* delegate,
    RenderWidgetHostDelegate* widget_delegate,
    int32_t routing_id,
    int32_t main_frame_routing_id,
    bool swapped_out,
    bool hidden) {
  // RenderViewHost creation can be either browser-driven (by the user opening a
  // new tab) or renderer-driven (by script calling window.open, etc).
  //
  // In the browser-driven case, the routing ID of the view is lazily assigned:
  // this is signified by passing MSG_ROUTING_NONE for |routing_id|.
  if (routing_id == MSG_ROUTING_NONE) {
    routing_id = instance->GetProcess()->GetNextRoutingID();
  } else {
    // Otherwise, in the renderer-driven case, the routing ID of the view is
    // already set. This is due to the fact that a sync render->browser IPC is
    // involved. In order to quickly reply to the sync IPC, the routing IDs are
    // assigned as early as possible. The IO thread immediately sends a reply to
    // the sync IPC, while deferring the creation of the actual Host objects to
    // the UI thread.
  }
  if (factory_) {
    return factory_->CreateRenderViewHost(instance, delegate, widget_delegate,
                                          routing_id, main_frame_routing_id,
                                          swapped_out);
  }
  return new RenderViewHostImpl(
      instance,
      base::MakeUnique<RenderWidgetHostImpl>(
          widget_delegate, instance->GetProcess(), routing_id, nullptr, hidden),
      delegate, main_frame_routing_id, swapped_out,
      true /* has_initialized_audio_host */);
}
```

​		RenderViewHostFactory 类的静态成员函数 Create 首先是创建了一个 RenderViewHostImpl 对象，然后再将这个 RenderViewHostImpl 对象返回给调用者。从前面 Chromium 的 Render 进程启动过程分析一文可以知道，<u>在创建 RenderViewHostImpl 对象的过程中，将会启动一个 Render 进程。这个 Render 进程接下来将会负责加载指定的 URL。</u>

​		这一步执行完成之后，回到 RenderFrameHostManager 类的成员函数 CreateRenderFrameHost 中，它接下来调用 RenderFrameHostFactory 类的静态成员函数 Create 创建了一个 RenderFrameHostImpl对象。这个 RenderFrameHostImpl 对象返回给 RenderFrameHostManager 类的成员函数 Init 后，将会被保存在成员变量 render_frame_host_ 中。

​		RenderFrameHostFactory 类的静态成员函数 Create 的实现如下所示：

```c++
// static
std::unique_ptr<RenderFrameHostImpl> RenderFrameHostFactory::Create(
    SiteInstance* site_instance,
    RenderViewHostImpl* render_view_host,
    RenderFrameHostDelegate* delegate,
    RenderWidgetHostDelegate* rwh_delegate,
    FrameTree* frame_tree,
    FrameTreeNode* frame_tree_node,
    int32_t routing_id,
    int32_t widget_routing_id,
    bool hidden,
    bool renderer_initiated_creation) {
  ......
  return base::WrapUnique(new RenderFrameHostImpl(
      site_instance, render_view_host, delegate, rwh_delegate, frame_tree,
      frame_tree_node, routing_id, widget_routing_id, hidden,
      renderer_initiated_creation));
}
```

​		从这里可以看到，RenderFrameHostFactory 类的静态成员函数 Create 创建的是一个RenderFrameHostImpl 对象，并且将这个 RenderFrameHostImpl 对象返回给调用者。

​		RenderFrameHostImpl 对象的创建过程，即 RenderFrameHostImpl 类的构造函数的实现，如下所示：

```c++
RenderFrameHostImpl::RenderFrameHostImpl(SiteInstance* site_instance,
                                         RenderViewHostImpl* render_view_host,
                                         RenderFrameHostDelegate* delegate,
                                         RenderWidgetHostDelegate* rwh_delegate,
                                         FrameTree* frame_tree,
                                         FrameTreeNode* frame_tree_node,
                                         int32_t routing_id,
                                         int32_t widget_routing_id,
                                         bool hidden,
                                         bool renderer_initiated_creation)
    : render_view_host_(render_view_host),
      ...... {
  ......
}
```

​		RenderFrameHostImpl 类的构造函数将参数 render_view_host 描述的一个R enderViewHostImpl对象保存在成员变量 render_view_host_ 中，这个 RenderViewHostImpl 对象就是前面调用RenderViewHostFactory 类的静态成员函数 Create 创建的。







这一步执行完成之后，回到 WebContentsImpl 类的成员函数 Init 中，它接下来还做有另外一个初始化工作，如下所示：

```c++
void WebContentsImpl::Init(const WebContents::CreateParams& params) {
  ......

  if (GuestMode::IsCrossProcessFrameGuest(this)) {
    view_.reset(new WebContentsViewChildFrame(
        this, delegate, &render_view_host_delegate_view_));
  } else {
    view_.reset(CreateWebContentsView(this, delegate,
                                      &render_view_host_delegate_view_));
  }

  .....
}
```

​		当 WebContentsImpl 类的成员变量 browser_plugin_guest_ 指向了一个 BrowserPluginGuest 对象时，表示当前正在处理的 WebContentsImpl 对象用来为一个 Browser Plugin 加载网页。我们不考虑这种情况。这时候 WebContentsImpl 类的成员函数 Init 就会调用函数 CreateWebContentsView 创建一个WebContentsViewAura 对象，并且保存在成员变量 view_ 中。

​		函数CreateWebContentsView的实现如下所示：

```c++
WebContentsView* CreateWebContentsView(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate,
    RenderViewHostDelegateView** render_view_host_delegate_view) {
  WebContentsViewAura* rv = new WebContentsViewAura(web_contents, delegate);
  *render_view_host_delegate_view = rv;
  return rv;
}
```

​		从这里可以看到，，函数CreateWebContentsView返回给调用者的是一个 WebContentsViewAura 对象。









​		这一步执行完成之后，回到静态成员函数 **`Navigate`**，这时候它就创建了一个WebContentsImpl 对象。之后在 contents 中加载 URL 。

​		LoadURLInContents 的实现如下：

```c++
void LoadURLInContents(WebContents* target_contents,
                       const GURL& url,
                       chrome::NavigateParams* params) {
  NavigationController::LoadURLParams load_url_params(url);
  load_url_params.source_site_instance = params->source_site_instance;
  load_url_params.referrer = params->referrer;
  load_url_params.frame_name = params->frame_name;
  load_url_params.frame_tree_node_id = params->frame_tree_node_id;
  load_url_params.redirect_chain = params->redirect_chain;
  load_url_params.transition_type = params->transition;
  load_url_params.extra_headers = params->extra_headers;
  load_url_params.should_replace_current_entry =
      params->should_replace_current_entry;
  load_url_params.is_renderer_initiated = params->is_renderer_initiated;
  load_url_params.started_from_context_menu = params->started_from_context_menu;

  if (params->uses_post) {
    load_url_params.load_type = NavigationController::LOAD_TYPE_HTTP_POST;
    load_url_params.post_data = params->post_data;
  }

  target_contents->GetController().LoadURLWithParams(load_url_params);
}
```

​		从上文中可以知道 target_contents 是我们刚刚创建的 WebContentsImpl，装配完参数之后调用该WebContentsImpl 对象的成员函数 GetController 获得一个 NavigationControllerImpl 对象再调用该NavigationControllerImpl 对象的成员函数 LoadURLWithParams 加载参数 params 描述的 URL，如下所示：

```c++
void NavigationControllerImpl::LoadURLWithParams(const LoadURLParams& params) {
  ......

  std::unique_ptr<NavigationEntryImpl> entry;

  // For subframes, create a pending entry with a corresponding frame entry.
  int frame_tree_node_id = params.frame_tree_node_id;
  if (frame_tree_node_id != -1 || !params.frame_name.empty()) {
    FrameTreeNode* node =
        params.frame_tree_node_id != -1
            ? delegate_->GetFrameTree()->FindByID(params.frame_tree_node_id)
            : delegate_->GetFrameTree()->FindByName(params.frame_name);
    if (node && !node->IsMainFrame()) {
      DCHECK(GetLastCommittedEntry());

      // Update the FTN ID to use below in case we found a named frame.
      frame_tree_node_id = node->frame_tree_node_id();

      // Create an identical NavigationEntry with a new FrameNavigationEntry for
      // the target subframe.
      entry = GetLastCommittedEntry()->Clone();
      entry->AddOrUpdateFrameEntry(
          node, -1, -1, nullptr,
          static_cast<SiteInstanceImpl*>(params.source_site_instance.get()),
          params.url, params.referrer, params.redirect_chain, PageState(),
          "GET", -1);
    }
  }
    
......
    
  LoadEntry(std::move(entry));
}
```

​		NavigationControllerImpl 类的成员函数 LoadURLWithParams 首先创建一个 NavigationEntryImpl 对象并装填一些信息，如下所示：



















