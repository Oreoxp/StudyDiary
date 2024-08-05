[TOC]

# 网页 Layer Tree 创建过程分析

​		在 Chromium 中，WebKit 会创建一个 Graphics Layer Tree 描述网页。Graphics Layer Tree 是和网页渲染相关的一个 Tree。网页渲染最终由 Chromium 的 CC 模块完成，因此 CC 模块又会根据 Graphics Layer Tree 创建一个 Layer Tree，以后就会根据这个 Layer Tree 对网页进行渲染。本文接下来就分析网页 Layer Tree 的创建过程。

​		从前面Chromium网页Graphics Layer Tree创建过程分析一文可以知道，网页的 Graphics Layer Tree 是根据 Render Layer Tree 创建的，Render Layer Tree 又是根据 Render Object Tree 创建的。Graphics Layer Tree 与 Render Layer Tree、Render Layer Tree与  Render Object Tree的节点是均是一对多的关系，然而 Graphics Layer Tree 与 CC 模块创建的 Layer Tree 的节点是**一一对应的关系**，如图1所示：

![img](markdownimage/20160323014354693)

​		也就是说，每一个 Graphics Layer 都对应有一个 CC Layer。不过，Graphics Layer 与 CC Layer **不是直接的一一对应的**，它们是透过另外两个 Layer 才对应起来的，如图2所示：

![img](markdownimage/20160323024815959)

​		中间的两个 Layer 分别是 WebContentLayerImpl 和 WebLayerImpl，它们是属于 Content 层的对象。关于 Chromium 的层次划分，可以参考前面 Chromium 网页加载过程简要介绍和学习计划一文的介绍。Graphics Layer 与 CC Layer 的对应关系，是在 Graphics Layer 的创建过程中建立起来的，接下来我们就通过源码分析这种对应关系的建立过程。

​		从前面Chromium网页Graphics Layer Tree创建过程分析一文可以知道，Graphics Layer 是通过调用GraphicsLayerFactoryChromium 类的成员函数 createGraphicsLayer 创建的，如下所示：

```c++
PassOwnPtr<GraphicsLayer> GraphicsLayerFactoryChromium::createGraphicsLayer(GraphicsLayerClient* client)  
{  
    OwnPtr<GraphicsLayer> layer = adoptPtr(new GraphicsLayer(client));  
    ......  
    return layer.release();  
}  
```

​		参数 client 指向的实际上是一个 CompositedLayerMapping 对象，这个 CompositedLayerMapping 对象会用来构造一个 Graphics Layer。Graphics Layer 的构造过程，也就是 GraphicsLayer 类的构造函数的实现，如下所示：

```c++
GraphicsLayer::GraphicsLayer(GraphicsLayerClient* client)
    : m_client(client)
    , ...... 
{
    ......
 
    m_opaqueRectTrackingContentLayerDelegate = adoptPtr(new OpaqueRectTrackingContentLayerDelegate(this));
    m_layer = adoptPtr(Platform::current()->compositorSupport()->createContentLayer(m_opaqueRectTrackingContentLayerDelegate.get()));
    
    ......
}
```

​		GraphicsLayer 类的构造函数首先是将参数 client 指向的 CompositedLayerMapping 对象保存在成员变量 m_client 中，接着又创建了一个 OpaqueRectTrackingContentLayerDelegate 对象保存在成员变量opaqueRectTrackingContentLayerDelegate 中。

​		再接下来 GraphicsLayer 类的构造函数通过 Platform 类的静态成员函数 current 获得一个RendererWebKitPlatformSupportImpl 对象。这个 RendererWebKitPlatformSupportImpl 对象定义在Content 模块中，它实现了由 WebKit 定义的 Platform 接口，用来向 WebKit 层提供平台相关的实现。

​		通过调用 RendererWebKitPlatformSupportImpl 类的成员函数 compositorSupport 可以获得一个WebCompositorSupportImpl 对象。有了这个 WebCompositorSupportImpl 对象之后，就可以调用它的成员函数 createContentLayer 创建一个 WebContentLayerImpl 对象，并且保存在GraphicsLayer类的成员变量 m_layer 中。

​		WebCompositorSupportImpl 类的成员函数 createContentLayer 的实现如下所示：

```c++
WebContentLayer* WebCompositorSupportImpl::createContentLayer(
    WebContentLayerClient* client) {
  return new WebContentLayerImpl(client);
}
```

​		从这里可以看到，WebCompositorSupportImpl 类的成员函数 createContentLayer 创建了一个WebContentLayerImpl 对象返回给调用者。

​		WebContentLayerImpl 对象的创建过程，即 WebContentLayerImpl 类的构造函数的实现，如下所示：

```c++
WebContentLayerImpl::WebContentLayerImpl(blink::WebContentLayerClient* client)
    : client_(client), ...... {
  if (WebLayerImpl::UsingPictureLayer())
    layer_ = make_scoped_ptr(new WebLayerImpl(PictureLayer::Create(this)));
  else
    layer_ = make_scoped_ptr(new WebLayerImpl(ContentLayer::Create(this)));
  ......
}
```

​		从前面的调用过程可以知道，参数 client 指向的实际上是一个 OpaqueRectTrackingContentLayerDelegate 对象，WebContentLayerImpl 类的构造函数首先将它保存在成员变量 client_ 中 。

​		WebContentLayerImpl 类的构造函数接下来调用 WebLayerImpl 类的静态成员函数 UsingPictureLayer 判断Render 进程是否启用 Impl Side Painting 特性。如果启用的话，就会调用 PictureLayer 类的静态成员函数 Create 创建一个 Picture Layer；否则的话，就会调用 ContentLayer 类的静态成员函数 Create 创建一个 Content Layer。有了 Picture Layer 或者 Content Layer 之后，再创建一个 WebLayerImpl 对象，保存在WebContentLayerImpl 类的成员变量 layer_ 中。

​		当 Render 进程设置了 enable-impl-side-painting 启动选项时，就会启用 Impl Side Painting 特性，也就是会在 Render 进程中创建一个 Compositor 线程，与 Render 进程中的 Main 线程一起协作完成网页的渲染。在这种情况下，Graphics Layer 在绘制网页内容的时候，实际上只是记录了绘制命令。这些绘制命令就记录在对应的Picture Layer 中。

​		另一方面，如果 Render 进程没有设置 enable-impl-side-painting 启动选项，那么 Graphics Layer 在绘制网页内容的时候，就会通过 Content Layer 提供的一个 Canvas 真正地把网页内容对应的 UI 绘制在一个内存缓冲区中。

​		无论是 Picture Layer 还是Content Layer，它们都是在 cc::Layer 类继承下来的，也就是说，它们对应于图2所示的 CC Layer。不过，我们只考虑 Picture Layer 的情况，因此接下来我们继续分析 Picture Layer 的创建过程，也就是 PictureLayer 类的静态成员函数 Create 的实现，如下所示：

```c++
scoped_refptr<PictureLayer> PictureLayer::Create(ContentLayerClient* client) {
  return make_scoped_refptr(new PictureLayer(client));
}
```

​		从这里可以看到，PictureLayer 类的静态成员函数 Create 创建了一个 PictureLayer 对象返回给调用者。

​		PictureLayer 对象的创建过程，也就是 PictureLayer 类的构造函数的实现，如下所示：

```c++
PictureLayer::PictureLayer(ContentLayerClient* client)
    : client_(client),
      pile_(make_scoped_refptr(new PicturePile())),
      ...... {
}
```

​		从前面的调用过程可以知道，参数 client 指向的实际上是一个 WebContentLayerImpl 对象，PictureLayer 类的构造函数将它保存在成员变量 client_ 中。

​		PictureLayer 类的构造函数还做了另外一件重要的事情，就是创建了一个 PicturePile 对象，并且保存在成员变量 pile_ 中。这个 PicturePile 对象是用来将 Graphics Layer 的绘制命令记录在 Pictrue Layer 中的，后面我们分析网页内容的绘制过程时就会看到这一点。

​		回到 WebContentLayerImpl 类的构造函数中，它创建了一个 Pictrue Layer 之后，接下来就会以这个 Pictrue Layer 为参数，创建一个 WebLayerImpl 对象，如下所示：

```c++
WebLayerImpl::WebLayerImpl(scoped_refptr<Layer> layer) : layer_(layer) {
  ......
}
```

​		WebLayerImpl 类的构造函数主要就是将参数 layer 描述的一个 PictrueLayer 对象保存在成员变量 layer_ 中。

​		从前面Chromium网页Graphics Layer Tree创建过程分析一文还可以知道，Graphics Layer 与 Graphics Layer 是通过 GraphicsLayer 类的成员函数 addChild 形成父子关系的（从而形成 Graphics Layer Tree），如下所示：

```c++
void GraphicsLayer::addChild(GraphicsLayer* childLayer)
{
    addChildInternal(childLayer);
    updateChildList();
}
```

​		GraphicsLayer 类的成员函数 addChild 首先调用成员函数 addChildInternal 将参数 childLayer 描述的一个Graphics Layer 作为当前正在处理的 Graphics Layer 的 子Graphics Layer，如下所示：

