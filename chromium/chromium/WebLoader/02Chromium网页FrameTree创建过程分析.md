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

​		NavigationControllerImpl 类的成员函数 LoadURLWithParams 首先创建一个 NavigationEntryImpl 对象并装填一些信息，它将要加载的 URL 封装在一个 NavigationEntryImpl 对象之后，传递给另外一个成员函数LoadEntry，让后者执行加载 URL 的操作。

​		NavigationControllerImpl 类的成员函数 LoadEntry 的实现如下所示：

```c++
void NavigationControllerImpl::LoadEntry(
    std::unique_ptr<NavigationEntryImpl> entry) {
  // Remember the last pending entry for which we haven't received a response
  // yet. This will be deleted in the NavigateToPendingEntry() function.
  DCHECK_EQ(nullptr, last_pending_entry_);
  DCHECK_EQ(-1, last_pending_entry_index_);
  last_pending_entry_ = pending_entry_;
  last_pending_entry_index_ = pending_entry_index_;
  last_transient_entry_index_ = transient_entry_index_;

  pending_entry_ = nullptr;
  pending_entry_index_ = -1;
  // When navigating to a new page, we don't know for sure if we will actually
  // end up leaving the current page.  The new page load could for example
  // result in a download or a 'no content' response (e.g., a mailto: URL).
  SetPendingEntry(std::move(entry));
  NavigateToPendingEntry(ReloadType::NONE);
}
```

​		NavigationControllerImpl 类的成员函数 LoadEntry 首先调用另外一个成员函数 SetPendingEntry <u>将参数 entry 指向的一个 NavigationEntryImpl 对象保存在成员变量 pending\_entry_ 中，表示NavigationEntryImpl 对象封装的 URL 正在等待加载</u>，如下所示：

```c++
void NavigationControllerImpl::SetPendingEntry(
    std::unique_ptr<NavigationEntryImpl> entry) {
  ......
  pending_entry_ = entry.release();
  ......
}
```

​		回到 NavigationControllerImpl 类的成员函数 LoadEntry 中，将要加载的 URL 保存在成员变量pending_entry_ 之后，接下来就调用另外一个成员函数 NavigateToPendingEntry 对其进行加载，如下所示：

```c++
void NavigationControllerImpl::NavigateToPendingEntry(ReloadType reload_type) {
  ......
  bool success = NavigateToPendingEntryInternal(reload_type);
  ......
}
```

​		WebContentsImpl 类的成员函数 NavigateToPendingEntryInternal 的实现如下所示：

```c++
bool NavigationControllerImpl::NavigateToPendingEntryInternal(
    ReloadType reload_type) {
  DCHECK(pending_entry_);
  FrameTreeNode* root = delegate_->GetFrameTree()->root();

  // Compare FrameNavigationEntries to see which frames in the tree need to be
  // navigated.
  FrameLoadVector same_document_loads;
  FrameLoadVector different_document_loads;
  if (GetLastCommittedEntry()) {
    FindFramesToNavigate(root, &same_document_loads, &different_document_loads);
  }

  if (same_document_loads.empty() && different_document_loads.empty()) {
    // If we don't have any frames to navigate at this point, either
    // (1) there is no previous history entry to compare against, or
    // (2) we were unable to match any frames by name. In the first case,
    // doing a different document navigation to the root item is the only valid
    // thing to do. In the second case, we should have been able to find a
    // frame to navigate based on names if this were a same document
    // navigation, so we can safely assume this is the different document case.
    different_document_loads.push_back(
        std::make_pair(root, pending_entry_->GetFrameEntry(root)));
  }

  // If all the frame loads fail, we will discard the pending entry.
  bool success = false;

  // Send all the same document frame loads before the different document loads.
  for (const auto& item : same_document_loads) {
    FrameTreeNode* frame = item.first;
    success |= frame->navigator()->NavigateToPendingEntry(frame, *item.second,
                                                          reload_type, true);
  }
  for (const auto& item : different_document_loads) {
    FrameTreeNode* frame = item.first;
    success |= frame->navigator()->NavigateToPendingEntry(frame, *item.second,
                                                          reload_type, false);
  }
  return success;
}
```

​		从前面的分析可以知道，WebContentsImpl 类的成员变量 delegate_ 指向的是一个 WebContentsImpl 对象，调用这个 FrameTree 对象的成员函数 root 可以获得一个 FrameTreeNode 对象。这个 FrameTreeNode 对象代表的就是正在等待加载的网页的 Main Frame。再从 root 中找到需要导航的网页，然后通知网页进行导航，其中：先通知相同的文档，再通知不同的文档。

​		FrameTreeNode 类的成员函数 navigator 的实现如下所示：

```c++
class CONTENT_EXPORT FrameTreeNode {
 public:
  ......
 
  Navigator* navigator() {
    return navigator_.get();
  }
 
  ......
 
 private:
  ......
 
  scoped_refptr<Navigator> navigator_;
 
  ......
}
```

