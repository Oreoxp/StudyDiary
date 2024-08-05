[TOC]

# Chromium 网页 Render Layer Tree 创建过程分析

​		在前面一文中，我们分析了网页 Render Object Tree 的创建过程。在创建 Render Object Tree 的同时，WebKit 还会创建 Render Layer Tree，但不是每一个 Render Object 都有对应的 Render Layer。Render Layer 是一个最小渲染单元，被若干 Render Object 共用。本文接下来就分析 Render Layer Tree 的创建过程。

​		网页的 Render Object Tree 与 Render Layer Tree 的关系可以通过图1描述，如下所示：

![img](markdownimage/20160210021036877)

​		从图1还可以看到，Render Layer Tree 创建完成之后，WebKit 还会继续创建一个 Graphics Layer Tree。本文主要关注 Render Layer Tree 的创建过程。DOM Tree 和 Render Object Tree 的创建过程可以参考Chromium网页DOM Tree创建过程分析和Chromium网页Render Object Tree创建过程分析这两篇文章。Graphics Layer Tree 的创建过程在接下来一篇文章分析。

​		**网页的 Render Layer Tree 是在创建 Render Object Tree 的过程中创建的。确切地说，是在设置R ender Object 的 CSS 属性的过程中创建的**。从前面Chromium网页Render Object Tree创建过程分析一文可以知道，当 DOM Tree 中的 HTMLElement 节点需要进行渲染的时候，WebKit 就会为其创建一个 Render Object。这个 Render Object 在创建完成之后，就会被设置 CSS 属性，如下所示：

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

​		从这里可以看到，新创建的 Render Object 的 CSS 属性是通过调用 RenderObject 类的成员函数 setStyle设置的，它的实现如下所示：

```c++
void RenderObject::setStyle(PassRefPtr<RenderStyle> style)
{
    ......
 
    if (m_style == style) {
        ......
        return;
    }
 
    StyleDifference diff;
    unsigned contextSensitiveProperties = ContextSensitivePropertyNone;
    if (m_style)
        diff = m_style->visualInvalidationDiff(*style, contextSensitiveProperties);
 
    ......
 
    RefPtr<RenderStyle> oldStyle = m_style.release();
    setStyleInternal(style);
 
    ......
 
    styleDidChange(diff, oldStyle.get());
 
    ......
}
```

​		RenderObject 类的成员变量 m_style 指向的是一个 RenderStyle 对象，它描述的是当前正在处理的RenderObject 对象的 CSS 属性。RenderObject 类的成员函数 setStyle 首先比较成员变量 m_style 指向的RenderStyle 对象和参数 style 指向的 RenderStyle 对象所描述的 CSS 属性是否是一样的。如果是一样的，那么就**不再**重新设置当前正在处理的 RenderObject 对象的 CSS 属性。

​		如果成员变量 m_style 指向的 RenderStyle 对象和参数 style 指向的 RenderStyle 对象所描述的 CSS 属性不一样。那么 RenderObject 类的成员函数 setStyle 接下来就会计算出它们的差异，并且调用成员函数setStyleInternal 将参数 style 指向的 RenderStyle 对象保存在成员变量 m_style 中，作为当前正在处理的RenderObject 对象的 CSS 属性，最后调用成员函数 styleDidChange 通知子类当前正在处理的RenderObject 对象的 CSS 属性发生了变化。

​		在前面Chromium网页Render Object Tree创建过程分析一文中，我们假设创建的 Render Object 实际上是一个 RenderBlockFlow 对象，也就是当前正在处理的是一个 RenderBlockFlow 对象。RenderBlockFlow类是从 RenderObject 类继承下来的，并且重写了成员函数 styleDidChange。因此，接下来RenderBlockFlow 类的成员函数 styleDidChange 就会被调用。在调用的过程中，就会检查是否需要创建一个 Render Layer。

​		RenderBlockFlow 类的成员函数 styleDidChange 的实现如下所示：

```c++
void RenderBlockFlow::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
 
    ......
}
```

​		RenderBlockFlow 类的成员函数 styleDidChange 又会调用父类 RenderBlock 的成员函数styleDidChange 通知它当前正在处理的 Render Object 的 CSS 属性发生了变化。

​		RenderBlock 类的成员函数 styleDidChange 的实现如下所示：

```c++
void RenderBlock::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);
 
    ......
}
```