```c++
void GraphicsLayer::addChildInternal(GraphicsLayer* childLayer)
{
    ......
 
    childLayer->setParent(this);
    m_children.append(childLayer);
 
    ......
}
```

​		这一步执行完成后，Graphics Layer 之间就建立了父子关系。回到 GraphicsLayer 类的成员函数 addChild中，它接下来还会调用另外一个成员函数 updateChildList，用来在 CC Layer 之间建立父子关系，从而形 CC Layer Tree。

​		GraphicsLayer 类的成员函数 updateChildList 的实现如下所示：

```c++
void GraphicsLayer::updateChildList()
{
    WebLayer* childHost = m_layer->layer();
    ......
 
    for (size_t i = 0; i < m_children.size(); ++i)
        childHost->addChild(m_children[i]->platformLayer());
 
    ......
}
```

​		从前面的分析可以知道，GraphicsLayer 类的成员变量 m_layer 指向的是一个 WebContentLayerImpl 对象，调用这个 WebContentLayerImpl 对象的成员函数 layer 获得的是一个 WebLayerImpl 对象，如下所示：

```c++
blink::WebLayer* WebContentLayerImpl::layer() {
  return layer_.get();
}
```

​		从前面的分析可以知道，WebContentLayerImpl 类的成员变量 layer_ 指向的是一个 WebLayerImpl 对象，因此 WebContentLayerImpl 类的成员函数 layer 返回的是一个 WebLayerImpl 对象。

​		回到 GraphicsLayer 类的成员函数 updateChildList 中，它接下来调用 GraphicsLayer 类的成员函数platformLayer 获得当前正在处理的 Graphics Layer 的所有子 Graphics Layer 对应的 WebLayerImpl 对象，如下所示：

```c++
WebLayer* GraphicsLayer::platformLayer() const
{
    return m_layer->layer();
}
```

​		这些子 Graphics Layer 对应的 WebLayerImpl 对象也就是通过调用它们的成员变量 m_layer 指向的WebContentLayerImpl 对象的成员函数 layer 获得的。

​		再回到 GraphicsLayer 类的成员函数 updateChildList 中，获得当前正在处理的 Graphics Layer 对应的WebLayerImpl 对象，以及其所有的子 Graphics Layer 对应的 WebLayerImpl 对象之后，就可以通过调用WebLayerImpl 类的成员函数 addChild 在它们之间也建立父子关系，如下所示：

```c++
void WebLayerImpl::addChild(WebLayer* child) {
  layer_->AddChild(static_cast<WebLayerImpl*>(child)->layer());
}
```

​		从前面的分析可以知道，WebLayerImpl 类的成员变量 layer_ 指向的是一个 PictrueLayer 对象，因此WebLayerImpl 类的成员函数 addChild 所做的事情就是在两个 PictrueLayer 对象之间建立父子关系，这是通过调用 PictrueLayer 类的成员函数 AddChild 实现的。

​		PictrueLayer 类的成员函数 AddChild 是父类 Layer 继承下来的，它的实现如下所示：

```c++
void Layer::AddChild(scoped_refptr<Layer> child) {
  InsertChild(child, children_.size());
}
```

​		Layer 类的成员函数 AddChild 将参数 child 描述的 Pictrue Layer 设置为当前正在处理的 Picture Layer 的子Picture Layer，这是通过调用 Layer 类的成员函数 InsertChild 实现的，如下所示：

```c++
void Layer::InsertChild(scoped_refptr<Layer> child, size_t index) {
  DCHECK(IsPropertyChangeAllowed());
  child->RemoveFromParent();
  child->SetParent(this);
  child->stacking_order_changed_ = true;
 
  index = std::min(index, children_.size());
  children_.insert(children_.begin() + index, child);
  SetNeedsFullTreeSync();
}
```

​		Layer 类的成员函数I nsertChild 所做的第一件事情是将当前正在处理的 Picture Layer 设置为参数 child 描述的 Pictrue Layer 的父 Picture Layer，并且将参数 child 描述的 Pictrue Layer 保存在当前正在处理的 Picture Layer 的子 Picture Layer 列表中。

​		Layer 类的成员函数 InsertChild 所做的第二件事情是调用另外一个成员函数 SetNeedsFullTreeSync 发出一个通知，要在 CC Layer Tree 与 CC Pending Layer Tree 之间做一个 Tree 结构同步。

​		Layer 类的成员函数 SetNeedsFullTreeSync 的实现如下所示：

```c++
void Layer::SetNeedsFullTreeSync() {
  if (!layer_tree_host_)
    return;
 
  layer_tree_host_->SetNeedsFullTreeSync();
}
```

​		**Layer 类的成员变量 layer_tree_host_ 指向的是一个 LayerTreeHost 对象，这个 LayerTreeHost 是用来管理CC Layer Tree 的**，后面我们再分析它的创建过程。Layer 类的成员函数 SetNeedsFullTreeSync 所做的事情就是调用这个 LayerTreeHost 对象的成员函数 SetNeedsFullTreeSync 通知它 CC Layer Tree 结构发生了变化，需要将这个变化同步到 CC Pending Layer Tree 中去。

​		LayerTreeHost 类的成员函数 SetNeedsFullTreeSync 的实现如下所示：

```c++
void LayerTreeHost::SetNeedsFullTreeSync() {
  needs_full_tree_sync_ = true;
  SetNeedsCommit();
}
```

​		LayerTreeHost 类的成员函数 SetNeedsFullTreeSync 将成员变量 needs_full_tree_sync_ 设置为 true，以标记要在 CC Layer Tree 和 CC Pending Layer Tree 之间做一次结构同步，然后再调用另外一个成员函数 SetNeedsCommit 请求在前面 Chromium 网页渲染机制简要介绍和学习计划一文中提到的调度器将 CC Layer Tree 同步到 CC Pending Tree 去。至于这个同步操作什么时候会执行，就是由调度器根据其内部的状态机决定了。这一点我们在后面的文章再分析。

​		这一步执行完成之后，就可以在 CC 模块中得到一个 Layer Tree，这个 Layer Tree 与 WebKit 中的 Graphics Layer Tree 在结构上是完全同步的，并且这个同步过程是由 WebKit 控制的。这个同步过程之所以要由 WebKit 控制，是因为 CC Layer Tree 是根据 Graphics Layer Tree 创建的，而 Graphics Layer Tree 又是由 WebKit 管理的。

​		WebKit 现在还需要做的另外一件重要的事情是告诉 CC 模块，哪一个 Picture Layer 是 CC Layer Tree 的根节点，这样 CC 模块才可以对整个 CC Layer Tree 进行管理。很显然，Graphics Layer Tree 的根节点对应的 Picture Layer，就是 CC Layer Tree 的根节点。因此，WebKit 会在创建 Graphics Layer Tree 的根节点的时候，将该根节点对应的 Picture Layer 设置到 CC 模块中去，以便后者将其作为 CC Layer Tree 的根节点。

​		Graphics Layer Tree 的根节点是什么时候创建的呢？从前面 Chromium 网页加载过程简要介绍和学习计划这个系列的文章可以知道，Graphics Layer Tree 的根节点对应于 Render Layer Tree 的根节点，Render Layer Tree 的根节点又对应于 Render Object Tree 的根节点，因此我们就从 Render Object Tree 的根节点的创建过程开始，分析 Graphics Layer Tree 的根节点的创建过程。

​		从前面Chromium网页DOM Tree创建过程分析一文可以知道，Render Object Tree 的根节点是在 Document 类的成员函数 attach 中创建的，如下所示：

```c++
void Document::attach(const AttachContext& context)
{
    ......
 
    m_renderView = new RenderView(this);
    ......
 
    m_renderView->setStyle(StyleResolver::styleForDocument(*this));
 
    ......
}
```



​		Document 类的成员函数 attach 首先创建了一个 RenderView 对象，保存在成员变量 m_renderView 中。这个 RenderView 对象就是 Render Object Tree 的根节点。Document 类的成员函数 attach 接下来还会调用RenderView 类的成员函数 setStyle 给前面创建的 RenderView 对象设置 CSS 属性。

​		从前面Chromium网页Render Layer Tree创建过程分析一文可以知道，在给 Render Object Tree 的节点设置 CSS 属性的过程中，会创建相应的 Render Layer。这一步发生在 RenderLayerModelObject 类的成员函数styleDidChange 中，如下所示：

```c++
void RenderLayerModelObject::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)  
{  
    ......  
  
    LayerType type = layerTypeRequired();  
    if (type != NoLayer) {  
        if (!layer() && layerCreationAllowedForSubtree()) {  
            ......  
  
            createLayer(type);  
            
            ......  
        }  
    } else if (layer() && layer()->parent()) {  
        ......  
  
        layer()->removeOnlyThisLayer(); // calls destroyLayer() which clears m_layer  
  
        ......  
    }  
  
    if (layer()) {  
        ......  
  
        layer()->styleChanged(diff, oldStyle);  
    }  
  
    ......  
}  
```