​		从前面的分析可以知道，FrameTreeNode 类的成员变量 navigator_ 指向的是一个 NavigatorImpl 对象，FrameTreeNode 对象的成员函数 navigator 会将这个 NavigatorImpl 对象返回给调用者。

​		NavigateToPendingEntry 加载正在等待加载的 URL，如下所示：

```c++
bool NavigatorImpl::NavigateToPendingEntry(
    FrameTreeNode* frame_tree_node,
    const FrameNavigationEntry& frame_entry,
    ReloadType reload_type,
    bool is_same_document_history_load) {
  return NavigateToEntry(frame_tree_node, frame_entry,
                         *controller_->GetPendingEntry(), reload_type,
                         is_same_document_history_load, false, true, nullptr);
}
```

​		从前面的分析可以知道，NavigatorImpl 类的成员变量 controller_ 描述的是一个NavigationControllerImpl 对象，NavigatorImpl 对象的成员函数 NavigateToPendingEntry 首先会调用这个 NavigationControllerImpl 对象的成员函数 GetPendingEntry 获得正在等待加载的 URL，如下所示：

```c++
NavigationEntryImpl* NavigationControllerImpl::GetPendingEntry() const {
  .......
  return pending_entry_;
}
```

​		NavigationControllerImpl 对象的成员函数 GetPendingEntry 返回的是成员变量 pending_entry_ 描述的一个 NavigationEntryImpl 对象。从前面的分析可以知道，这个 NavigationEntryImpl 对象描述的就是正在等待加载的 URL。

​		回到 NavigatorImpl 类的成员函数 NavigateToPendingEntry 中，获得了等待加载的 URL 之后，它接下来调用另外一个成员函数 NavigateToEntry 对该 URL 进行加载，如下所示：

```c++
bool NavigatorImpl::NavigateToEntry(
    FrameTreeNode* frame_tree_node,
    const FrameNavigationEntry& frame_entry,
    const NavigationEntryImpl& entry,
    ReloadType reload_type,
    bool is_same_document_history_load,
    bool is_history_navigation_in_new_child,
    bool is_pending_entry,
    const scoped_refptr<ResourceRequestBody>& post_body) {
  ......
    RenderFrameHostImpl* dest_render_frame_host =
        frame_tree_node->render_manager()->Navigate(
            dest_url, frame_entry, entry, reload_type != ReloadType::NONE);
    ......
    if (!is_transfer_to_same) {
      navigation_data_.reset(new NavigationMetricsData(
          navigation_start, dest_url, entry.restore_type()));
      // Create the navigation parameters.
      FrameMsg_Navigate_Type::Value navigation_type = GetNavigationType(
          frame_tree_node->current_url(),  // old_url
          dest_url,                        // new_url
          reload_type,                     // reload_type
          entry,                           // entry
          frame_entry,                     // frame_entry
          is_same_document_history_load);  // is_same_document_history_load

      dest_render_frame_host->Navigate(
          entry.ConstructCommonNavigationParams(
              frame_entry, post_body, dest_url, dest_referrer, navigation_type,
              previews_state, navigation_start),
          entry.ConstructStartNavigationParams(),
          entry.ConstructRequestNavigationParams(
              frame_entry, GURL(), std::string(),
              is_history_navigation_in_new_child,
              entry.GetSubframeUniqueNames(frame_tree_node),
              frame_tree_node->has_committed_real_load(),
              controller_->GetPendingEntryIndex() == -1,
              controller_->GetIndexOfEntry(&entry),
              controller_->GetLastCommittedEntryIndex(),
              controller_->GetEntryCount()));
    } else {
      dest_render_frame_host->navigation_handle()->set_is_transferring(false);
    }
......
}
```

​		从前面的调用过程可以知道，参数 frame_tree_node 描述的就是前面 for 循环中的 FrameTreeNode 对象。NavigatorImpl 类的成员函数 Navigate 首先获得这个 RenderFrameHostImpl 对象对应的 Frame Tree Node 所关联的 RenderFrameHostManager 对象（dest_render_frame_host）。有了这个RenderFrameHostManager 对象之后，NavigatorImpl 类的成员函数 NavigateToEntry 就调用它的成员函数 Navigate，用来通知它即将要导航到指定的 URL 去。

​		RenderFrameHostManager 类的成员函数 Navigate 会返回一个 RenderFrameHostImpl 对象dest_render_frame_host。RenderFrameHostImpl 对象 dest_render_frame_host 与RenderFrameHostImpl 对象 render_frame_host 有可能是相同的，也有可能是不同的。什么情况下相同呢？如果上述 RenderFrameHostManager 对象即将要导航到的 URL 与它之前已经导航至的 URL 属于相同站点，那么就是相同的。反之则是不同的。无论是哪一种情况，导航至指定的 URL 的操作最后都是通过调用RenderFrameHostImpl 对象 dest_render_frame_host 的成员函数 Navigate 完成的。

​		接下来我们就先分析 RenderFrameHostManager 类的成员函数 Navigate 的实现，然后再分析RenderFrameHostImpl 类的成员函数 Navigate 的实现。