​		RenderBlock 类的成员函数 styleDidChange 又会调用父类 RenderBox 的成员函数 styleDidChang e通知它当前正在处理的 Render Object 的 CSS 属性发生了变化。

​		RenderBox 类的成员函数 styleDidChange 的实现如下所示：

```c++
void RenderBox::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    ......
 
    RenderBoxModelObject::styleDidChange(diff, oldStyle);
 
    ......
}
```

​		RenderBox 类的成员函数 styleDidChange 又会调用父类 RenderBoxModelObject 的成员函数styleDidChange 通知它当前正在处理的 Render Object 的 CSS 属性发生了变化。

​		RenderBoxModelObject 类就是用来描述在前面 Chromium网页Render Object Tree创建过程分析一文提到的 CSS Box Model 的，它的成员函数 styleDidChange 是从父类 RenderLayerModelObject 继承下来的，因此接下来我们继续分析 RenderLayerModelObject 类的成员函数 styleDidChange 的实现，如下所示：

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

​		RenderLayerModelObject 类的成员函数 styleDidChange 首先调用成员函数 layerTypeRequired 判断是否需要为当前正在处理的 Render Object 创建一个 Render Layer。如果需要创建，那么RenderLayerModelObject 类的成员函数 layerTypeRequired 的返回值就不等于 NoLayer。在这种情况下，RenderLayerModelObject 类的成员函数就会调用成员函数 layer 检查当前正在处理的 Render Object 是否已经关联有一个 Render Layer 了 。

​		如果已经关联有，那么调用 RenderLayerModelObject 类的成员函数 layer 的返回值就不等于 NULL。这时候就不需要为当前正在处理的 Render Object 创建一个 Render Layer。如果还没有关联，那么RenderLayerModelObjec t类的成员函数 styleDidChang e继续调用成员函数layerCreationAllowedForSubtree 判断在当前正在处理的 Render Object 的祖先节点中，是否存在一个类型为 RenderSVGHiddenContainer 的 Render Object。

​		**类型为 RenderSVGHiddenContainer 的 Render Object 是不需要绘制的**，并且它的子节点也是不需要绘制的。因此，如果在当前正在处理的 Render Object 的祖先节点中存在一个类型为RenderSVGHiddenContainer 的 Render Object，那么就不需要为当前正在处理的 Render Object 创建一个Render Layer 了。

​		另一方面，如果此时在当前正在处理的 Render Object 的祖先节点中不存在一个类型为RenderSVGHiddenContainer 的 Render Object，那么就需要为当前正在处理的 Render Object 创建一个Render Layer，这是通过调用 RenderLayerModelObject 类的成员函数 createLayer 创建一个 Render Layer。

​		如果前面 RenderLayerModelObject 类的成员函数 layerTypeRequired 的返回值等于 NoLayer，并且当前正在处理的 Render Object 关联有 Render Layer，以及这个 Render Layer 位于 Render Layer Tree中，也就是这个 Render Layer 有父节点，那么就需要将这个 Render Layer 从 Render Layer Tree 中删除，这是通过调用 RenderLayer 类的成员函数 removeOnlyThisLayer 实现的。

​		最后，如果前面 RenderLayerModelObject 类的成员函数 styleDidChange 为当前正在处理的 Render Object 创建了一个 Render Layer，或者当前正在处理的 Render Object 本来就已经关联有一个 Render Layer，那么 RenderLayerModelObject 类的成员函数 styleDidChange 还会通知这个 Render Layer，它所关联的 Render Object 的 CSS 属性发生了变化，这是通过调用 RenderLayer 类的成员函数 styleChanged 实现的。RenderLayer 类的成员函数 styleChanged 在调用期间，会判断是否需要为当前正在处理的 Render Layer 创建一个 Graphics Layer。创建出来的 Graphics Layer 就会形成图1所示的 Graphics Layer Tree。在接下来一篇文章中，我们再详细分析Graphics Layer Tree的创建过程。

​		接下来，我们继续分析 RenderLayerModelObject 类的成员函数 layerTypeRequired 和 createLayer 的实现，以便了解一个 Render Object 在什么情况下创建一个 Render Layer，以及这个 Render Layer 的创建过程。