​		RenderLayerModelObject 类的成员函数 styleDidChange 的详细分析可以参考Chromium网页Render Layer Tree创建过程分析一文。其中，Render Layer 的创建是通过调用 RenderLayerModelObject 类的成员函数createLayer 实现的，并且创建出来的 Render Layer 的成员函数 styleChanged 会被调用，用来设置它的 CSS 属性。

​		在设置 Render Layer Tree 的根节点的 CSS 属性的过程中，会触发 Graphics Layer Tree 的根节点的创建，因此接下来我们继续分析 RenderLayer 类的成员函数 styleChanged 的实现，如下所示：

```c++
void RenderLayer::styleChanged(StyleDifference diff, const RenderStyle* oldStyle)
{
    ......
 
    m_stackingNode->updateStackingNodesAfterStyleChange(oldStyle);
 
    ......
}
```

​		RenderLayer 类的成员变量 m_stackingNode 指向的是一个 RenderLayerStackingNode 对象。这个RenderLayerStackingNode 对象描述的是一个 Stacking Context。关于 Stacking Context，可以参考前面Chromium网页Graphics Layer Tree创建过程分析一文。RenderLayer 类的成员函数 styleChanged 调用上述RenderLayerStackingNode 对象的成员函数 updateStackingNodesAfterStyleChange 通知它所关联的 Render Layer 的 CSS 属性发生了变化，这样它可能就需要更新自己的子元素。

​		RenderLayerStackingNode 类的成员函数 updateStackingNodesAfterStyleChange 的实现如下所示：

```c++
void RenderLayerStackingNode::updateStackingNodesAfterStyleChange(const RenderStyle* oldStyle)
{
    bool wasStackingContext = oldStyle ? !oldStyle->hasAutoZIndex() : false;
    int oldZIndex = oldStyle ? oldStyle->zIndex() : 0;
 
    bool isStackingContext = this->isStackingContext();
    if (isStackingContext == wasStackingContext && oldZIndex == zIndex())
        return;
 
    dirtyStackingContextZOrderLists();
 
    if (isStackingContext)
        dirtyZOrderLists();
    else
        clearZOrderLists();
}
```

​		RenderLayerStackingNode 类的成员函数 updateStackingNodesAfterStyleChange 判断当前正在处理的RenderLayerStackingNode 对象关联的 Render Layer 的 CSS 属性变化，是否导致它从一个 Stacking Context 变为一个非 Stacking Context，或者从一个非 Stacking Context 变为一个 Stacking Context。

​		在从非 Stacking Context 变为 Stacking Context 的情况下，RenderLayerStackingNode 类的成员函数updateStackingNodesAfterStyleChange 就会调用另外一个成员函数 dirtyZOrderLists 将 Stacking Context 标记为 Dirty 状态，这样以后在需要的时候就会根据该 Stacking Context 的子元素的 z-index 重新构建 Graphics Layer Tree。

​		RenderLayerStackingNode 类的成员函数 dirtyZOrderLists 的实现如下所示：

```c++
void RenderLayerStackingNode::dirtyZOrderLists()
{
    ......
 
    if (m_posZOrderList)
        m_posZOrderList->clear();
    if (m_negZOrderList)
        m_negZOrderList->clear();
    m_zOrderListsDirty = true;
 
    if (!renderer()->documentBeingDestroyed())
        compositor()->setNeedsCompositingUpdate(CompositingUpdateRebuildTree);
}
```

​		RenderLayerStackingNode 类的成员函数 dirtyZOrderLists 首先是将用来保存子元素的两个列表清空。其中一个列表用来保存 z-index 为正数的子元素，另一个列表用来保存 z-index 为负数的子元素。这些子元素在各自的列表中均是按照从小到大的顺序排列的。有了这个顺序之后，Graphics Layer Tree 就可以方便地按照 z-index 顺序创建出来。

​		RenderLayerStackingNode 类的成员函数 dirtyZOrderLists 接下来将成员变量 m_zOrderListsDirty 的值设置为 true，就将自己的状态标记为 Dirty，以后就会重新根据子元素的 z-index 值，将它们分别保存在对应的列表中。

​		RenderLayerStackingNode 类的成员函数 dirtyZOrderLists 最后判断当前加载的网页有没有被销毁。如果没有被销毁，就会调用另外一个成员函数 compositor，获得一个 RenderLayerCompositor 对象。这个RenderLayerCompositor 对象是用来管理当前加载的网页的 Graphics Layer Tree 的。有了这个RenderLayerCompositor 对象之后，就可以调用它的成员函数 setNeedsCompositingUpdate，用来通知它需要重建 Graphics Layer Tree。

​		RenderLayerCompositor 类的成员函数 setNeedsCompositingUpdate 的实现如下所示：

```c++
void RenderLayerCompositor::setNeedsCompositingUpdate(CompositingUpdateType updateType)
{
    ......
    if (!m_renderView.needsLayout())
        enableCompositingModeIfNeeded();
 
    m_pendingUpdateType = std::max(m_pendingUpdateType, updateType);
    ......
}
```

​		RenderLayerCompositor 类的成员变量 m_renderView 描述的是一个 RenderView 对象。这个 RenderView对象就是在前面分析的 Document 类的成员函数 attach 中创建的 RenderView 对象。RenderLayerCompositor类的成员函数 setNeedsCompositingUpdate 判断它是否需要重新 Layout。如果需要的话，就会调用另外一个成员函数 enableCompositingModeIfNeeded 将网页的 Render Layer Tree 的根节点设置为一个 Compositing Layer，也就是要为它创建一个 Graphics Layer。

​		在我们这个情景中，RenderLayerCompositor 类的成员变量 m_renderView 描述的 RenderView 对象是刚刚创建的，这意味它需要执行一个 Layout 操作，因此接下来 RenderLayerCompositor 类的成员函数setNeedsCompositingUpdate 会调用成员函数 enableCompositingModeIfNeeded 为 Render Layer Tree 的根节点创建一个 Graphics Layer，作为 Graphics Layer Tree 的根节点。

​		RenderLayerCompositor 类的成员函数 enableCompositingModeIfNeeded 的实现如下所示：

```c++
void RenderLayerCompositor::enableCompositingModeIfNeeded()
{
    ......
 
    if (rootShouldAlwaysComposite()) {
        ......
        setCompositingModeEnabled(true);
    }
}
```

​		RenderLayerCompositor 类的成员函数 enableCompositingModeIfNeeded 首先调用成员函数rootShouldAlwaysComposite 判断是否要为网页 Render Layer Tree 的根节点创建一个 Graphics Layer。如果需要的话，就调用另外一个成员函数 setCompositingModeEnabled 进行创建。

​		RenderLayerCompositor 类的成员函数 rootShouldAlwaysComposite 的实现如下所示：

```c++
bool RenderLayerCompositor::rootShouldAlwaysComposite() const
{
    if (!m_hasAcceleratedCompositing)
        return false;
    return m_renderView.frame()->isMainFrame() || m_compositingReasonFinder.requiresCompositingForScrollableFrame();
}
```

​		只有在采用硬件加速渲染网页的情况下，才需要创建 Graphics Layer。当 RenderLayerCompositor 类的成员变量 m_hasAcceleratedCompositing 的值等于 true 的时候，就表示描述网页采用硬件加速渲染。因此，当RenderLayerCompositor 类的成员变量 m_hasAcceleratedCompositing 的值等于 false 的时候，RenderLayerCompositor 类的成员函数就返回一个 false 值给调用者，表示不需要为网页 Render Layer Tree 的根节点创建 Graphics Layer。

​		在采用硬件加速渲染网页的情况下，在两种情况下，需要为 Render Layer Tree 的根节点创建 Graphics Layer。第一种情况是当前网页加载在 Main Frame 中。第二种情况是当前网页不是加载在 Main Frame，例如是通过 iframe 嵌入在 Main Frame 中，但是它是可滚动的。

​		我们假设当前网页是加载在 Main Frame 中的，因此 RenderLayerCompositor 类的成员函数rootShouldAlwaysComposite 的返回值为 true，这时候 RenderLayerCompositor 类的成员函数enableCompositingModeIfNeeded 就会调用另外一个成员函数 setCompositingModeEnabled 为网页 Render Layer Tree 的根节点创建 Graphics Layer。

​		RenderLayerCompositor 类的成员函数 setCompositingModeEnabled 的实现如下所示：

```c++
void RenderLayerCompositor::setCompositingModeEnabled(bool enable)
{
    ......
 
    m_compositing = enable;
 
    ......
 
    if (m_compositing)
        ensureRootLayer();
    else
        destroyRootLayer();
 
    ......
}
```

​		从前面的调用过程可以知道，参数 enable 的值等于 true，这时候 RenderLayerCompositor 类的成员函数setCompositingModeEnabled 会调用另外一个成员函数 ensureRootLayer 创建 Graphics Layer Tree 的根节点。

​		RenderLayerCompositor 类的成员函数 ensureRootLayer 的实现如下所示：