​		RenderFrameHostManager 类的成员函数 Navigate 的实现如下所示：

```c++
RenderFrameHostImpl* RenderFrameHostManager::Navigate(
    const GURL& dest_url,
    const FrameNavigationEntry& frame_entry,
    const NavigationEntryImpl& entry,
    bool is_reload) {
  ......
  RenderFrameHostImpl* dest_render_frame_host = UpdateStateForNavigate(
      dest_url, frame_entry.source_site_instance(), frame_entry.site_instance(),
      entry.GetTransitionType(), entry.restore_type() != RestoreType::NONE,
      entry.IsViewSourceMode(), entry.transferred_global_request_id(),
      entry.bindings(), is_reload);
  ......
  if (!dest_render_frame_host->IsRenderFrameLive()) {
    .......
    if (!ReinitializeRenderFrame(dest_render_frame_host))
      return nullptr;

    ......
  }
......

  return dest_render_frame_host;
}
```

​		RenderFrameHostManager 类的成员函数 Navigate 首先调用另外一个成员函数 UpdateStateForNavigate  获得与即将要加载的 URL 对应的一个 RenderFrameHostImpl 对象。获得了这个 RenderFrameHostImpl 对象之后，调用它的成员函数 render_view_host 可以获得一个 RenderViewHostImpl 对象。这个 RenderViewHostImpl 对象是在前面分析的 RenderFrameHostManager 类的成员函数 CreateRenderFrameHost 中创建的。有了这个 RenderViewHostImpl 对象之后，就可以调用它的成员函数 IsRenderViewLive 判断它是否关联有一个 Render View 控件。这个 RenderView 控件是一个由平台实现的控件，描述的是用来显示网页的一个区域。

​		在两种情况下，一个 RenderViewHostImpl 对象没有关联一个 Render View 控件。第一种情况是这个RenderViewHostImpl 对象还没有加载过 URL。第二种情况下是这个 RenderViewHostImpl 对象加载过 URL，但是由于某种原因，负责加载该 URL 的 Render 进程崩溃了。在第二种情况下，一个 RenderViewHostImpl 对象关联的 Render View 控件会被销毁，所以会导致它没有关联 Render View 控件。无论是上述两种情况的哪一种，RenderFrameHostManager 类的成员函数 Navigate 都会调用另外一个成员函数 ReinitializeRenderFrame 为其关联一个 RenderView 控件。

​		 接下来，我们就分别分析 RenderFrameHostManager 类的成员函数 UpdateStateForNavigate 和InitRenderView 的实现。

​		RenderFrameHostManager 类的成员函数 UpdateStateForNavigate 的实现如下所示：

```c++
RenderFrameHostImpl* RenderFrameHostManager::UpdateStateForNavigate(
    const GURL& dest_url,
    SiteInstance* source_instance,
    SiteInstance* dest_instance,
    ui::PageTransition transition,
    bool dest_is_restore,
    bool dest_is_view_source_mode,
    const GlobalRequestID& transferred_request_id,
    int bindings,
    bool is_reload) {
  SiteInstance* current_instance = render_frame_host_->GetSiteInstance();
  bool was_server_redirect = transfer_navigation_handle_ &&
                             transfer_navigation_handle_->WasServerRedirect();
  scoped_refptr<SiteInstance> new_instance = GetSiteInstanceForNavigation(
      dest_url, source_instance, dest_instance, nullptr, transition,
      dest_is_restore, dest_is_view_source_mode, was_server_redirect);

  ......
    RenderFrameHostImpl* transferring_rfh =
        transfer_navigation_handle_->GetRenderFrameHost();
    bool transfer_started_from_current_rfh =
        transferring_rfh == render_frame_host_.get();
    bool should_transfer =
        new_instance.get() != transferring_rfh->GetSiteInstance() &&
        (!transfer_started_from_current_rfh || allowed_to_swap_process);
    if (should_transfer)
      transfer_navigation_handle_->Transfer();
  }
......

  if (new_instance.get() != current_instance && allowed_to_swap_process) {
    ......
      return render_frame_host_.get();
    }
.....

  return render_frame_host_.get();
}
```

​		RenderFrameHostManager 类的成员变量 render_frame_host_ 描述的是一个RenderFrameHostImpl 对象。这个 RenderFrameHostImpl 对象是在前面分析的 RenderFrameHostFactory 类的静态成员函数 Create 中创建的，与前面分析的 NavigatorImpl 类的成员函数 NavigateToEntry 的参数 render_frame_host 描述的 RenderFrameHostImpl 对象是相同的。

​		RenderFrameHostManager 类的成员函数 **<u>UpdateStateForNavigate 主要做的事情就是检查即将要加载的 URL，即参数 entry 描述的一个 URL，与当前已经加载的 URL，是否属于相同的站点。如果是不相同的站点，那么 RenderFrameHostManager 类的成员变量 pending\_render\_frame\_host_ 会指向另外一个 RenderFrameHostImpl 对象。这个 RenderFrameHostImpl 对象负责加载参数 entry 描述的URL。因此，这个 RenderFrameHostImpl 对象会返回给调用者。</u>**