​		前面提到，当前正在处理的 Render Object 实际上是一个 RenderBlockFlow 对象。RenderBlockFlow 类是从 RenderBox 类继承下来的，RenderBox 类又是从 RenderLayerModelObject 类继承下来的，并且它重写了 RenderLayerModelObject 类的成员函数 layerTypeRequired，因此，前面分析的RenderLayerModelObject 类的成员函数 styleDidChange 实际上是调用了子类 RenderBox 类的成员函数layerTypeRequired 判断是否需要为当前正在处理的 Render Object 创建一个 Render Layer。

​		RenderBox 类的成员函数 layerTypeRequired 的实现如下所示：

```c++
class RenderBox : public RenderBoxModelObject {
public:
    ......
 
    virtual LayerType layerTypeRequired() const OVERRIDE
    {
        if (isPositioned() || createsGroup() || hasClipPath() || hasTransform() || hasHiddenBackface() || hasReflection() || style()->specifiesColumns() || !style()->hasAutoZIndex() || style()->shouldCompositeForCurrentAnimations())
            return NormalLayer;
        if (hasOverflowClip())
            return OverflowClipLayer;
 
        return NoLayer;
    }
 
    ......
};
```

**一个Render Object的CSS属性如果具有以下10种情况之一，那么就需要为它创建一个Render Layer：**

   1. isPositioned：position属性值不等于默认值static；

   2. createsGroup：设置有透明度（transparent）、遮罩（mask）、滤镜（filter）或者混合模式（mix-blend-mode）；

   3. hasClipPath：设置有剪切路径（clip-path）; 

   4. hasTransform：设置有2D或者3D转换（matrix、translate、scale、rotate、skew、perspective）;

   5. hasHiddenBackface：隐藏背面（backface-visibility: hidden）；

   6. hasReflection：设置有倒影（box-reflect）；

   7. specifiesColumns：设置有列宽和列数（columns: column-width column-count）；

   8. !hasAutoZIndex：z-index属性值不等于默认值auto，即指定了z-index值；

   9. shouldCompositeForCurrentAnimations：指定了不透明度（opacity）、变换（transform）或者滤镜（filter）动画；

   10. hasOverflowClip：剪切溢出内容（overflow:hidden）。

​        其中，前 9 种情况创建的 Render Layer 的类型为 NormalLayer，第 10 种情况创建的 Render Layer 的类型为 OverflowClipLayer。这两种类型的 Render Layer 在本质上并没有区别，将一个 Render Layer 的类型设置为 OverflowClipLayer 只为 Bookkeeping 目的，即在需要的时候可以在 Render Layer Tree 中找到一个类型为 OverflowClipLayer 的 Render Layer 做相应的处理。

​		RenderLayerModelObject 类的成员函数 createLayer 的实现如下所示：

```c++
void RenderLayerModelObject::createLayer(LayerType type)
{
    ASSERT(!m_layer);
    m_layer = adoptPtr(new RenderLayer(this, type));
    setHasLayer(true);
    m_layer->insertOnlyThisLayer();
}
```

​		RenderLayerModelObject 类的成员函数 createLayer 首先是创建了一个 RenderLayer 对象，并且保存在成员变量 m_layer 中。这个 RenderLayer 对象描述的就是与当前正在处理的 Render Object 关联的Render Layer。RenderLayerModelObject 类的成员函数 createLayer 接下来又调用另外一个成员函数setHasLayer 将当前正在处理的 Render Object 标记为是关联有 Render Layer 的。

​		RenderLayer 对象的创建过程，即 RenderLayer 类的构造函数的实现如下所示：

```c++
RenderLayer::RenderLayer(RenderLayerModelObject* renderer, LayerType type)
    : m_layerType(type)
    , ......
    , m_renderer(renderer)
    , m_parent(0)
    , ......
{
    ......
}
```

​		RenderLayer 类的构造函数主要是将参数 renderer 和 type 描述的 Render Object 和 Layer Type 分别保存在成员变量 m_renderer 和 m_layerType 中，并且将成员变量 m_parent 的值初始化为 0，表示当前正在创建的 Render Layer 还未插入到网页的 Render Layer Tree 中。

​		回到 RenderLayerModelObject 类的成员函数 createLayer 中，它最后调用前面创建的 RenderLayer 对象的成员函数 insertOnlyThisLayer 将其插入到网页的 Render Layer Tree 中去，如下所示：