```c++
void RenderLayerCompositor::ensureRootLayer()  
{  
    RootLayerAttachment expectedAttachment = m_renderView.frame()->isMainFrame() ? RootLayerAttachedViaChromeClient : RootLayerAttachedViaEnclosingFrame;
    ......  
  
    if (!m_rootContentLayer) {  
        m_rootContentLayer = GraphicsLayer::create(graphicsLayerFactory(), this);  
        ......  
    }  
  
    if (!m_overflowControlsHostLayer) {  
        ......  
  
        // Create a layer to host the clipping layer and the overflow controls layers.  
        m_overflowControlsHostLayer = GraphicsLayer::create(graphicsLayerFactory(), this);  
  
        // Create a clipping layer if this is an iframe or settings require to clip.  
        m_containerLayer = GraphicsLayer::create(graphicsLayerFactory(), this);  
        ......  
  
        m_scrollLayer = GraphicsLayer::create(graphicsLayerFactory(), this);  
        ......  
        // Hook them up  
        m_overflowControlsHostLayer->addChild(m_containerLayer.get());  
        m_containerLayer->addChild(m_scrollLayer.get());  
        m_scrollLayer->addChild(m_rootContentLayer.get());  
  
        ......  
    }  
  
    ......  
 
    attachRootLayer(expectedAttachment);
}  
```

​		RenderLayerCompositor 类的成员函数 ensureRootLayer 的详细分析可以参考前面Chromium网页Graphics Layer Tree创建过程分析一文，现在我们关注的重点是它最后调用另外一个成员函数 attachRootLayer 将 Graphics Layer Tree 的根节点设置给 WebKit 的使用者，即 Chromium 的 Content 层。

​		RenderLayerCompositor 类的成员函数 attachRootLayer 的实现如下所示：

```c++
void RenderLayerCompositor::attachRootLayer(RootLayerAttachment attachment)
{
    ......
 
    switch (attachment) {
        ......
        case RootLayerAttachedViaChromeClient: {
            LocalFrame& frame = m_renderView.frameView()->frame();
            Page* page = frame.page();
            if (!page)
                return;
            page->chrome().client().attachRootGraphicsLayer(rootGraphicsLayer());
            break;
        }
        ......
    }
 
    m_rootLayerAttachment = attachment;
}
```

​		从前面的调用过程可以知道，如果当前网页是在 Main Frame 中加载的，那么参数 attachment 的值就等于RootLayerAttachedViaChromeClient，这时候 RenderLayerCompositor 类的成员函数 attachRootLayer 与当前加载网页关联的一个 ChromeClientImpl 对象，并且调用这个 ChromeClientImpl 对象的成员函数attachRootGraphicsLayer 将 Graphics Layer Tree 的根节点传递给它处理。Graphics Layer Tree 的根节点可以通过调用 RenderLayerCompositor 类的成员函数 rootGraphicsLayer 获得。

​		ChromeClientImpl 类的成员函数 attachRootGraphicsLayer 的实现如下所示：

```c++
void ChromeClientImpl::attachRootGraphicsLayer(GraphicsLayer* rootLayer)
{
    m_webView->setRootGraphicsLayer(rootLayer);
}
```

​		ChromeClientImpl 类的成员变量 m_webView 指向的是一个 WebViewImpl 对象。这个 WebViewImpl 对象的创建过程可以参考前面Chromium网页Frame Tree创建过程分析一文。ChromeClientImpl 类的成员函数attachRootGraphicsLayer 所做的事情就是调用这个 WebViewImpl 对象的成员函数 setRootGraphicsLayer，以便将 Graphics Layer Tree 的根节点传递给它处理。

​		WebViewImpl 类的成员函数 setRootGraphicsLayer 的实现如下所示：

```c++
void WebViewImpl::setRootGraphicsLayer(GraphicsLayer* layer)
{
    if (pinchVirtualViewportEnabled()) {
        PinchViewport& pinchViewport = page()->frameHost().pinchViewport();
        pinchViewport.attachToLayerTree(layer, graphicsLayerFactory());
        if (layer) {
            m_rootGraphicsLayer = pinchViewport.rootGraphicsLayer();
            m_rootLayer = pinchViewport.rootGraphicsLayer()->platformLayer();
            ......
        } 
        ......
    } else {
        m_rootGraphicsLayer = layer;
        m_rootLayer = layer ? layer->platformLayer() : 0;
        ......
    }
 
    setIsAcceleratedCompositingActive(layer);
    
    ......
}
```

​		如果浏览器设置了 " enable-pinch-virtual-viewport " 启动选项，调用 WebViewImpl 类的成员函数pinchVirtualViewportEnabled 得到的返回值就会为 true。这时候网页有两个 Viewport，一个称为 Inner Viewport，另一个称为 Outer Viewport。Outer Viewport 有两个特性。第一个特性是它的大小不跟随页面进行缩放，第二个特性是 fixed positioned 元素的位置是根据它来计算的。这种体验的特点是 fixed positioned 元素的位置不会随页面的缩放发生变化。为实现这种体验，需要在 Graphics Layer Tree 中增加一些 Graphics Layer。这些 Graphics Layer 通过一个 PinchViewport 管理。这时候 Graphics Layer Tree 的根节点就不再是参数 layer 描述的 Graphics Layer，而是 PinchViewport 额外创建的一个 Graphics Layer。关于 Pinch Virtual Viewport 特性的更多信息，可以参考官方文档：Layer-based Solution for Pinch Zoom / Fixed Position。

​		为简单起见，我们假设没有设置 " enable-pinch-virtual-viewpor t" 启动选项，这时候 WebViewImpl 类的成员函数 setRootGraphicsLayer 会将参数 layer 指向的一个 Graphics Layer，也就是 Graphics Layer Tree 的根节点，保存在成员变量 m_rootGraphicsLayer 中，并且调用它的成员函数 platformLayer 获得与它关联的一个WebLayerImpl 对象，保存在另外一个成员变量 m_rootLayer 中。

​		再接下来，WebViewImpl 类的成员函数 setRootGraphicsLayer 调用另外一个成员函数setIsAcceleratedCompositingActive 激活网页的硬件加速渲染，它的实现如下所示：

```c++
void WebViewImpl::setIsAcceleratedCompositingActive(bool active)
{
    ......
 
    if (!active) {
        m_isAcceleratedCompositingActive = false;
        ......
    } else if (m_layerTreeView) {
        m_isAcceleratedCompositingActive = true;
        ......
    } else {
        ......
 
        m_client->initializeLayerTreeView();
        m_layerTreeView = m_client->layerTreeView();
        if (m_layerTreeView) {
            m_layerTreeView->setRootLayer(*m_rootLayer);
            ......
        }
 
        ......
 
        m_isAcceleratedCompositingActive = true;
 
        ......
    }
 
    ......
}
```

​		从前面的调用过程可以知道，参数 active 的值是等于 true 的，WebViewImpl 类的成员变量m_isAcceleratedCompositingActive 的值将被设置为参数 active 的值，用来表示网页是否已经激活网页硬件加速渲染。

​		WebViewImpl 类还有两个重要的成员变量 m_client 和 m_layerTreeView。其中，成员变量 m_client 的初始化过程可以参考前面 Chromium网页Frame Tree创建过程分析一文，它指向的是一个在 Chromium 的 Content 层创建的 RenderViewImpl 对象。

​		另外一个成员变量 m_layerTreeView 是一个类型为 WebLayerTreeView 指针，它的值开始的时候是等于 NULL 的。WebViewImpl 类的成员函数 setIsAcceleratedCompositingActive 被调用的时候，如果参数 active 的值是等于 true，并且成员变量 m_layerTreeView 的值也等于 NULL，那么 WebKit 就会先请求使用者，也就是Chromium 的 Content 层，初始化 CC Layer Tree。这是通过调用成员变量 m_client 指向的一个RenderViewImpl 对象的成员函数 initializeLayerTreeView 实现的。Layer Tree View 初始化完成之后，WebViewImpl 类再将成员变量 m_rootLayer 描述的 WebLayerImpl 对象关联的 Picture Layer 设置为 CC Layer Tree 的根节点。

​		接下来我们先分析 RenderViewImpl 类的成员函数 initializeLayerTreeView 初始化 CC Layer Tree 过程，然后再分析设置 CC Layer Tree 根节点的过程。

​		RenderViewImpl 类的成员函数 initializeLayerTreeView 的实现如下所示：

```c++
void RenderViewImpl::initializeLayerTreeView() {
  RenderWidget::initializeLayerTreeView();
  ......
}
```

​		RenderViewImpl 类的成员函数 initializeLayerTreeView 主要是调用父类 RenderWidget 的成员函数initializeLayerTreeView 初始化一个 CC Layer Tree，如下所示：

```c++
void RenderWidget::initializeLayerTreeView() {
  compositor_ = RenderWidgetCompositor::Create(
      this, is_threaded_compositing_enabled_);
  ......
  if (init_complete_)
    StartCompositor();
}
```