​		另一方面，如果即将要加载的 URL 与当前已经加载的 URL 是相同的站点，那么RenderFrameHostManager 类的成员函数 UpdateStateForNavigate 返回的是成员变量render_frame_host_ 描述的 RenderFrameHostImpl 对象。

​		回到 RenderFrameHostManager 类的成员函数 Navigate 中，我们假设它通过调用成员函数UpdateStateForNavigate 获得的 RenderFrameHostImpl 对象还没有关联一个 RenderView 控件，这时候它就会调用另外一个成员函数 InitRenderView 为这个 RenderFrameHostImpl 关联一个 RenderView 控件。

ReinitializeRenderFrame 的实现如下所示：

```c++
bool RenderFrameHostManager::ReinitializeRenderFrame(
    RenderFrameHostImpl* render_frame_host) {
  CreateOpenerProxies(render_frame_host->GetSiteInstance(), frame_tree_node_);

  if (!frame_tree_node_->parent()) {
    DCHECK(!GetRenderFrameProxyHost(render_frame_host->GetSiteInstance()));
    if (!InitRenderView(render_frame_host->render_view_host(), nullptr))
      return false;
  } else {
    if (!InitRenderFrame(render_frame_host))
      return false;
    RenderFrameProxyHost* proxy_to_parent = GetProxyToParent();
    if (proxy_to_parent)
      GetProxyToParent()->SetChildRWHView(render_frame_host->GetView());
  }

  return true;
}
```

​		对于跨进程子帧，InitRenderView 不会重新创建 RenderFrame，因此改用 InitRenderFrame。

​		RenderFrameHostManager 类的成员函数 InitRenderView 的实现如下所示：

```c++
bool RenderFrameHostManager::InitRenderView(
    RenderViewHostImpl* render_view_host,
    RenderFrameProxyHost* proxy) {
  .......

  bool created = delegate_->CreateRenderViewForRenderManager(
      render_view_host, opener_frame_routing_id,
      proxy ? proxy->GetRoutingID() : MSG_ROUTING_NONE,
      frame_tree_node_->devtools_frame_token(),
      frame_tree_node_->current_replication_state());

  if (created && proxy)
    proxy->set_render_frame_proxy_created(true);

  return created;
}
```

​		从前面的分析可以知道，RenderFrameHostManager 类的成员变量 delegate_ 描述的是一个WebContentsImpl 对象。RenderFrameHostManager 类的成员函数 InitRenderView 调用这个WebContentsImpl 对象的成员函数 CreateRenderViewForRenderManager  为参数 render_view_host 描述的一个 RenderViewHostImpl 对象创建一个 RenderView 控件。这个 RenderViewHostImpl 对象是与RenderFrameHostManager 类的成员函数 UpdateStateForNavigate 返回的 RenderFrameHostImpl 对象关联的，因此，这里调用成员变量 delegate_ 描述的 WebContentsImpl 对象的成员函数CreateRenderViewForRenderManager 实际上是为该 RenderFrameHostImpl 对象创建一个 RenderView 控件。

​		WebContentsImpl 类的成员函数 CreateRenderViewForRenderManager 的实现如下所示：

```c++
bool WebContentsImpl::CreateRenderViewForRenderManager(
    RenderViewHost* render_view_host,
    int opener_frame_routing_id,
    int proxy_routing_id,
    const base::UnguessableToken& devtools_frame_token,
    const FrameReplicationState& replicated_frame_state) {
    .....
  if (proxy_routing_id == MSG_ROUTING_NONE)
    CreateRenderWidgetHostViewForRenderManager(render_view_host);

  if (!static_cast<RenderViewHostImpl*>(render_view_host)
           ->CreateRenderView(opener_frame_routing_id, proxy_routing_id,
                              devtools_frame_token, replicated_frame_state,
                              created_with_opener_)) {
    return false;
  }
    .......

  return true;
}

void WebContentsImpl::CreateRenderWidgetHostViewForRenderManager(
    RenderViewHost* render_view_host) {
  RenderWidgetHostViewBase* rwh_view =
      view_->CreateViewForWidget(render_view_host->GetWidget(), false);
	......
}
```

​		当参数 proxy_routing_id 的值等于 MSG_ROUTING_NONE 的时候，表示 WebContentsImpl 类的成员函数CreateRenderViewForRenderManager 要为一个网页的 Main Frame 创建一个 Render View 控件，而当参数

​		从这里可以看到，网页的 Main Frame 关联的 Render View 控件是通过调用 WebContentsImpl 类的成员变量 view_ 指向的一个 CreateViewForWidget 创建的。

​		为参数 render_view_host 描述的 RenderViewHostImpl 对象创建了一个 Render View 控件之后，WebContentsImpl 类的成员函数 CreateRenderViewForRenderManager 接下来还会调用这个RenderViewHostImpl 对象的成员函数 CreateRenderView 请求<u>在对应的 Render 进程中创建一个RenderFrameImpl 对象</u>。这个 RenderFrameImpl 对象与前面通过调用成员函数 UpdateStateForNavigate 获得的 RenderFrameHostImpl 对象相对应，也就是这两个对象以后可以进行 Render Frame 相关的进程间通信操作。

