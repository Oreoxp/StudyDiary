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