​		RenderWidget 类的成员函数 initializeLayerTreeView 首先是调用 RenderWidgetCompositor 类的静态成员函数 Create 创建一个 RenderWidgetCompositor 对象，并且保存在成员变量 compositor_ 中。在创建这个RenderWidgetCompositor 对象期间，也会伴随着创建一个 CC Layer Tree。

​		RenderWidget 类有一个成员变量 init_complete_，当它的值等于 true 的时候，表示 Browser 进程已经为当前正在加载的网页初始化好 Render View，这时候 RenderWidget 类的成员函数 initializeLayerTreeView 就会调用另外一个成员函数 StartCompositor 激活前面Chromium网页加载过程简要介绍和学习计划一文中提到的调度器，表示它可以开始进行调度工作了。

​		接下来我们先分析 RenderWidget 类的成员变量 init_complete_ 被设置为 true 的过程。从前面Chromium网页Frame Tree创建过程分析一文可以知道，当 Browser 进程为在 Render 进程中加载的网页创建了一个 Render View 之后，会向 Render 进程发送一个类型为 ViewMsg_New 的消息。这个 IPC 消息被 RenderThreadImpl 类的成员函数 OnCreateNewView 处理。在处理期间，会创建一个 RenderViewImpl 对象，并且调用它的成员函数Initialize 对其进行初始化，如下所示：

```c++
void RenderViewImpl::Initialize(RenderViewImplParams* params) {  
  ......  
  
  main_render_frame_.reset(RenderFrameImpl::Create(  
      this, params->main_frame_routing_id));  
  ......  
  
  WebLocalFrame* web_frame = WebLocalFrame::create(main_render_frame_.get());  
  main_render_frame_->SetWebFrame(web_frame);  
  ......  
  
  webwidget_ = WebView::create(this);  
  ......  
 
  // If this is a popup, we must wait for the CreatingNew_ACK message before
  // completing initialization.  Otherwise, we can finish it now.
  if (opener_id_ == MSG_ROUTING_NONE) {
    ......
    CompleteInit();
  }
  
  webview()->setMainFrame(main_render_frame_->GetWebFrame());  
    
  ......  
}
```

​		RenderViewImpl 类的成员函数 Initialize 的详细分析可以参考前面Chromium网页Frame Tree创建过程分析一文。这里我们看到，当 RenderViewImpl 类的成员变量 opener_id_ 的值等于 MSG_ROUTING_NONE 的时候，另外一个成员函数 CompleteInit 就会被调用。RenderViewImpl 类的成员变量 opener_id_ 什么时候会等于MSG_ROUTING_NONE 呢？如果正在加载的网页不是在一个 Popup Window 显示时，它的值就会等于MSG_ROUTING_NONE ，否则它的值等于将它 Popup 出来的网页的 Routing ID。从代码注释我们还可以看到，如果当前加载的网页是在一个 Popup Window 显示时，RenderViewImpl 类的成员函数 CompleteInit 将会延迟到 Render 进程接收到 Broswer 发送另外一个类型为 ViewMsg_CreatingNew_ACK 的 IPC 消息时才会被调用。

​		我们假设正在加载的网页不是一个 Popup Window 显示，这时候 RenderViewImpl 类的成员函数CompleteInit 就会被调用。RenderViewImpl 类的成员函数 CompleteInit 是从父类 RenderWidget 继承下来的，它的实现如下所示：

```c++
void RenderWidget::CompleteInit() {
  ......
 
  init_complete_ = true;
 
  if (compositor_)
    StartCompositor();
 
  ......
}
```

​		从这里就可以看到，RenderWidget 类的成员变量 init_complete_ 将会被设置为 true，并且在成员变量compositor_ 的值不等于 NULL 的情况下，会调用前面提到的成员函数 StartCompositor 激活前面 Chromium网页加载过程简要介绍和学习计划一文中提到的调度器。

​		回到前面分析的 RenderWidget 类的成员函数 initializeLayerTreeView 中，我们假设正在加载的网页不是在一个 Popup Window 显示，因此当 RenderWidget 类的成员函数 initializeLayerTreeView 被调用时，Browser 进程已经为正在加载的网页初始化好了 Render View，这意味着此时 RenderWidget 类的成员变量 init_complete_ 已经被设置为 true，于是 RenderWidget 类的成员函数 initializeLayerTreeView 就会先调用RenderWidgetCompositor 类的静态成员函数 Create 创建一个 RenderWidgetCompositor 对象，然后再调用另外一个成员函数 StartCompositor 激活前面Chromium网页加载过程简要介绍和学习计划一文中提到的调度器。接下来我们就先分析 RenderWidgetCompositor 类的静态成员函数 Create 的实现，在接下来一篇文章中再分析RenderWidget 类的成员函数 StartCompositor 激活调度器的过程。

​		RenderWidgetCompositor 类的静态成员函数 Create 的实现如下所示：

```c++
scoped_ptr<RenderWidgetCompositor> RenderWidgetCompositor::Create(
    RenderWidget* widget,
    bool threaded) {
  scoped_ptr<RenderWidgetCompositor> compositor(
      new RenderWidgetCompositor(widget, threaded));
 
  CommandLine* cmd = CommandLine::ForCurrentProcess();
 
  cc::LayerTreeSettings settings;
  ......
 
  settings.initial_debug_state.show_debug_borders =
      cmd->HasSwitch(cc::switches::kShowCompositedLayerBorders);
  settings.initial_debug_state.show_fps_counter =
      cmd->HasSwitch(cc::switches::kShowFPSCounter);
  settings.initial_debug_state.show_layer_animation_bounds_rects =
      cmd->HasSwitch(cc::switches::kShowLayerAnimationBounds);
  settings.initial_debug_state.show_paint_rects =
      cmd->HasSwitch(switches::kShowPaintRects);
  settings.initial_debug_state.show_property_changed_rects =
      cmd->HasSwitch(cc::switches::kShowPropertyChangedRects);
  settings.initial_debug_state.show_surface_damage_rects =
      cmd->HasSwitch(cc::switches::kShowSurfaceDamageRects);
  settings.initial_debug_state.show_screen_space_rects =
      cmd->HasSwitch(cc::switches::kShowScreenSpaceRects);
  settings.initial_debug_state.show_replica_screen_space_rects =
      cmd->HasSwitch(cc::switches::kShowReplicaScreenSpaceRects);
  settings.initial_debug_state.show_occluding_rects =
      cmd->HasSwitch(cc::switches::kShowOccludingRects);
  settings.initial_debug_state.show_non_occluding_rects =
      cmd->HasSwitch(cc::switches::kShowNonOccludingRects);
  ......
 
  compositor->Initialize(settings);
 
  return compositor.Pass();
}
```

​		RenderWidgetCompositor 类的静态成员函数 Create 首先创建一个 RenderWidgetCompositor 对象，接着根据 Render 进程的启动选项初始化一个 LayerTreeSettings 对象，最后以这个 LayerTreeSettings 对象为参数，对前面创建的 RenderWidgetCompositor 对象进行初始化，这是通过调用 RenderWidgetCompositor 类的成员函数 Initialize 实现的。

​		接下来我们先分析 RenderWidgetCompositor 对象的创建过程，也就是 RenderWidgetCompositor 类的构造函数的实现，接下来再分析 RenderWidgetCompositor 对象的初始化过程，也就是 RenderWidgetCompositor 类的成员函数 Initialize 的实现。

​		RenderWidgetCompositor 类的构造函数的实现如下所示：

```c++
RenderWidgetCompositor::RenderWidgetCompositor(RenderWidget* widget,
                                               bool threaded)
    : threaded_(threaded),
      ......,
      widget_(widget) {
}
```

​		RenderWidgetCompositor 类的构造函数主要是将参数 widget 指向的 RenderViewImpl 对象保存在成员变量 widget_ 中，并且将参数 threaded 的值保存在成员变量 threaded_ 中。参数 threaded 用来描述 Render 进程是否要采用线程化渲染，也就是是否需要创建一个 Compositor 线程来专门执行渲染相关的工作。

​		从前面的调用过程可以知道，参数 threaded 是从 RenderWidget 类的成员函数 initializeLayerTreeView 中传递过来的，它的值等于 RenderWidget 类的成员变量 is_threaded_compositing_enabled_ 的值。RenderWidget 类的成员变量 is_threaded_compositing_enabled_ 是在构造函数初始化的，如下所示：

```c++
RenderWidget::RenderWidget(blink::WebPopupType popup_type,
                           const blink::WebScreenInfo& screen_info,
                           bool swapped_out,
                           bool hidden,
                           bool never_visible)
    : ...... {
  ......
  is_threaded_compositing_enabled_ =
      CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableThreadedCompositing);
}
```

​		这意味着当 Render 进程设置了 “ enable-threaded-compositing ” 启动选项时，Render 进程就会采用线程化渲染机制。我们接下来以及以后的文章只考虑线程化渲染机制这种情况。

​		回到 RenderWidgetCompositor 类的构造函数中，这意味着它的成员变量 threaded_ 会被设置为 true。