​		RenderViewHostImpl 类的成员函数 CreateRenderView 的实现如下所示：

```c++
bool RenderViewHostImpl::CreateRenderView(
    int opener_frame_route_id,
    int proxy_route_id,
    const base::UnguessableToken& devtools_frame_token,
    const FrameReplicationState& replicated_frame_state,
    bool window_was_created_with_opener) {
......
  mojom::CreateViewParamsPtr params = mojom::CreateViewParams::New();
  params->renderer_preferences =
      delegate_->GetRendererPrefs(GetProcess()->GetBrowserContext());
  GetPlatformSpecificPrefs(&params->renderer_preferences);
  params->web_preferences = GetWebkitPreferences();
  params->view_id = GetRoutingID();
  params->main_frame_routing_id = main_frame_routing_id_;
  if (main_frame_routing_id_ != MSG_ROUTING_NONE) {
    RenderFrameHostImpl* main_rfh = RenderFrameHostImpl::FromID(
        GetProcess()->GetID(), main_frame_routing_id_);
    DCHECK(main_rfh);
    RenderWidgetHostImpl* main_rwh = main_rfh->GetRenderWidgetHost();
    params->main_frame_widget_routing_id = main_rwh->GetRoutingID();
  }
  params->session_storage_namespace_id =
      delegate_->GetSessionStorageNamespace(instance_.get())->id();
  // Ensure the RenderView sets its opener correctly.
  params->opener_frame_route_id = opener_frame_route_id;
  params->swapped_out = !is_active_;
  params->replicated_frame_state = replicated_frame_state;
  params->proxy_routing_id = proxy_route_id;
  params->hidden = is_active_ ? GetWidget()->is_hidden()
                              : GetWidget()->delegate()->IsHidden();
  params->never_visible = delegate_->IsNeverVisible();
  params->window_was_created_with_opener = window_was_created_with_opener;
  params->enable_auto_resize = GetWidget()->auto_resize_enabled();
  params->min_size = GetWidget()->min_size_for_auto_resize();
  params->max_size = GetWidget()->max_size_for_auto_resize();
  params->page_zoom_level = delegate_->GetPendingPageZoomLevel();
  params->devtools_main_frame_token = devtools_frame_token;

  GetWidget()->GetResizeParams(&params->initial_size);
  GetWidget()->SetInitialRenderSizeParams(params->initial_size);

  GetProcess()->GetRendererInterface()->CreateView(std::move(params));

  // Let our delegate know that we created a RenderView.
  DispatchRenderViewCreated();

  // Since this method can create the main RenderFrame in the renderer process,
  // set the proper state on its corresponding RenderFrameHost.
  if (main_frame_routing_id_ != MSG_ROUTING_NONE) {
    RenderFrameHostImpl::FromID(GetProcess()->GetID(), main_frame_routing_id_)
        ->SetRenderFrameCreated(true);
  }
  GetWidget()->delegate()->SendScreenRects();
  PostRenderViewReady();

  return true;
}
```

​		RenderViewHostImpl 类的成员函数 CreateRenderView 主要就是向与当前正在处理的RenderViewHostImpl 对象对应的 Render 进程发送一个类型为 ViewMsg_New 的 IPC 消息。方式是调用 CreateView。 这个 Render 进程是在创建当前正在处理的 RenderViewHostImpl对象 时启动的，这一点可以参考前面分析的RenderViewHostFactory 类的静态成员函数 Create。

​		CreateView 的实现如下：

```c++
void RendererProxy::CreateView(
    CreateViewParamsPtr in_params) {
  const bool kExpectsResponse = false;
  const bool kIsSync = false;
  
  const uint32_t kFlags =
      ((kExpectsResponse) ? mojo::Message::kFlagExpectsResponse : 0) |
      ((kIsSync) ? mojo::Message::kFlagIsSync : 0);
  
  mojo::Message message(
      internal::kRenderer_CreateView_Name, kFlags, 0, 0, nullptr);
  auto* buffer = message.payload_buffer();
  ::content::mojom::internal::Renderer_CreateView_Params_Data::BufferWriter
      params;
  mojo::internal::SerializationContext serialization_context;
  params.Allocate(buffer);
  typename decltype(params->params)::BaseType::BufferWriter
      params_writer;
  mojo::internal::Serialize<::content::mojom::CreateViewParamsDataView>(
      in_params, buffer, &params_writer, &serialization_context);
  params->params.Set(
      params_writer.is_null() ? nullptr : params_writer.data());
  MOJO_INTERNAL_DLOG_SERIALIZATION_WARNING(
      params->params.is_null(),
      mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER,
      "null params in Renderer.CreateView request");
  message.AttachHandlesFromSerializationContext(
      &serialization_context);
  // This return value may be ignored as false implies the Connector has
  // encountered an error, which will be visible through other means.
  ignore_result(receiver_->Accept(&message));
}
```