```c++
void RenderLayer::insertOnlyThisLayer()
{
    if (!m_parent && renderer()->parent()) {
        // We need to connect ourselves when our renderer() has a parent.
        // Find our enclosingLayer and add ourselves.
        RenderLayer* parentLayer = renderer()->parent()->enclosingLayer();
        ASSERT(parentLayer);
        RenderLayer* beforeChild = !parentLayer->reflectionInfo() || parentLayer->reflectionInfo()->reflectionLayer() != this ? renderer()->parent()->findNextLayer(parentLayer, renderer()) : 0;
        parentLayer->addChild(this, beforeChild);
    }
 
    ......
}
```

​		前面提到，RenderLayer 类的成员变量 m_parent 等于 0 的时候，表示当前正在处理的 Render Layer 还未插入到网页的 Render Layer Tree 中，这时候才有可能需要将其插入到网页的 Render Layer Tree 中去。这里说有可能，是因为还需要满足另外一个条件，就是当前正在处理的 Render Layer 所关联的 Render Object 已经插入到网页的 Render Object Tree 中，也就是该 Render Object 具有父节点。

​		RenderLayer 类的成员函数 insertOnlyThisLayer 按照以下三个步骤将一个 Render Layer 插入到网页的 Render Layer Tree 中去：

1. **找到要插入的 Render Layer 关联的 Render Object 的父节点所对应的 Render Layer。这里找到的Render Layer 就作为要插入的 Render Layer 的父 Render Layer。**

2. **找到要插入的 Render Layer 在父 Render Layer 的 Child List 中的位置。**

3. **将要插入的 Render Layer 设置为父 Render Layer 的 Child。**

​        其中，第 1 步和第 3 步分别是通过调用 RenderObject类 的成员函数 enclosingLayer 和 RenderLayer 类的成员函数 addChild 实现的，接下来我们就继续分析它们的实现，以便了解网页的 Render Layer Tree 的创建过程。

​		RenderObject 类的成员函数 enclosingLayer 的实现如下所示：

```c++
RenderLayer* RenderObject::enclosingLayer() const
{
    for (const RenderObject* current = this; current; current = current->parent()) {
        if (current->hasLayer())
            return toRenderLayerModelObject(current)->layer();
    }
    // FIXME: We should remove the one caller that triggers this case and make
    // this function return a reference.
    ASSERT(!m_parent && !isRenderView());
    return 0;
}
```

​		**从这里可以看到，一个 Render Object 如果没有关联有 Render Layer，那么它就与离其最近的关联有Render Layer 的父节点使用同一个 Render Layer。**

​		RenderLayer类的成员函数addChild的实现如下所示：

```c++
void RenderLayer::addChild(RenderLayer* child, RenderLayer* beforeChild)
{
    RenderLayer* prevSibling = beforeChild ? beforeChild->previousSibling() : lastChild();
    if (prevSibling) {
        child->setPreviousSibling(prevSibling);
        prevSibling->setNextSibling(child);
        ASSERT(prevSibling != child);
    } else
        setFirstChild(child);
 
    if (beforeChild) {
        beforeChild->setPreviousSibling(child);
        child->setNextSibling(beforeChild);
        ASSERT(beforeChild != child);
    } else
        setLastChild(child);
 
    child->m_parent = this;
 
    ......
}
```

​		一个 Render Layer 的子 Render Layer 以链表方式进行组织。RenderLayer 类的成员函数 addChild 将参数 child 描述的子 Render Layer 插入在参数 beforeChild 描述的子 Render Layer 的前面。如果没有指定Before Child，那么参数 child 描述的子 Render Layer 就会插入在链表的最后。另一方面，如果指定的Before Child 就是保存在链表的第一个位置，那么参数 child 描述的子 Render Layer 就会取代它保存在链表的第一个位置。

​		最后，RenderLayer 类的成员函数 addChild 还会将参数 child 描述的 Render Layer 的父 Render Layer设置为当前正在处理的 Render Layer，从而完成将一个 Render Layer 插入到网页的 Render Layer Tree 的操作。

​		至此，我们就分析完成网页的 Render Layer Tree 的创建过程了。在接下来一篇文章中，我们将继续分析网页的 Graphics Layer Tree 的创建过程，它是根据网页的 Render Layer Tree 进行创建的。有了 Graphics Layer Tree 之后，WebKit 才能对网页进行绘制。