​		接下来我们继续分析 RenderWidgetCompositor 对象的初始化过程，也就是 RenderWidgetCompositor 类的成员函数 Initialize 的实现，如下所示：

```c++
void RenderWidgetCompositor::Initialize(cc::LayerTreeSettings settings) {
  scoped_refptr<base::MessageLoopProxy> compositor_message_loop_proxy;
  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  ......
  // render_thread may be NULL in tests.
  if (render_thread) {
    compositor_message_loop_proxy =
        render_thread->compositor_message_loop_proxy();
    ......
  }
  if (compositor_message_loop_proxy.get()) {
    layer_tree_host_ = cc::LayerTreeHost::CreateThreaded(
        this, shared_bitmap_manager, settings, compositor_message_loop_proxy);
  } else {
    layer_tree_host_ = cc::LayerTreeHost::CreateSingleThreaded(
        this, this, shared_bitmap_manager, settings);
  }
  ......
}
```

​		RenderWidgetCompositor 类的成员函数 Initialize 首先调用 RenderThreadImpl 类的静态成员函数 current获得一个 RenderThreadImpl 对象。这个 RenderThreadImpl 对象描述的实际上是 Render 进程的 Render 线程，也就是 Main 线程。这个 Main 线程是在 Render 进程启动的时候创建的，是一定会存在的，具体可以参考Chromium的Render进程启动过程分析一文。

​		RenderWidgetCompositor 类的成员函数 Initialize 接下来调用前面获得的 RenderThreadImpl 对象的成员函数 compositor_message_loop_proxy 获得 Render 进程的 Compositor 线程的消息循环代理对象。当这个消息循环代理对象存在时，就会调用 cc::LayerTreeHost 类的静态成员函数 CreateThreaded 创建一个支持线程化渲染的 LayerTreeHost 对象，并且保存在成员变量 layer_tree_host_ 中。另一方面，如果 Render 进程中不存在Compositor 线程，那么 RenderWidgetCompositor 类的成员函数 Initialize 就会调用 cc::LayerTreeHost 类的静态成员函数 CreateSingleThreaded 创建一个不支持线程化渲染的 LayerTreeHost 对象。在后一种情况下，所有的渲染操作都将在 Render 进程的 Main 线程中执行。

​		接下来我们首先分析 Render 进程的 Compositor 线程的创建过程。前面提到，当 Browser 进程为 Render 进程中加载的网页创建了一个 Render View 时，就会发送一个类型为 ViewMsg_New 的 IPC 消息给 Render 进程。Render 进程通过 RenderThreadImpl 类的成员函数 OnCreateNewView 接收和处理该消息，如下所示：

```c++
void RenderThreadImpl::OnCreateNewView(const ViewMsg_New_Params& params) {  
  EnsureWebKitInitialized();  
  // When bringing in render_view, also bring in webkit's glue and jsbindings.  
  RenderViewImpl::Create(params.opener_route_id,  
                         params.window_was_created_with_opener,  
                         params.renderer_preferences,  
                         params.web_preferences,  
                         params.view_id,  
                         params.main_frame_routing_id,  
                         params.surface_id,  
                         params.session_storage_namespace_id,  
                         params.frame_name,  
                         false,  
                         params.swapped_out,  
                         params.proxy_routing_id,  
                         params.hidden,  
                         params.never_visible,  
                         params.next_page_id,  
                         params.screen_info,  
                         params.accessibility_mode);  
}  
```

​		除了调用 RenderViewImpl 类的静态成员函数 Create 创建一个 RenderViewImpl 对象，RenderThreadImpl类的成员函数 OnCreateNewView 还会调用另外一个成员函数 EnsureWebKitInitialized 确保 WebKit 已经初始化好。

​		RenderThreadImpl 类的成员函数 EnsureWebKitInitialized 的实现如下所示：

```c++
void RenderThreadImpl::EnsureWebKitInitialized() {
  if (webkit_platform_support_)
    return;
 
  webkit_platform_support_.reset(new RendererWebKitPlatformSupportImpl);
  blink::initialize(webkit_platform_support_.get());
  ......
 
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
 
  bool enable = command_line.HasSwitch(switches::kEnableThreadedCompositing);
  if (enable) {
    ......
 
    if (!compositor_message_loop_proxy_.get()) {
      compositor_thread_.reset(new base::Thread("Compositor"));
      compositor_thread_->Start();
      ......
      compositor_message_loop_proxy_ =
          compositor_thread_->message_loop_proxy();
      ......
    }
 
    ......
  }
 
  ......
}
```

​		除了执行初始化 WebKit 的工作，RenderThreadImpl 类的成员函数 EnsureWebKitInitialized 还做的另外一件重要事情是检查 Render 进程是否设置了 “enable-threaded-compositing” 启动选项。如果设置了，那么就会创建和启动一个 Compositor 线程，并且将这个 Compositor 线程的消息循环代理对象保存在成员变量compositor_message_loop_proxy_ 中。这样当 RenderThreadImpl 类的成员函数compositor_message_loop_proxy 被调用时，Compositor 线程的消息循环代理对象就会被返回给调用者，如下所示：

```c++
class CONTENT_EXPORT RenderThreadImpl : public RenderThread,
                                        public ChildThread,
                                        public GpuChannelHostFactory {
 public:
  ......
 
  scoped_refptr<base::MessageLoopProxy> compositor_message_loop_proxy() const {
    return compositor_message_loop_proxy_;
  }
 
  ......
};
```

​		有了这个消息循环代理对象之后，就可以向 Compositor 线程发送消息请求其执行相应的操作了。

​		回到 RenderWidgetCompositor 类的成员函数 Initialize，它获得了 Compositor 线程的消息循环代理对象之后，接下来就调用 cc::LayerTreeHost 类的静态成员函数 CreateThreaded 创建一个支持线程化渲染的LayerTreeHost 对象，如下所示：

```c++
scoped_ptr<LayerTreeHost> LayerTreeHost::CreateThreaded(
    LayerTreeHostClient* client,
    SharedBitmapManager* manager,
    const LayerTreeSettings& settings,
    scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner) {
  DCHECK(impl_task_runner);
  scoped_ptr<LayerTreeHost> layer_tree_host(
      new LayerTreeHost(client, manager, settings));
  layer_tree_host->InitializeThreaded(impl_task_runner);
  return layer_tree_host.Pass();
}
```

​		LayerTreeHost 类的静态成员函数 CreateThreaded 首先创建了一个 LayerTreeHost 对象，如下所示：

```c++
LayerTreeHost::LayerTreeHost(LayerTreeHostClient* client,
                             SharedBitmapManager* manager,
                             const LayerTreeSettings& settings)
    : ......,
      client_(client),
      ...... {
  ......
}
```

​		从前面的调用过程可以知道，参数 client 指向的是一个 RenderWidgetCompositor 对象，这个RenderWidgetCompositor 对象将会被保存 LayerTreeHost 类的成员变量 client_ 中。

​		回到 LayerTreeHost 类的静态成员函数 CreateThreaded 中，它创建了一个 LayerTreeHost 对象之后，接下来会调用它的成员函数 InitializeThreaded 对其进行初始化，如下所示：

```c++
void LayerTreeHost::InitializeThreaded(
    scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner) {
  InitializeProxy(ThreadProxy::Create(this, impl_task_runner));
}
```

​		从前面的调用过程可以知道，参数 impl_task_runner 描述的是 Compositor 线程的消息循环，LayerTreeHost 类的成员函数 InitializeThreaded 首先调用 ThreadProxy 类的静态成员函数 Create 创建一个ThreadProxy 对象，接着用这个 ThreadProxy 对象初始化当前正在处理的 LayerTreeHost 对象，这是通过调用LayerTreeHost 类的成员函数 IntializeProxy 实现的。

​		接下来我们先分析 ThreadProxy 类的静态成员函数 Create 创建 ThreadProxy 对象的过程，接着再分析LayerTreeHost 类的成员函数 IntializeProxy 初始化 LayerTreeHost 对象的过程。

​	ThreadProxy 类的静态成员函数 Create 的实现如下所示：

```c++
scoped_ptr<Proxy> ThreadProxy::Create(
    LayerTreeHost* layer_tree_host,
    scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner) {
  return make_scoped_ptr(new ThreadProxy(layer_tree_host, impl_task_runner))
      .PassAs<Proxy>();
}
```

​		从这里可以看到，ThreadProxy 类的静态成员函数 Create 创建了一个 ThreadProxy 对象返回给调用者。

​		ThreadProxy 对象的创建过程，即 ThreadProxy 类的构造函数的实现，如下所示：

```c++
ThreadProxy::ThreadProxy(
    LayerTreeHost* layer_tree_host,
    scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner)
    : Proxy(impl_task_runner),
      ...... {
  ......
}
```

​		ThreadProxy 类的构造函数主要是调用了父类 Proxy 的构造函数执行初始化工作，如下所示：

```c++
Proxy::Proxy(scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner)
    : main_task_runner_(base::MessageLoopProxy::current()),
      ......
      impl_task_runner_(impl_task_runner),
      ...... {
  ......
}
```