​		主要就是向与当前正在处理的 RenderViewHostImpl 对象对应的 Render 进程发送一个 IPC **Accept** 消息。



​		在 Render 进程中，类型 IPC 消息由 RendererStubDispatch 类的成员函数 **Accept** 进行接收，如下所示：

```c++
// static
bool RendererStubDispatch::Accept(
    Renderer* impl,
    mojo::Message* message) {
  switch (message->header()->name) {
    case internal::kRenderer_CreateView_Name: {
      mojo::internal::MessageDispatchContext context(message);

      DCHECK(message->is_serialized());
      internal::Renderer_CreateView_Params_Data* params =
          reinterpret_cast<internal::Renderer_CreateView_Params_Data*>(
              message->mutable_payload());
      
      mojo::internal::SerializationContext serialization_context;
      serialization_context.TakeHandlesFromMessage(message);
      bool success = true;
      CreateViewParamsPtr p_params{};
      Renderer_CreateView_ParamsDataView input_data_view(params, &serialization_context);
      
      if (!input_data_view.ReadParams(&p_params))
        success = false;
      if (!success) {
        ReportValidationErrorForMessage(
            message,
            mojo::internal::VALIDATION_ERROR_DESERIALIZATION_FAILED,
            "Renderer::CreateView deserializer");
        return false;
      }
      // A null |impl| means no implementation was bound.
      assert(impl);
      impl->CreateView(
std::move(p_params));
      return true;
    }
  }
......
}
```

​		从这里可以看到，RendererStubDispatch 中直接调用 RenderThreadImpl 静态成员函数 CreateView 创建一个 RenderViewImpl 对象

```c++
void RenderThreadImpl::CreateView(mojom::CreateViewParamsPtr params) {
  CompositorDependencies* compositor_deps = this;
  is_scroll_animator_enabled_ = params->web_preferences.enable_scroll_animator;
  // When bringing in render_view, also bring in webkit's glue and jsbindings.
  RenderViewImpl::Create(compositor_deps, *params,
                         RenderWidget::ShowCallback());
}
```

​		RenderViewImpl 类的静态成员函数 Create 创建一个 RenderViewImpl 对象，RenderViewImpl 类的静态成员函数 Create 的实现如下所示：

```c++
/*static*/
RenderViewImpl* RenderViewImpl::Create(
    CompositorDependencies* compositor_deps,
    const mojom::CreateViewParams& params,
    const RenderWidget::ShowCallback& show_callback) {
  DCHECK(params.view_id != MSG_ROUTING_NONE);
  RenderViewImpl* render_view = NULL;
  if (g_create_render_view_impl)
    render_view = g_create_render_view_impl(compositor_deps, params);
  else
    render_view = new RenderViewImpl(compositor_deps, params);

  render_view->Initialize(params, show_callback);
  return render_view;
}
```

​		RenderViewImpl 类的静态成员函数 Create 主要就是创建了一个 RenderViewImpl 对象，并且调用这个RenderViewImpl 对象的成员函数 Initialize 对其进行初始化。

​		RenderViewImpl 类的成员函数 Initialize 的实现如下所示：

```c++
void RenderViewImpl::Initialize(
    const mojom::CreateViewParams& params,
    const RenderWidget::ShowCallback& show_callback) {
......

  webview_ = WebView::Create(this, is_hidden()
                                       ? blink::kWebPageVisibilityStateHidden
                                       : blink::kWebPageVisibilityStateVisible);
  RenderWidget::Init(show_callback, webview_->GetWidget());
.......
  WebFrame* opener_frame =
      RenderFrameImpl::ResolveOpener(params.opener_frame_route_id);

  if (params.main_frame_routing_id != MSG_ROUTING_NONE) {
    main_render_frame_ = RenderFrameImpl::CreateMainFrame(
        this, params.main_frame_routing_id, params.main_frame_widget_routing_id,
        params.hidden, screen_info(), compositor_deps_, opener_frame,
        params.devtools_main_frame_token, params.replicated_frame_state);
  }

......

  if (main_render_frame_)
    main_render_frame_->Initialize();

  // If this RenderView's creation was initiated by an opener page in this
  // process, (e.g. window.open()), we won't be visible until we ask the opener,
  // via show_callback, to make us visible. Otherwise, we went through a
  // browser-initiated creation, and show() won't be called.
  if (!was_created_by_renderer)
    did_show_ = true;

  // TODO(davidben): Move this state from Blink into content.
  if (params.window_was_created_with_opener)
    webview()->SetOpenedByDOM();

  UpdateWebViewWithDeviceScaleFactor();
  OnSetRendererPrefs(params.renderer_preferences);

  if (!params.enable_auto_resize) {
    OnResize(params.initial_size);
  } else {
    OnEnableAutoResize(params.min_size, params.max_size);
  }

  idle_user_detector_.reset(new IdleUserDetector(this));

  GetContentClient()->renderer()->RenderViewCreated(this);

  page_zoom_level_ = params.page_zoom_level;
}

```