​		从前面的分析可以知道，参数 impl_task_runner 描述的是 Compositor 线程的消息循环，它将被保存在Proxy 类的成员变量 impl_task_runner_ 中。此外，Proxy 类的构造函数还会通过调用 MessageLoopProxy 类的静态成员函数 current 获得当前线程的消息循环，并且保存在成员变量 main_task_runner_ 。当前线程即为Render 进程的 Main 线程，因此 Proxy 类的成员变量 main_task_runner_ 描述的 Main 线程的消息循环。       

​		初始化好 Proxy 类的成员变量 main_task_runner_ 和 impl_task_runner_ 之后，以后就可以通过调用 Proxy类的成员函数 MainThreadTaskRunner 和 ImplThreadTaskRunner 获得它们描述的线程的消息循环，如下所示：

```c++
base::SingleThreadTaskRunner* Proxy::MainThreadTaskRunner() const {
  return main_task_runner_.get();
}
 
......
 
base::SingleThreadTaskRunner* Proxy::ImplThreadTaskRunner() const {
  return impl_task_runner_.get();
}
```

​		回到 LayerTreeHost 类的成员函数 InitializeThreaded 中，它创建了一个 ThreadProxy 对象之后，接下来就会调用另外一个成员函数 InitializeProxy 启动前面创建的 ThreadProxy 对象，如下所示：

```c++
void LayerTreeHost::InitializeProxy(scoped_ptr<Proxy> proxy) {
  ......
 
  proxy_ = proxy.Pass();
  proxy_->Start();
}
```

​		LayerTreeHost 类的成员函数 InitializeProxy 首先将参数 proxy 描述的一个 ThreadProxy 对象保存在成员变量 proxy_ 中，接着再调用上述 ThreadProxy 对象的成员函数 Start 对它进行启动，如下所示：

```c++
void ThreadProxy::Start() {
  ......
 
  CompletionEvent completion;
  Proxy::ImplThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&ThreadProxy::InitializeImplOnImplThread,
                 base::Unretained(this),
                 &completion));
  completion.Wait();
 
  main_thread_weak_ptr_ = main().weak_factory.GetWeakPtr();
 
  main().started = true;
}
```

​		ThreadProxy 类的成员函数 Start 主要是向 Compositor 线程的消息队列发送了一个 Task，并且等待这个Task 完成。这个 Task 绑定了 ThreadProxy 类的成员函数 InitializeImplOnImplThread，因此接下来ThreadProxy 类的成员函数 InitializeImplOnImplThread 就会在 Compositor 线程中执行，如下所示：

```c++
void ThreadProxy::InitializeImplOnImplThread(CompletionEvent* completion) {
  ......
  impl().layer_tree_host_impl =
      layer_tree_host()->CreateLayerTreeHostImpl(this);
 
  SchedulerSettings scheduler_settings(layer_tree_host()->settings());
  impl().scheduler = Scheduler::Create(this,
                                       scheduler_settings,
                                       impl().layer_tree_host_id,
                                       ImplThreadTaskRunner());
  ......
 
  impl_thread_weak_ptr_ = impl().weak_factory.GetWeakPtr();
  completion->Signal();
}
```

​		ThreadProxy 类的成员函数 InitializeImplOnImplThread 主要是做了三件事情。

​		第一件事情是调用前面创建的 LayerTreeHost 对象的成员函数 CreateLayerTreeHostImpl 函数创建了一个LayerTreeHostImpl 对象，并且保存在内部的一个 CompositorThreadOnly 对象的成员变量layer_tree_host_impl 中。前面创建的 LayerTreeHost 对象可以通过调用成员函数 layer_tree_host 获得。内部的CompositorThreadOnly 对象可以通过调用成员函数 impl 获得。创建出来的 LayerTreeHostImpl 对象以后负责管理 CC Pending Layer Tree 和 CC Active Layer Tree。

​		第二件事情是调用 Scheduler 类的静态成员函数 Create 创建了一个 Scheduler 对象。这个 Scheduler 对象以后就负责在 Main 线程与 Compositor 线程之间调度渲染工作。

​		第三件事情是将参数 completion 描述的 Completion Event 设置为有信号，这样正在等待的 Main 线程就可以唤醒继续执行其它工作了。

​		接下来我们继续分析 LayerTreeHost 类的成员函数 CreateLayerTreeHostImpl 创建 LayerTreeHostImpl 对象的过程，如下所示：

```c++
scoped_ptr<LayerTreeHostImpl> LayerTreeHost::CreateLayerTreeHostImpl(
    LayerTreeHostImplClient* client) {
  ......
  scoped_ptr<LayerTreeHostImpl> host_impl =
      LayerTreeHostImpl::Create(settings_,
                                client,
                                proxy_.get(),
                                rendering_stats_instrumentation_.get(),
                                shared_bitmap_manager_,
                                id_);
  host_impl->SetUseGpuRasterization(UseGpuRasterization());
  ......
  return host_impl.Pass();
}
```

​		LayerTreeHost 类的成员函数 CreateLayerTreeHostImpl 首先是调用 LayerTreeHostImpl 类的静态成员函 数 Create 创建了一个 LayerTreeHostImpl 对象，接着再调用这个 LayerTreeHostImpl 对象的成员函数SetUseGpuRasterization 设置它是否使用 GPU 光栅化。

​		LayerTreeHostImpl 对象的创建过程，即 LayerTreeHostImpl 类的构造函数的实现，如下所示：

```c++
LayerTreeHostImpl::LayerTreeHostImpl(
    const LayerTreeSettings& settings,
    LayerTreeHostImplClient* client,
    Proxy* proxy,
    RenderingStatsInstrumentation* rendering_stats_instrumentation,
    SharedBitmapManager* manager,
    int id)
    : client_(client),
      proxy_(proxy),
      ...... {
 
  .......
 
  // LTHI always has an active tree.
  active_tree_ = LayerTreeImpl::create(this);
  
  ......
}
```

​		从前面的调用过程可以知道，参数 client 和 proxy 指向的是同一个 ThreadProxy 对象，它们分别保存在LayerTreeHostImpl 类的成员变量 client_ 和 proxy_ 中。LayerTreeHostImpl 类的构造函数接下来还调用LayerTreeImpl 类的静态成员函数 create 创建了一个 Active Layer Tree。注意，这时候 Pending Layer Tree 还没有创建，等到要将 Layer Tree 内容同步到 Pending Layer Tree 的时候才会创建。

​		回到 LayerTreeHost 类的成员函数 CreateLayerTreeHostImpl，它创建了一个 LayerTreeHostImpl 对象之后，接下来调用另外一个成员函数 UseGpuRasterization 获取 Render 进程是否要采用 GPU 光栅化的信息，如下所示：

```c++
bool LayerTreeHost::UseGpuRasterization() const {
  if (settings_.gpu_rasterization_forced) {
    return true;
  } else if (settings_.gpu_rasterization_enabled) {
    return has_gpu_rasterization_trigger_ &&
           content_is_suitable_for_gpu_rasterization_;
  } else {
    return false;
  }
}
```

​		当 Render 进程设置了 "force-gpu-rasterization" 和 "enable-impl-side-painting" 启动选项时，LayerTreeHost 类的成员变量 settings_ 指向的 LayerTreeSettings 对象的成员变量 gpu_rasterization_forced 的值就会等于 true，这时候就会强制使用 GPU 光栅化。

​		当 Render 进程设置了 "enable-gpu-rasterization" 启动选项时，LayerTreeHost 类的成员变量 settings_ 指向的 LayerTreeSettings 对象的成员变量 gpu_rasterization_enabled 的值就会等于 true，这时候是否使用 GPU 光栅化取决于 LayerTreeHost 类另外两个成员变量 has_gpu_rasterization_trigger_ 和content_is_suitable_for_gpu_rasterization_。当这两个成员变量的值均等于 true 的时候，就会使作 GPU 光栅化。

​		LayerTreeHost 类的成员变量 has_gpu_rasterization_trigger_ 的值由 WebKit 间接调用 LayerTreeHost 类的成员函数 SetHasGpuRasterizationTrigger 进行设置，这一点可以参考 WebViewImpl 类的成员函数updatePageDefinedViewportConstraints，主要是与网页的 Viewport 大小以及当前的缩放因子有关。LayerTreeHost 类的成员变量 content_is_suitable_for_gpu_rasterization_ 的值由 Skia 决定，后者根据当前要绘制的内容决定的，例如，如果要绘制的内容包含太多的凹多边形，并且这些凹多边形使用了反锯齿效果，那么就会禁用 GPU 光栅化。这一点可以参考 SkPicturePlayback 类的成员函数 suitableForGpuRasterization。

​		在其它情况下，都是禁止使用 GPU 光栅化的。在目前的版本中，Render 进程设置了 "force-gpu-rasterization" 和 "enable-impl-side-painting" 启动选项，因此会强制使用 GPU 光栅化。在以后的文章中，我们都是假设 Chromium 是使用 GPU 光栅化的。