​		WebViewImpl 类的静态成员函数 create 根据这个 RenderViewImpl 对象创建了一个 WebViewImpl 对象，并且返回给调用者。

```c++
WebView* WebView::Create(WebViewClient* client,
                         WebPageVisibilityState visibility_state) {
  return WebViewImpl::Create(client, visibility_state);
}

WebViewImpl* WebViewImpl::Create(WebViewClient* client,
                                 WebPageVisibilityState visibility_state) {
  // Pass the WebViewImpl's self-reference to the caller.
  auto web_view = WTF::AdoptRef(new WebViewImpl(client, visibility_state));
  web_view->AddRef();
  return web_view.get();
}
```

​		WebViewImpl对象的创建过程，即WebViewImpl类的构造函数的实现，如下所示：

```c++
WebViewImpl::WebViewImpl(WebViewClient* client,
                         WebPageVisibilityState visibility_state)
    : client_(client),
     ...... {
  Page::PageClients page_clients;
  page_clients.chrome_client = chrome_client_.Get();
  page_clients.context_menu_client = &context_menu_client_;
  page_clients.editor_client = &editor_client_;
  page_clients.spell_checker_client = &spell_checker_client_impl_;

  page_ = Page::CreateOrdinary(page_clients);
  CoreInitializer::GetInstance().ProvideModulesToPage(*page_, client_);
  page_->SetValidationMessageClient(ValidationMessageClientImpl::Create(*this));
  SetVisibilityState(visibility_state, true);

  InitializeLayerTreeView();

  dev_tools_emulator_ = DevToolsEmulator::Create(this);

  AllInstances().insert(this);

  page_importance_signals_.SetObserver(client);
  resize_viewport_anchor_ = new ResizeViewportAnchor(*page_);
}
```

​		WebViewImpl 类的构造函数将参数 RenderViewImpl 指向的一个 RenderViewImpl 对象保存在成员变量client_ 中，并且还会创建一个Page对象，保存在成员变量 page_ 中。



​		RenderViewImpl 类的成员函数 Initialize 首先调用 RenderFrameImpl 类的静态成员函数 CreateMainFrame 创建了一个 RenderFrameImpl 对象，如下所示：

```c++
// static
RenderFrameImpl* RenderFrameImpl::CreateMainFrame(
    RenderViewImpl* render_view,
    int32_t routing_id,
    int32_t widget_routing_id,
    bool hidden,
    const ScreenInfo& screen_info,
    CompositorDependencies* compositor_deps,
    blink::WebFrame* opener,
    const base::UnguessableToken& devtools_frame_token,
    const FrameReplicationState& replicated_state) {
    
  RenderFrameImpl* render_frame =
      RenderFrameImpl::Create(render_view, routing_id, devtools_frame_token);
  render_frame->InitializeBlameContext(nullptr);
  WebLocalFrame* web_frame = WebLocalFrame::CreateMainFrame(
      render_view->webview(), render_frame,
      render_frame->blink_interface_registry_.get(), opener,
      // This conversion is a little sad, as this often comes from a
      // WebString...
      WebString::FromUTF8(replicated_state.name),
      replicated_state.sandbox_flags);
  render_frame->render_widget_ = RenderWidget::CreateForFrame(
      widget_routing_id, hidden, screen_info, compositor_deps, web_frame);
  // TODO(avi): This DCHECK is to track cleanup for https://crbug.com/545684
  DCHECK_EQ(render_view->GetWidget(), render_frame->render_widget_)
      << "Main frame is no longer reusing the RenderView as its widget! "
      << "Does the RenderFrame need to register itself with the RenderWidget?";
  render_frame->in_frame_tree_ = true;
  return render_frame;
}

// static
RenderFrameImpl* RenderFrameImpl::Create(
    RenderViewImpl* render_view,
    int32_t routing_id,
    const base::UnguessableToken& devtools_frame_token) {
  DCHECK(routing_id != MSG_ROUTING_NONE);
  CreateParams params(render_view, routing_id, devtools_frame_token);

  if (g_create_render_frame_impl)
    return g_create_render_frame_impl(params);
  else
    return new RenderFrameImpl(params);
}
```

​		从这里可以看到，RenderFrameImpl 类的静态成员函数 Create 创建的是一个 RenderFrameImpl 对象。这个 RenderFrameImpl 对象保存在 RenderViewImpl 类的成员变量 main_render_frame_ 中。

​		它接下来调用 WebLocalFrame 类的静态成员函数 CreateMainFrame 创建一个 WebLocalFrameImpl 对象，如下所示：

```c++
WebLocalFrame* WebLocalFrame::CreateMainFrame(
    WebView* web_view,
    WebFrameClient* client,
    InterfaceRegistry* interface_registry,
    WebFrame* opener,
    const WebString& name,
    WebSandboxFlags sandbox_flags) {
  return WebLocalFrameImpl::CreateMainFrame(
      web_view, client, interface_registry, opener, name, sandbox_flags);
}
```