​		再回到 LayerTreeHost 类的成员函数 CreateLayerTreeHostImpl，它获得 Render 进程是否要采用 GPU 光栅化的信息之后，就会设置给前面创建的 LayerTreeHostImpl 对象，这是通过调用 LayerTreeHostImpl 类的成员函数 SetUseGpuRasterization 如下所示：       

```c++
void LayerTreeHostImpl::SetUseGpuRasterization(bool use_gpu) {
  if (use_gpu == use_gpu_rasterization_)
    return;
 
  use_gpu_rasterization_ = use_gpu;
  ReleaseTreeResources();
 
  // Replace existing tile manager with another one that uses appropriate
  // rasterizer.
  if (tile_manager_) {
    DestroyTileManager();
    CreateAndSetTileManager();
  }
 
  // We have released tilings for both active and pending tree.
  // We would not have any content to draw until the pending tree is activated.
  // Prevent the active tree from drawing until activation.
  active_tree_->SetRequiresHighResToDraw();
}
```

​		LayerTreeHostImpl 类的成员函数 SetUseGpuRasterization 会将参数 use_gpu 的值保存在成员变量use_gpu_rasterization_ 。如果这时候已经创建了分块管理器，即 LayerTreeHostImpl 类的成员变量tile_manager_ 的值不等于 NULL，那么还会先销毁它，然后再重新创建。因为分块管理器在创建的时候就决定了光栅化的方式。最后，LayerTreeHostImpl 类的成员函数 SetUseGpuRasterization 还会要求重新绘制高分辨率的Active Layer Tree。

​		这一步执行完成之后，CC Layer Tree 就初始化完成了，回到前面分析的 WebViewImpl 类的成员函数setIsAcceleratedCompositingActive 中，它接下来会调用成员变量 m_client 指向的 RenderViewImpl 对象的成员函数 layerTreeView 获得之前创建的一个 RenderWidgetCompositor 对象。

​		RenderViewImpl 类的成员函数 layerTreeView 是从父类 RenderWidget 继承下来的，它的实现如下所示：

```c++
blink::WebLayerTreeView* RenderWidget::layerTreeView() {
  return compositor_.get();
}
```

​		从这里可以看到，RenderWidget 类的成员函数 layerTreeView 返回的是成员变量 compositor_ 描述的一个RenderWidgetCompositor 对象。

​		再回到前面分析的 WebViewImpl 类的成员函数 setIsAcceleratedCompositingActive，它获得了一个RenderWidgetCompositor 对象，就会调用这个 RenderWidgetCompositor 对象的成员函数 setRootLayer，以便将 CC Layer Tree 的根节点传递给它处理，如下所示：

```c++
void RenderWidgetCompositor::setRootLayer(const blink::WebLayer& layer) {
  layer_tree_host_->SetRootLayer(
      static_cast<const WebLayerImpl*>(&layer)->layer());
}
```

​		从前面的调用过程可以知道，参数 layer 描述的实际上是一个 WebLayerImpl 对象，调用它的成员函数 layer可以获得与它关联的一个 PictureLayer 对象。这个 PictureLayer 对象将作为 CC Layer Tree 的根节点，设置给RenderWidgetCompositor 类的成员变量 layer_tree_host_ 描述的一个 LayerTreeHost 对象。这是通过调用LayerTreeHost 类的成员函数 SetRootLayer 实现的，如下所示：

```c++
void LayerTreeHost::SetRootLayer(scoped_refptr<Layer> root_layer) {
  ......
 
  root_layer_ = root_layer;
  ......
 
  SetNeedsFullTreeSync();
}
```

​		从这里可以看到，LayerTreeHost 类的成员函数 SetRootLayer 将 CC Layer Tree 的根节点保存在成员变量root_layer_ 中，以后通过这个成员变量就可以遍历整个 CC Layer Tree。由于设置了新的 CC Layer Tree，LayerTreeHost 类的成员函数 SetRootLayer 还会调用另外一个成员函数 SetNeedsFullTreeSync 请求将 CC Layer Tree 同步到 CC Pending Layer Tree 去。

​		前面我们已经分析过 LayerTreeHost 类的成员函数 SetNeedsFullTreeSync 的实现了，它调用了另外一个成员函数 SetNeedsCommit 请求将 CC Layer Tree 同步到 CC Pending Layer Tree 去，也就执行一次ACTION_COMMIT 操作，后者的实现如下所示：

```c++
void LayerTreeHost::SetNeedsCommit() {
  ......
  proxy_->SetNeedsCommit();
  ......
}
```

​		LayerTreeHost 类的成员函数 SetNeedsCommit 调用成员变量 proxy_ 指向的一个 ThreadProxy 对象的成员函数 SetNeedsCommit 请求执行一次 ACTION_COMMIT 操作，如下所示：

```c++
void ThreadProxy::SetNeedsCommit() {
  ......
 
  if (main().commit_requested)
    return;
  ......
  main().commit_requested = true;
 
  SendCommitRequestToImplThreadIfNeeded();
}
```

​		ThreadProxy 对象的成员函数 SetNeedsCommit 首先判断之前请求的 ACTION_COMMIT 操作是否已经被执行。如果还没有被执行，那么就会忽略当前请求，实际上是将多个 ACTION_COMMIT 请求合成一个执行。事实上，每当 WebKit 修改了 Render Object Tree 的内容时，都会请求 CC 模块执行一次 ACTION_COMMIT 操作。不过，如果这些 ACTION_COMMIT 操作请求得太过频繁，就会合成一个一起执行。

​		如果之前没有请求过 ACTION_COMMIT 操作，或者之前请求的 ACTION_COMMIT 操作已经被执行，那么ThreadProxy 类的成员函数 SetNeedsCommit 就会调用另外一个成员函数SendCommitRequestToImplThreadIfNeeded 请求 Compositor 线程执行一次 ACTION_COMMIT 请求，如下所示：

```c++
void ThreadProxy::SendCommitRequestToImplThreadIfNeeded() {
  DCHECK(IsMainThread());
  if (main().commit_request_sent_to_impl_thread)
    return; 
 
  main().commit_request_sent_to_impl_thread = true;
  Proxy::ImplThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&ThreadProxy::SetNeedsCommitOnImplThread,
                 impl_thread_weak_ptr_));
}
```

​		ThreadProxy 类的成员函数 SendCommitRequestToImplThreadIfNeeded 必须要确保在 Main 线程中执行。这也是 Chromium 的多线程编程哲学，在设计上规定了某些对象只能在特定的线程进行访问。

​		ThreadProxy 类的成员函数 SendCommitRequestToImplThreadIfNeeded 首先判断 Main 线程之前是否已经向 Compositor 线程请求过执行 ACTION_COMMIT 操作，并且该请求已经被执行。如果是的话，那么就会忽略当前请求。否则的话，就会向 Compositor 线程的消息队列发送一个 Task，这个 Task 绑定了 ThreadProxy 类的成员函数 SetNeedsCommitOnImplThread。

​		这意味着接下来 ThreadProxy 类的成员函数 SetNeedsCommitOnImplThread 接下来会在 Compositor 线程中执行，如下所示：

```c++
void ThreadProxy::SetNeedsCommitOnImplThread() {
  ......
  DCHECK(IsImplThread());
  impl().scheduler->SetNeedsCommit();
}
```

​		Compositor 线程并不是马上就执行请求的 ACTION_COMMIT 操作，它只是调用了内部的调度器的成员函数SetNeedsCommit，即 Scheduler 类的成员函数 SetNeedsCommit，如下所示：

```c++
void Scheduler::SetNeedsCommit() {
  state_machine_.SetNeedsCommit();
  ProcessScheduledActions();
}
```

​		Scheduler 类的成员函数 SetNeedsCommit 首先通过调用成员变量 state_machine_ 描述的一个SchedulerStateMachine 对象的成员函数 SetNeedsCommit 将内部的状态机设置为需要 COMMIT，如下所示：

```c++
void SchedulerStateMachine::SetNeedsCommit() { needs_commit_ = true; }
```

​		SchedulerStateMachine 类的成员函数 SetNeedsCommit 只是将成员变量 needs_commit_ 的值设置为t rue，表示需要执行一次 ACTION_COMMIT 操作。不过，这个 ACTION_COMMIT 有可能不能马上执行，因为有可能 CC 模块还没有为网页创建 Output Surface，或者 CC 模块现在需要对网页上一帧执行光栅化操作等等。调度器当前到底需要执行什么样的操作，由 Scheduler 类的成员函数 SetNeedsCommit 调用另外一个成员函数ProcessScheduledActions 决定。

​		至此，我们就分析完成 Chromium 为网页创建 CC Layer Tree 的过程了。有了网页的 CC Layer Tre e之后，Chromium 还要为网页创建渲染上下文，也就是 Output Surface，这样才可以将网页的内容渲染出来。网页Output Surface 的创建以及网页其它渲染相关的操作，都是通过一个调度器根据一定的策略触发执行的。因此在接下来的一篇文章中，我们就继续分析 Chromium 网页渲染调度器的执行过程











