​		这个 WebLocalFrameImpl 对象返回到 RenderFrameImpl 类的成员函数 CreateMainFrame 之后，会调用 RenderWidget 的静态函数 CreateForFrame 来根据这个 MainFrame 生成一个 RenderWidget 对象。

​		RenderWidget::CreateForFrame 的实现如下：

```c++
// static
RenderWidget* RenderWidget::CreateForFrame(
    int widget_routing_id,
    bool hidden,
    const ScreenInfo& screen_info,
    CompositorDependencies* compositor_deps,
    blink::WebLocalFrame* frame) {
  CHECK_NE(widget_routing_id, MSG_ROUTING_NONE);
  // TODO(avi): Before RenderViewImpl has-a RenderWidget, the browser passes the
  // same routing ID for both the view routing ID and the main frame widget
  // routing ID. https://crbug.com/545684
  RenderViewImpl* view = RenderViewImpl::FromRoutingID(widget_routing_id);
  if (view) {
    view->AttachWebFrameWidget(
        RenderWidget::CreateWebFrameWidget(view->GetWidget(), frame));
    return view->GetWidget();
  }
  scoped_refptr<RenderWidget> widget(
      g_create_render_widget
          ? g_create_render_widget(widget_routing_id, compositor_deps,
                                   blink::kWebPopupTypeNone, screen_info, false,
                                   hidden, false)
          : new RenderWidget(widget_routing_id, compositor_deps,
                             blink::kWebPopupTypeNone, screen_info, false,
                             hidden, false));
  widget->for_oopif_ = true;
  // Init increments the reference count on |widget|, keeping it alive after
  // this function returns.
  widget->Init(RenderWidget::ShowCallback(),
               RenderWidget::CreateWebFrameWidget(widget.get(), frame));

  if (g_render_widget_initialized)
    g_render_widget_initialized(widget.get());
  return widget.get();
}
```













​		这一步执行完成后，Render 进程就处理完毕从 Browser 进程发送过来的 IPC 消息。Render 进程在处理 IPC消息的过程中，主要就是创建了一个 RenderViewImpl 对象、一个 RenderFrameImpl 对象、一个WebLocalFrameImpl 对象、一个 LocalFrame 对象。这些对象的关系和作用可以参考前面 Chromium 网页加载过程简要介绍和学习计划一文的介绍。创建这些对象是为后面加载网页内容作准备的。

​		回到 NavigatorImpl 类的成员函数 NavigateToEntry，它向 Render 进程发送了一个类型为 ViewMsg_New 的 IPC 消息之后，接下来又会调用 RenderFrameHostImpl 类的成员函数 Navigate 请求导航到指定的 URL。

​		RenderFrameHostImpl 类的成员函数Navigate的实现如下所示：

```c++
void RenderFrameHostImpl::Navigate(
    const CommonNavigationParams& common_params,
    const StartNavigationParams& start_params,
    const RequestNavigationParams& request_params) {
......
  // Only send the message if we aren't suspended at the start of a cross-site
  // request.
  if (navigations_suspended_) {
    // This may replace an existing set of params, if this is a pending RFH that
    // is navigated twice consecutively.
    suspended_nav_params_.reset(
        new NavigationParams(common_params, start_params, request_params));
  } else {
    // Get back to a clean state, in case we start a new navigation without
    // completing an unload handler.
    ResetWaitingState();
    SendNavigateMessage(common_params, start_params, request_params);
  }
......
}
```

​		从前面的分析可以知道，RenderFrameHostImpl 类的成员变量 render_view_host_ 描述的是一个RenderViewHostImpl 对象。当这个 RenderViewHostImpl 对象的成员变量 navigations_suspended_ 的值等于true 的时候，表示参数 params 描述的 URL 被挂起加载，这时候 RenderFrameHostImpl 类的成员函数 Navigate 将要加载的 URL 记录起来，等到挂起被恢复时，再进行加载。

​		另一方面，RenderFrameHostImpl 类的成员变量 render_view_host_ 描述的 RenderViewHostImpl 对象的成员变量 navigations_suspended_ 的值等于 false 时，表示要马上加载参数 params 描述的 URL，这时候RenderFrameHostImpl 类的成员函数 Navigate 就会向一个 Render 进程发送一个类型为 FrameMsg_Navigate的 IPC 消息，请求该 Render 进程加载该 URL 对应的网页。Render 进程加载网页的过程，我们在接下来的一篇文章中再分析。

### 总结

​		至此，我们就分析完成一个网页对应的 Frame Tree 的创建过程了。这个 Frame Tree 是在网页内容加载过程之前在 Browser 进程中创建的。要加载的网页对应于这个 Frame Tree 的一个 Node。这个 Node 又关联有一个RenderFrameHostImpl 对象。这个 RenderFrameHostImpl 对象在 Render 进程又对应有一个RenderFrameImpl 对象。RenderFrameHostImpl 对象和 RenderFrameImpl 对象负责在 Browser 进程和Render 进程之间处理网页导航相关的操作。



















