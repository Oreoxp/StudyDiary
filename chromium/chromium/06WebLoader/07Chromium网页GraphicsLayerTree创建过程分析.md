[TOC]

# Chromium 网页 Graphics Layer Tree 创建过程分析

​		在前面一文中，我们分析了网页 Render Layer Tree 的创建过程。在创建 Render Layer 的同时，WebKit还会为其创建 Graphics Layer。这些 Graphics Layer 形成一个Graphics Layer Tree。Graphics Layer 可看作是一个图形缓冲区，被若干 Render Layer 共用。本文接下来就分析 Graphics Layer Tree 的创建过程。

​		网页的 Render Layer Tree 与 Graphics Layer Tree 的关系可以通过图1描述，如下所示：

![img](markdownimage/20160214013252908)

​		在 WebKit 中，Graphics Layer 又称为 Composited Layer。我们可以将 Graphics Layer 看作是Composited Layer 的一种具体实现。这种具体实现是由 WebKit 的使用者 Chromium 提供的。Composited Layer 描述的是一个具有后端存储的图层，因此可以将它看作是一个图形缓冲区。**在软件渲染方式中，这个图形缓冲区就是一块系统内存；在硬件渲染方式中，这个图形缓冲区就是一个 OpenGL 里面的一个 Frame Buffer Object（FBO）。**

​		Composited Layer 涉及到的一个重要概念是 “ Layer Compositing ” 。Layer Compositing 是现代 UI 框架普遍采用的一种渲染机制。例如，Android 系统的 UI子 系统（Surface Flinger）就是通过 Compositing Surface 来获得最终要显示在屏幕上的内容的。这里的 Surface 就相当于是 Chromium 的 Layer。

​		Layer Compositing 的三个主要任务是：

   1. 确定哪些内容应该在哪些 Composited Layer 上绘制；

   2. 绘制每一个 Composited Layer；

   3. 将所有已经绘制好的 Composited Layer 再次将绘制在一个最终的、可以显示在屏幕上进行显示的图形缓冲区中。

​        其中，第 1 个任务它完成之后就可以获得一个 Graphics Layer Tree，第 3 个任务要求按照一定的顺序对Composited Layer 进行绘制。注意，这个绘制顺序非常重要，否则最终合成出来的 UI 就会出现不正确的 Overlapping。同时，这个绘制顺序对理解 Graphics Layer Tree 的组成也非常重要。因此，接下来我们首先介绍与这个绘制顺序有关的概念。为了方便描述，本文将上述绘制顺序称为 Composited Layer 的绘制顺序。

### Graphics Layer Tree 与 Render Layer Tree 区别

- **目的和应用**：Graphics Layer Tree 更多关注于图层的硬件加速和复合操作，而 Render Layer Tree 侧重于页面的视觉结构和元素的渲染。Graphics Layer Tree 是对 Render Layer Tree 的一种优化和补充，用于提升性能和动画效果。
- **内容和结构**：Graphics Layer Tree 关注的是如何将页面分解为可以独立处理的图层，以及这些图层如何通过硬件加速来提升渲染性能。Render Layer Tree 则是基于DOM和CSSOM的视觉表示，它描述了页面中的渲染层及其属性。
- **渲染过程中的角色**：在渲染过程中，Render Layer Tree 通常先被构建，基于它，浏览器决定哪些部分需要被提升为独立图层，进而生成 Graphics Layer Tree，最终利用这些图层进行高效的渲染和复合。

### Composited Layer 的绘制顺序

​		在介绍 Composited Layer 的绘制顺序之前，我们还需要回答一个问题：**为什么要采用 Layer Compositing 这种 UI 渲染机制？**主要有两个原因：

1. 避免不必要的重绘。考虑一个网页有两个 Layer。在网页的某一帧显示中，Layer 1 的元素发生了变化，Layer 2 的元素没有发生变化。这时候只需要重新绘制 Layer 1 的内容，然后再与 Layer 2 原有的内容进行 Compositing，就可以得到整个网页的内容。这样就可以避免对没有发生变化的 Layer 2 进行不必要的绘制。

2. 利用硬件加速高效实现某些 UI 特性。例如网页的某一个 Layer 设置了可滚动、3D 变换、透明度或者滤镜，那么就可以通过 GPU 来高效实现。

​        在默认情况下，网页元素的绘制是按照 Render Object Tree 的**先序遍历**顺序进行的，并且它们在空间上是按照各自的 display 属性值依次进行布局的。例如，如果一个网页元素的 display 属性值为 " inline " ，那么它就会以内联元素方式显示，也就是紧挨在前一个绘制的元素的后面进行显示。又如，如果一个网页元素的display 属性值为 " block " ，那么它就会以块级元素进行显示，也就是它的前后会各有一个换行符。我们将这种网页元素绘制方式称为 Normal Flow 或者 In Flow。

​		有默认情况，就会有例外情况。例如，如果一个网页元素同时设置了 position 和 z-index 属性，那么它可能就不会以 In Flow 的方式进行显示，而是以 Out of Flow 的方式进行显示。在默认情况下，一个网页元素的position 和 z-index 属性值被设置为 “static” 和 "auto"。网页元素的 position 属性还可以取值为 “relative”、“absolute” 和 “fixed”，这一类网页元素称为 Positioned 元素。当一个 Positioned 元素的 z-index 属性值不等于 "auto" 时，它就会以 Out of Flow 的方式进行显示。

​		CSS 2.1 规范规定网页渲染引擎要为每一个 z-index 属性值不等于 "auto" 的 Positioned 元素创建一个Stacking Context。对于其它的元素，它们虽然没有自己的 Stacking Context，但是它们会与最近的、具有自己的 Stacking Context 的元素共享同相同的 Stacking Context。不同 Stacking Context 的元素的绘制顺序是不会相互交叉的。假设有两个 Stacking Context，一个包含有 A 和 B 两个元素，另一个包含有 C 和 D 两个元素，那么 A、B、C 和 D 四个元素的绘制顺序只可能为：

 1. A、B、C、D

 2. B、A、C、D

 3. A、B、D、C
 4. B、A、D、C
 5. C、D、A、B
 6. C、D、B、A
 7. D、C、A、B
 8. D、C、B、A

​       Stacking Context 的这个特性，使得它可以成为一个观念上的**原子类型绘制层（Atomic Conceptual Layer for Painting）**。也就是说，只要我们定义好 Stacking Context 内部元素的绘制顺序，那么再根据拥有 Stacking Context 的元素的 z-index 属性值，那么就可以得到网页的所有元素的绘制顺序。

​		我们可以通过图 2 所示的例子直观地理解 Stacking Context 的上述特性，如下所示：

![img](markdownimage/20160215031944251)

​		上图，一共有 4 个 Stacking Context。最下面的 Stacking Context 的 z-index 等于 -1；中间的Stacking Context 的 z-index 等于 0；最上面的 Stacking Context 的 z-index 等于 1，并且嵌套了另外一个z-index 等于 6 的 Stacking Context。我们观察被嵌套的 z-index 等于 6 的 Stacking Context，它包含了另外一个 z-index 也是等于 6 的元素，但是这两个 z-index 的含义是不一样的。其**中，Stacking Context 的z-index 值是放在父 Stacking Context 中讨论才有意义，而元素的 z-index 放在当前它所在的 Stacking Context 讨论才有意义**。再者，我们是下面和中间的两个 Stacking Context，虽然它们都包含有三个 z-index 分别等于 7、8 和 9 的元素，但是它们是完全不相干的。

![img](markdownimage/20160215031950437)

​		如果我们将图2中间的 Stacking Context 的 z-index 修改为 2，那么它就会变成最上面的 Stacking Context，并且会重叠在 z-index 等于 1 的 Stacking Context 上，以及嵌套在这个 Stacking Context 里面的那个 Stacking Context。

​		这样，我们就得到了 Stacking Context 的绘制顺序。如前所述，接下来只要定义好 Stacking Context 内的元素的绘制顺序，那么就可以网页的所有元素的绘制顺序。Stacking Context 内的元素的绘制顺序如下所示：

1. 背景（Backgrounds）和边界（Borders），也就是拥有Stacking Context的元素的背景和边界。

2. Z-index值为负数的子元素。

3. 内容（Contents），也就是拥有Stacking Context的元素的内容。
4. Normal Flow类型的子元素。
5. Z-index值为正数的子元素。

​        以上就是与 Composited Layer 的绘制顺序有关的背景知识。这些背景知识在后面分析 Graphics Layer Tree 的创建过程时就会用到。



​		从图1可以看到，Graphics Layer Tree 是根据 Render Layer Tree 创建的。也就是说，Render Layer 与Graphics Layer 存在对应关系，如下所示：

![img](markdownimage/20160216011427129)

​		原则上，Render Layer Tree 中的每一个 Render Layer 都对应有一个 **Composited Layer Mapping**，每一个 Composited Layer Mapping 又包含有若干个 Graphics Layer。但是这样将会导致创建大量的 Graphics Layer。创建大量的 Graphics Layer 意味着需要耗费大量的内存资源。这个问题称为 ” Layer Explosion “ 问题。

​		为了解决 “ Layer Explosion ” 问题，每一个需要创 建Composited Layer Mapping 的 Render Layer 都需要给出一个理由。这个理由称为 “Compositing Reason”，它描述的实际上是 Render Layer 的特征。例如，如果一个 Render Layer 关联的 Render Object 设置了 3D Transform 属性，那么就需要为该 Render Layer 创建一个 Composited Layer Mapping。

​		WebKit 一共定义了 54 个 Compositing Reason，如下所示：

```c++
// Intrinsic reasons that can be known right away by the layer
const uint64_t CompositingReason3DTransform                              = UINT64_C(1) << 0;
const uint64_t CompositingReasonVideo                                    = UINT64_C(1) << 1;
const uint64_t CompositingReasonCanvas                                   = UINT64_C(1) << 2;
const uint64_t CompositingReasonPlugin                                   = UINT64_C(1) << 3;
const uint64_t CompositingReasonIFrame                                   = UINT64_C(1) << 4;
const uint64_t CompositingReasonBackfaceVisibilityHidden                 = UINT64_C(1) << 5;
const uint64_t CompositingReasonActiveAnimation                          = UINT64_C(1) << 6;
const uint64_t CompositingReasonTransitionProperty                       = UINT64_C(1) << 7;
const uint64_t CompositingReasonFilters                                  = UINT64_C(1) << 8;
const uint64_t CompositingReasonPositionFixed                            = UINT64_C(1) << 9;
const uint64_t CompositingReasonOverflowScrollingTouch                   = UINT64_C(1) << 10;
const uint64_t CompositingReasonOverflowScrollingParent                  = UINT64_C(1) << 11;
const uint64_t CompositingReasonOutOfFlowClipping                        = UINT64_C(1) << 12;
const uint64_t CompositingReasonVideoOverlay                             = UINT64_C(1) << 13;
const uint64_t CompositingReasonWillChangeCompositingHint                = UINT64_C(1) << 14;
 
// Overlap reasons that require knowing what's behind you in paint-order before knowing the answer
const uint64_t CompositingReasonAssumedOverlap                           = UINT64_C(1) << 15;
const uint64_t CompositingReasonOverlap                                  = UINT64_C(1) << 16;
const uint64_t CompositingReasonNegativeZIndexChildren                   = UINT64_C(1) << 17;
const uint64_t CompositingReasonScrollsWithRespectToSquashingLayer       = UINT64_C(1) << 18;
const uint64_t CompositingReasonSquashingSparsityExceeded                = UINT64_C(1) << 19;
const uint64_t CompositingReasonSquashingClippingContainerMismatch       = UINT64_C(1) << 20;
const uint64_t CompositingReasonSquashingOpacityAncestorMismatch         = UINT64_C(1) << 21;
const uint64_t CompositingReasonSquashingTransformAncestorMismatch       = UINT64_C(1) << 22;
const uint64_t CompositingReasonSquashingFilterAncestorMismatch          = UINT64_C(1) << 23;
const uint64_t CompositingReasonSquashingWouldBreakPaintOrder            = UINT64_C(1) << 24;
const uint64_t CompositingReasonSquashingVideoIsDisallowed               = UINT64_C(1) << 25;
const uint64_t CompositingReasonSquashedLayerClipsCompositingDescendants = UINT64_C(1) << 26;
 
// Subtree reasons that require knowing what the status of your subtree is before knowing the answer
const uint64_t CompositingReasonTransformWithCompositedDescendants       = UINT64_C(1) << 27;
const uint64_t CompositingReasonOpacityWithCompositedDescendants         = UINT64_C(1) << 28;
const uint64_t CompositingReasonMaskWithCompositedDescendants            = UINT64_C(1) << 29;
const uint64_t CompositingReasonReflectionWithCompositedDescendants      = UINT64_C(1) << 30;
const uint64_t CompositingReasonFilterWithCompositedDescendants          = UINT64_C(1) << 31;
const uint64_t CompositingReasonBlendingWithCompositedDescendants        = UINT64_C(1) << 32;
const uint64_t CompositingReasonClipsCompositingDescendants              = UINT64_C(1) << 33;
const uint64_t CompositingReasonPerspectiveWith3DDescendants             = UINT64_C(1) << 34;
const uint64_t CompositingReasonPreserve3DWith3DDescendants              = UINT64_C(1) << 35;
const uint64_t CompositingReasonReflectionOfCompositedParent             = UINT64_C(1) << 36;
const uint64_t CompositingReasonIsolateCompositedDescendants             = UINT64_C(1) << 37;
 
// The root layer is a special case that may be forced to be a layer, but also it needs to be
// a layer if anything else in the subtree is composited.
const uint64_t CompositingReasonRoot                                     = UINT64_C(1) << 38;
 
// CompositedLayerMapping internal hierarchy reasons
const uint64_t CompositingReasonLayerForAncestorClip                     = UINT64_C(1) << 39;
const uint64_t CompositingReasonLayerForDescendantClip                   = UINT64_C(1) << 40;
const uint64_t CompositingReasonLayerForPerspective                      = UINT64_C(1) << 41;
const uint64_t CompositingReasonLayerForHorizontalScrollbar              = UINT64_C(1) << 42;
const uint64_t CompositingReasonLayerForVerticalScrollbar                = UINT64_C(1) << 43;
const uint64_t CompositingReasonLayerForScrollCorner                     = UINT64_C(1) << 44;
const uint64_t CompositingReasonLayerForScrollingContents                = UINT64_C(1) << 45;
const uint64_t CompositingReasonLayerForScrollingContainer               = UINT64_C(1) << 46;
const uint64_t CompositingReasonLayerForSquashingContents                = UINT64_C(1) << 47;
const uint64_t CompositingReasonLayerForSquashingContainer               = UINT64_C(1) << 48;
const uint64_t CompositingReasonLayerForForeground                       = UINT64_C(1) << 49;
const uint64_t CompositingReasonLayerForBackground                       = UINT64_C(1) << 50;
const uint64_t CompositingReasonLayerForMask                             = UINT64_C(1) << 51;
const uint64_t CompositingReasonLayerForClippingMask                     = UINT64_C(1) << 52;
const uint64_t CompositingReasonLayerForScrollingBlockSelection          = UINT64_C(1) << 53;
```

其中，有3个Compositing Reason比较特殊，如下所示：

```c++
const uint64_t CompositingReasonComboSquashableReasons =
    CompositingReasonOverlap
    | CompositingReasonAssumedOverlap
    | CompositingReasonOverflowScrollingParent;
```

​		它们是 CompositingReasonOverlap、CompositingReasonAssumedOverlap 和CompositingReasonOverflowScrollingParent，称为 Squashable Reason。WebKit 不会为具有这三种特征的 Render Layer 之一的 Render Layer 创建 Composited Layer Mapping。

​		如果启用了 Overlap Testing，那么 WebKit 会根据上述的 Stacking Context 顺序计算每一个 Render Layer 的后面是否有其它的 Render Layer 与其重叠。如果有，并且与其重叠的 Render Layer 有一个对应的Composited Layer Mapping，那么就会将位于上面的 Render Layer 的 Compositing Reason 设置为 CompositingReasonOverlap。

​		如果没有启用 Overlap Testing，那么 WebKit 会根据上述的 Stacking Context 顺序检查每一个 Render Layer 的后面是否有一个具有 Composited Layer Mapping 的 Render Layer。只要有，不管它们是否重叠，那么就会将位于上面的 Render Layer 的 Compositing Reason设置为CompositingReasonAssumedOverlap。

​		最后，如果一个 Render Layer 包含在一个具有 overflow 属性为 "scroll" 的 Render Block 中，并且该Render Block 所对应的 Render Layer 具有 Composited Layer Mapping，那么该 Render Layer 的Compositing Reason 就会被设置为 CompositingReasonOverflowScrollingParent。

​		WebKit 会将位于一个具有 Composited Layer Mapping 的 Render Layer 的上面的那些有着Squashable Reason 的 Render Layer 绘制在同一个 Graphics Layer 中。这种 Graphics Layer 称为Squashing Graphics Layer。这种机制也相应地称为 “ Layer Squashing”  。通过 Layer Squashing 机制，就可以在一定程度上减少 Graphics Layer 的数量，从而在一定程度上解决 “ Layer Explosion ” 问题。

​		我们思考一下，为什么 WebKit 会将具有上述 3 种 Compositing Reason 的 Render Layer 绘制在一个Squashing Graphics Layer 中？考虑具有 CompositingReasonOverlap 和CompositingReasonAssumedOverlap 的 Render Layer，当它们需要重绘，或者它们下面的具有Composited Layer Mapping 的 Render Layer 重绘时，都不可避免地对它们以及它们下面的具有Composited Layer Mapping 的 Render Layer 进行 Compositing。**这是由于它们相互之间存在重叠区域，只要其中一个发生变化，就会牵一发而动全身**。类似地，当一个 overflow 属性为 " scroll " 的 Render Block滚动时，包含在该 Render Block 内的 Render Layer 在执行完成重绘操作之后，需要参与到 Compositing 操作去。

​		WebKit定义了两个函数，用来判断一个Render Layer是需要Compositing还是Squashing，如下所示：

```c++
// Any reasons other than overlap or assumed overlap will require the layer to be separately compositing.
inline bool requiresCompositing(CompositingReasons reasons)
{
    return reasons & ~CompositingReasonComboSquashableReasons;
}
 
// If the layer has overlap or assumed overlap, but no other reasons, then it should be squashed.
inline bool requiresSquashing(CompositingReasons reasons)
{
    return !requiresCompositing(reasons) && (reasons & CompositingReasonComboSquashableReasons);
}
```

​		参数 reasons 描述的是一个 Render Layer 的 Compositing Reason，函数 requiresCompositing 判断该 Render Layer 是否需要Compositing，也就是是否要为该 Render Layer 创建一个 Composited Layer Mapping，而函数 requiresSquashing 判断该 Render Layer 需要 Squashing，也就是绘制在一个Squashing Graphics Layer 中。

​		除了Compositing Render Layer和Squashing Render Layer，剩下的其它 Render Layer 称为 Non-Compositing Render Layer。这些 Render Layer 将会与离其最近的具有 Composited Layer Mapping 的父Render Layer 绘制同样的 Graphics Layer 中。

​		WebKit 是根据 Graphics Layer Tree 来绘制网页内容的。在绘制一个 Graphics Layer 的时候，除了绘制Graphics Layer 本身所有的内容之外，还会在 Render Layer Tree 中。找到与该 Graphics Layer 对应的Render Layer，并且从该 Render Layer 开始，将那些 Non-Compositing 类型的子 Render Layer 也一起绘制，直到遇到一个具有 Composited Layer Mapping 的子 Render Layer 为止。这个过程在后面的文章中分析网页内容的绘制过程时就会看到。

​		前面提到，Composited Layer Mapping 包含有若干个 Graphics Layer，这些 Graphics Layer 在Composited Layer Mapping，也是形成一个 Graphics Layer Sub Tree 的，如图4所示：

![img](markdownimage/20160219021246990)

​		图4的左边是一个 Render Layer Tree。其中红色的 Render Layer 有对应的 Composited Layer Mapping。每一个 Composited Layer Mapping 内部都有一个 Graphics Layer Sub Tree。同时，这些Graphics Layer Sub Tree 又会组合在一起，从而形成整个网页的 Graphics Layer Tree。

​		一个典型的Composited Layer Mapping 对应的部分 Graphics Layer Sub Tree 如图5所示：

![img](markdownimage/20160220032309201)

​		注意，图5描述的是仅仅是一个 Composited Layer Mapping 对应的 Graphics Layer Sub Tree 的一部分。例如，如果拥有该 Composited Layer Mapping 的 Render Layer 的上面存在 Squashing Render Layer，那么上述 Graphics Layer Sub Tree 还包含有一个 Squashing Graphics Layer。不过这一部分Graphics Layer Sub Tree 已经足于让我们理解 Composited Layer Mapping 的组成。

​		在图5中，只有Main Layer是必须存在的，它用来绘制一个Render Layer自身的内容。其它的Graphics Layer是可选，其中：

1. 如果一个Render Layer被父Render Layer设置了裁剪区域，那么就会存在Clip Layer。

2. 如果一个Render Layer为子Render Layer设置了裁剪区域，那么就会存在Children Clip Layer。

3. 如果一个Render Layer是可滚动的，那么就会存在Scrolling Container。

4. Negative z-order children、Normal flow children和Positive z-order children描述的是按照Stacking Context规则排序的子Render Layer对应的Composited Layer Mapping描述的Graphics Layer Sub Tree，它们均以父Render Layer的Scrolling Container为父Graphics Layer。

5. 如果一个Render Layer是根Render Layer，并且它的背景被设置为固定的，即网页的body标签的CSS属性background-attachment被设置为“fixed”，那么就会存在Background Layer。

6. 当Negative z-order children存在时，就会存在Foreground Layer。从前面描述的Stacking Context规则可以知道，Negative z-order children对应的Graphics Layer Sub Tree先于当前Graphics Layer Sub Tree绘制。Negative z-order children对应的Graphics Layer Sub Tree在绘制的时候可能会设置了偏移位置。这些偏移位置不能影响后面的Normal flow children和Positive z-order children对应的Graphics Layer Sub Tree的绘制，因此就需要在中间插入一个Foreground Layer，用来抵消Negative z-order children对应的Graphics Layer Sub Tree设置的偏移位置。

7. 如果一个Render Layer的上面存在Squashing Render Layer，那么就会存在Squashing Layer。

了解了Composited Layer Mapping对应的Graphics Layer Sub Tree的结构之后，接下来我们就可以结合源码分析网页的Graphics Layer Tree的创建过程了。网页的Graphics Layer Tree的创建主要是分三步进行：

1. 计算各个Render Layer Tree中的Render Layer的Compositing Reason；

2. 为有需要的Render Layer创建Composited Layer Mapping；

3. 将各个Composited Layer Mapping描述Graphics Layer Sub Tree连接起来形成Graphics Layer Tree。

上述过程主要是发生在网页的 Layout 过程中。对网页进行 Layout 是网页渲染过程的一个重要步骤，以后我们分析网页的渲染过程时就会看到这一点。在 WebKit 中，每一个正在加载的网页都关联有一个 FrameView 对象。当需要对网页进行 Layout 时，就会调用这个 FrameView 对象的成员函数updateLayoutAndStyleForPainting，它的实现如下所示：

```c++
void FrameView::updateLayoutAndStyleForPainting()
{
    // Updating layout can run script, which can tear down the FrameView.
    RefPtr<FrameView> protector(this);
 
    updateLayoutAndStyleIfNeededRecursive();
 
    if (RenderView* view = renderView()) {
        ......
 
        view->compositor()->updateIfNeededRecursive();
 
        ......
    }
 
    ......
}
```

​		FrameView 类的成员函数 updateLayoutAndStyleForPainting 是通过调用另外一个成员函数updateLayoutAndStyleIfNeededRecursive 对网页进行 Layout 的。

​		执行完成 Layout 操作之后，FrameView 类的成员函数 updateLayoutAndStyleForPainting 又调用成员函数 renderView 获得一个 RenderView 对象。从前面Chromium网页DOM Tree创建过程分析一文可以知道，网页的 DOM Tre e的根节点对应的 Render Object 就是一个 RenderView 对象。因此，前面获得的RenderView 对象描述的就是正在加载的网页的 Render Layer Tree 的根节点。

​		再接下来，FrameView 类的成员函数 updateLayoutAndStyleForPainting 又调用上述 RenderView 对象的成员函数 compositor 获得一个 RenderLayerCompositor 对象，如下所示：

```c++
RenderLayerCompositor* RenderView::compositor()
{
    if (!m_compositor)
        m_compositor = adoptPtr(new RenderLayerCompositor(*this));
 
    return m_compositor.get();
}
```

​		这个 RenderLayerCompositor 对象负责管理网页的 Render Layer Tree，以及根据 Render Layer Tree创建  Graphics Layer Tree。

​		回到 FrameView 类的成员函数 updateLayoutAndStyleForPainting 中，它获得了正在加载的网页对应的 RenderLayerCompositor 对象之后，接下来就调用这个 RenderLayerCompositor 对象的成员函数updateIfNeededRecursive 根据 Render Layer Tree 创建或者更新 Graphics Layer Tree。

​		RenderLayerCompositor 类的成员函数 updateIfNeededRecursive 的实现如下所示：

```c++
void RenderLayerCompositor::updateIfNeededRecursive()
{
    ......
 
    updateIfNeeded();
    
    ......
}
```

​		RenderLayerCompositor 类的成员函数 updateIfNeededRecursive 调用另外一个成员函数updateIfNeeded 创建 Graphics Layer Tree，如下所示：

```c++
void RenderLayerCompositor::updateIfNeeded()
{
    CompositingUpdateType updateType = m_pendingUpdateType;
    m_pendingUpdateType = CompositingUpdateNone;
 
    if (!hasAcceleratedCompositing() || updateType == CompositingUpdateNone)
        return;
 
    RenderLayer* updateRoot = rootRenderLayer();
 
    Vector<RenderLayer*> layersNeedingRepaint;
 
    if (updateType >= CompositingUpdateAfterCompositingInputChange) {
        bool layersChanged = false;
        ......
 
        CompositingRequirementsUpdater(m_renderView, m_compositingReasonFinder).update(updateRoot);
 
        {
            ......
            CompositingLayerAssigner(this).assign(updateRoot, layersChanged, layersNeedingRepaint);
        }
 
        ......
 
        if (layersChanged)
            updateType = std::max(updateType, CompositingUpdateRebuildTree);
    }
 
    if (updateType != CompositingUpdateNone) {
        ......
        GraphicsLayerUpdater updater;
        updater.update(layersNeedingRepaint, *updateRoot);
 
        if (updater.needsRebuildTree())
            updateType = std::max(updateType, CompositingUpdateRebuildTree);
 
        ......
    }
 
    if (updateType >= CompositingUpdateRebuildTree) {
        GraphicsLayerVector childList;
        {
            ......
            GraphicsLayerTreeBuilder().rebuild(*updateRoot, childList);
        }
 
        if (childList.isEmpty())
            destroyRootLayer();
        else
            m_rootContentLayer->setChildren(childList);
 
        ......
    }
 
    ......
}
```

​		当 RenderLayerCompositor 类的成员变量 m_pendingUpdateType 的值不等于CompositingUpdateNone 的时候，就表明网页的 Graphics Layer Tree 需要进行更新。此外，要对网页的Graphics Layer Tree 需要进行更新，还要求浏览器开启硬件加速合成。当调用 RenderLayerCompositor 类的成员函数 hasAcceleratedCompositing 得到的返回值等于 true 的时候，就表明浏览器开启了硬件加速合成。

​		当  RenderLayerCompositor 类的成员变量 m_pendingUpdateType 的值大于等于CompositingUpdateAfterCompositingInputChange 的时候，表示 Graphics Layer Tree 的输入发生了变化，例如，Render Layer Tree 中的某一个 Render Layer 的内容发生了变化。在这种情况下，RenderLayerCompositor 类的成员函数 updateIfNeeded 会构造一个 CompositingRequirementsUpdater对象，并且调用这个 CompositingRequirementsUpdater 对象的成员函数 update 从网页的 Render Layer Tree 的根节点开始，递归计算每一个 Render Layer的 Compositing Reason，主要就是根据各个 Render Layer 包含的 Render Object 的 CSS 属性来计算。

​		计算好网页的 Render Layer Tree 中的每一个 Render Layer 的 Compositing Reason 之后，RenderLayerCompositor 类的成员函数 updateIfNeeded 接着再构造一个 CompositingLayerAssigner 对象，并且调用这个 CompositingLayerAssigner 对象的成员函数 assign 根据每一个 Render Layer 新的Compositing Reason 决定是否需要为它创建一个新的 Composited Layer Mapping 或者删除它原来拥有的Composited Layer Mapping。如果有的 Render Layer 原来是没有 Composited Layer Mapping 的，现在有了 Composited Layer Mapping，或者原来有 Composited Layer Mapping，现在没有了 Composited Layer Mapping，那么本地变量 layersChanged 的值就会被设置为 true。这时候本地变量 updateType 的值会被更新为 CompositingUpdateRebuildTree。

​		在本地变量 updateType 的值不等于 CompositingUpdateNone 的情况下，RenderLayerCompositor 类的成员函数 updateIfNeeded 接下来又会构造一个 GraphicsLayerUpdater 对象，并且调用这个GraphicsLayerUpdater 对象的成员函数 update 检查每一个拥有 Composited Layer Mapping 的 Render Layer 更新它的 Composited Layer Mapping 所描述的 Graphics Layer Sub Tree。如果有 Render Layer 更新了它的 Composited Layer Mapping 所描述的 Graphics Layer Sub Tree，那么调用上述GraphicsLayerUpdater 对象的成员函数 needsRebuildTree 获得的返回值就会等 于true。这时候本地变量updateType 的值也会被更新为 CompositingUpdateRebuildTree。

​		一旦本地变量 updateType 的值被更新为 CompositingUpdateRebuildTree，或者它本来的值，也就是RenderLayerCompositor 类的成员变量 m_pendingUpdateType 的值，原本就等于CompositingUpdateRebuildTree，那么 RenderLayerCompositor 类的成员函数 updateIfNeeded 又会构造一个 GraphicsLayerTreeBuilder 对象，并且调用这个 GraphicsLayerTreeBuilder 对象的成员函数rebuild 从 Render Layer Tree 的根节点开始，递归创建一个新的 Graphics Layer Tree。

​		注意，前面调用 GraphicsLayerTreeBuilder 类的成员函数 rebuild 的时候，传递进去的第一个参数updateRoot 是 Render Layer Tree 的根节点，第二个参数 childList 是一个输出参数，它里面保存的是Graphics Layer Tree 的根节点的子节点。Graphics Layer Tree 的根节点由 RenderLayerCompositor 类的成员函数 m_rootContentLayer 指向的 GraphicsLayer 对象描述，因此当参数 childList 描述的 Vector 不为空时，它里面所保存的 Graphics Layer 都会被设置为 RenderLayerCompositor 类的成员函数m_rootContentLayer 指向的 GraphicsLayer 对象的子 Graphics Layer。

​		接下来我们主要分析 CompositingLayerAssigner 类的成员函数 assign、GraphicsLayerUpdater 类的成员函数 update 以及 GraphicsLayerTreeBuilder 类的成员函数 rebuild 的实现，以及了解 Graphics Layer Tree 的创建过程。

​		CompositingLayerAssigner 类的成员函数 assign 主要是为 Render Layer 创建或者删除 Composited Layer Mapping，它的实现如下所示：

```c++
void CompositingLayerAssigner::assign(RenderLayer* updateRoot, bool& layersChanged, Vector<RenderLayer*>& layersNeedingRepaint)
{
    SquashingState squashingState;
    assignLayersToBackingsInternal(updateRoot, squashingState, layersChanged, layersNeedingRepaint);
    ......
}
```

​		从前面的分析可以知道，参数 updateRoot 描述的是 Render Layer Tree 的根节点，CompositingLayerAssigner 类的成员函数 assign 主要是调用另外一个成员函数assignLayersToBackingsInternal 从这个根节点开始，递归是否需要为每一个 Render Layer 创建或者删除Composited Layer Mapping。

​		CompositingLayerAssigner 类的成员函数 assignLayersToBackingsInternal 的实现如下所示：

```c++
void CompositingLayerAssigner::assignLayersToBackingsInternal(RenderLayer* layer, SquashingState& squashingState, bool& layersChanged, Vector<RenderLayer*>& layersNeedingRepaint)
{
    ......
 
    CompositingStateTransitionType compositedLayerUpdate = computeCompositedLayerUpdate(layer);
 
    if (m_compositor->allocateOrClearCompositedLayerMapping(layer, compositedLayerUpdate)) {
        layersNeedingRepaint.append(layer);
        layersChanged = true;
    }
 
    // Add this layer to a squashing backing if needed.
    if (m_layerSquashingEnabled) {
        if (updateSquashingAssignment(layer, squashingState, compositedLayerUpdate, layersNeedingRepaint))
            layersChanged = true;
 
        ......
    }
 
    if (layer->stackingNode()->isStackingContext()) {
        RenderLayerStackingNodeIterator iterator(*layer->stackingNode(), NegativeZOrderChildren);
        while (RenderLayerStackingNode* curNode = iterator.next())
            assignLayersToBackingsInternal(curNode->layer(), squashingState, layersChanged, layersNeedingRepaint);
    }
 
    if (m_layerSquashingEnabled) {
        // At this point, if the layer is to be "separately" composited, then its backing becomes the most recent in paint-order.
        if (layer->compositingState() == PaintsIntoOwnBacking || layer->compositingState() == HasOwnBackingButPaintsIntoAncestor) {
            ASSERT(!requiresSquashing(layer->compositingReasons()));
            squashingState.updateSquashingStateForNewMapping(layer->compositedLayerMapping(), layer->hasCompositedLayerMapping());
        }
    }
 
    RenderLayerStackingNodeIterator iterator(*layer->stackingNode(), NormalFlowChildren | PositiveZOrderChildren);
    while (RenderLayerStackingNode* curNode = iterator.next())
        assignLayersToBackingsInternal(curNode->layer(), squashingState, layersChanged, layersNeedingRepaint);
 
    ......
}
```

​		CompositingLayerAssigner 类的成员函数 assignLayersToBackingsInternal 首先调用成员函数computeCompositedLayerUpdate 计算参数 layer 描述的 Render Layer 的 Compositing State Transition，如下所示：

```c++
CompositingStateTransitionType CompositingLayerAssigner::computeCompositedLayerUpdate(RenderLayer* layer)
{
    CompositingStateTransitionType update = NoCompositingStateChange;
    if (needsOwnBacking(layer)) {
        if (!layer->hasCompositedLayerMapping()) {
            update = AllocateOwnCompositedLayerMapping;
        }
    } else {
        if (layer->hasCompositedLayerMapping())
            update = RemoveOwnCompositedLayerMapping;
 
        if (m_layerSquashingEnabled) {
            if (!layer->subtreeIsInvisible() && requiresSquashing(layer->compositingReasons())) {
                // We can't compute at this time whether the squashing layer update is a no-op,
                // since that requires walking the render layer tree.
                update = PutInSquashingLayer;
            } else if (layer->groupedMapping() || layer->lostGroupedMapping()) {
                update = RemoveFromSquashingLayer;
            }
        }
    }
    return update;
}
```

​		一个 Render Layer 的 Compositing State Transition 分为 4 种：

1. 它需要 Compositing，但是还没有创建 Composited Layer Mapping，这时候 Compositing State Transition 设置为 AllocateOwnCompositedLayerMapping，表示要创建一个新的 Composited Layer Mapping。

2. 它不需要 Compositing，但是之前已经创建有 Composited Layer Mapping，这时  候Compositing State Transition 设置为 RemoveOwnCompositedLayerMapping，表示要删除之前创建的Composited Layer Mapping。

3. 它需要 Squashing，这时候 Compositing State Transition设置为 PutInSquashingLayer，表示要将它绘制离其最近的一个Render Layer的Composited Layer Mapping里面的一个Squashing Layer上。

4. 它不需要 Squashing，这时候 Compositing State Transition设置为 RemoveFromSquashingLayer，表示要将它从原来对应的Squashing Layer上删除。

注意，后面 2 种 Compositing State Transition，只有在 CompositingLayerAssigner 类的成员变量m_layerSquashingEnabled 的值在等于 true 的时候才会进行设置。默认情况下，浏览器是开启 Layer Squashing 机制的，不过可以通过设置 “ disable-layer-squashing ” 选项进行关闭，或者通过设置 “ enable-layer-squashing ” 选项显式开启。

​		判断一个 Render Layer 是否需要 Compositing，是通过调用 CompositingLayerAssigner 类的成员函数 needsOwnBacking 进行的，它的实现如下所示：

```c++
bool CompositingLayerAssigner::needsOwnBacking(const RenderLayer* layer) const
{
    if (!m_compositor->canBeComposited(layer))
        return false;
 
    // If squashing is disabled, then layers that would have been squashed should just be separately composited.
    bool needsOwnBackingForDisabledSquashing = !m_layerSquashingEnabled && requiresSquashing(layer->compositingReasons());
 
    return requiresCompositing(layer->compositingReasons()) || needsOwnBackingForDisabledSquashing || (m_compositor->staleInCompositingMode() && layer->isRootLayer());
}
```

​		CompositingLayerAssigner 类的成员变量 m_compositor 指向的是一个 RenderLayerCompositor 对象，CompositingLayerAssigner 类的成员函数 needsOwnBacking 首先调用它的成员函数canBeComposited 判断参数 layer 描述的 Render Layer 是否需要 Compositing，如下所示：

```c++
bool RenderLayerCompositor::canBeComposited(const RenderLayer* layer) const
{
    // FIXME: We disable accelerated compositing for elements in a RenderFlowThread as it doesn't work properly.
    // See http://webkit.org/b/84900 to re-enable it.
    return m_hasAcceleratedCompositing && layer->isSelfPaintingLayer() && !layer->subtreeIsInvisible() && layer->renderer()->flowThreadState() == RenderObject::NotInsideFlowThread;
}
```

​		一个Render Layer可以Compositing，需要同时满足以下4个条件：

1. 浏览器开启硬件加速合成，即 RenderLayerCompositor 类的成员变量m_hasAcceleratedCompositing 的值等于 true。

2. Render Layer 本身有  Compositing 绘制的需求，也就是调用它的成员函数 isSelfPaintingLayer 得到的返回值为 true。从前面Chromium网页Render Layer Tree创建过程分析一文可以知道，Render Layer 的类型一般为 NormalLayer，但是如果将 overflow 属性设置为“hidden"，那么 Render Layer的类型被设置为 OverflowClipLayer。对于类型为 OverflowClipLayer 的 Render Layer，如果它的内容没有出现 overflow，那么就没有必要对它进行 Compositing。

3. Render Layer 描述的网页内容是可见的，也就是调用它的成员函数 subtreeIsInvisible 得到的返回值等于 false。

4. Render Layer 的内容不是渲染在一个 RenderFlowThread 中，也就是与 Render Layer 关联的 Render Object 的 Flow Thread State 等于 RenderObject::NotInsideFlowThread。从注释可以知道，在RenderFlowThread 中渲染的元素是禁用硬件加速合成的，因为不能正确地使用。RenderFlowThread是 CSS 3 定义的一种元素显示方式，更详细的信息可以参考CSS文档：CSS Regions Module Level 1。

回到 CompositingLayerAssigner 类的成员函数 needsOwnBacking 中，如果 RenderLayerCompositor 类的成员函数 canBeComposited 告诉它参数 layer 描述的 Render Layer 不可进行 Compositing，那么就不需要为它创建一个 Composited Layer Mapping。

​		另一方面，如果 RenderLayerCompositor 类的成员函数 canBeComposited 告诉CompositingLayerAssigner 类的成员函数 needsOwnBacking，参数 layer 描述的 Render Layer 可以进行Compositing，那么 CompositingLayerAssigner 类的成员函数 needsOwnBacking 还需要进一步判断该Render Layer 是否真的需要进行 Compositing。

​		如果参数 layer 描述的 Render Layer 满足以下 3 个条件之一，那么 CompositingLayerAssigner 类的成员函数 needsOwnBacking 就会认为它需要进行 Compositing：

1. Render Layer 的 Compositing Reason 表示它需要 Compositing，这是通过调用前面提到的函数requiresCompositing 判断的。

2. Render Layer 的 Compositing Reason 表示它需要 Squashing，但是浏览器禁用了“Layer Squashing”机制。当浏览器禁用“Layer Squashing”机制时，CompositingLayerAssigner 类的成员变量 m_layerSquashingEnabled 会等于 false。调用前面提到的函数requiresSquashing可以判断一个Render Layer是否需要Squashing。

3. Render Layer 是 Render Layer Tree 的根节点，并且 Render Layer Compositor 处于 Compositing 模式中。除非设置了 Render Layer Tree 的根节点无条件 Compositing，否则的话，当在 Render Layer Tree 根节点的子树中，没有任何 Render Layer 需要 Compositing 时， Render Layer Tree 根节点也不需要 Compositing，这时候 Render Layer Compositor 就会被设置为非 Compositing 模式。判断一个 Render Layer 是否是 Render Layer Tree 的根节点，调用它的成员函数 isRootLayer 即可，而判断一个 Render Layer Compositor 是否处于 Compositing 模式，调用它的成员函数staleInCompositingMode 即可。

回到 CompositingLayerAssigner 类的成员函数 computeCompositedLayerUpdate 中，当它调用结束后，再返回到 CompositingLayerAssigner 类的成员函数 assignLayersToBackingsInternal 中，这时候CompositingLayerAssigner 类的成员函数 assignLayersToBackingsInternal 就知道了参数 layer 描述的Render Layer 的 Compositing State Transition Type。

​		知道了参数 layer 描述的 Render Layer 的 Compositing State Transition Type 之后，CompositingLayerAssigner 类的成员函数 assignLayersToBackingsInternal 接下来调用成员变量m_compositor 描述的一个 RenderLayerCompositor 对象的成员函数allocateOrClearCompositedLayerMapping 为其创建或者删除 Composited Layer Mapping，如下所示：

```c++
bool RenderLayerCompositor::allocateOrClearCompositedLayerMapping(RenderLayer* layer, const CompositingStateTransitionType compositedLayerUpdate)
{
    bool compositedLayerMappingChanged = false;
    ......
 
    switch (compositedLayerUpdate) {
    case AllocateOwnCompositedLayerMapping:
        ......
 
        layer->ensureCompositedLayerMapping();
        compositedLayerMappingChanged = true;
 
        ......
        break;
    case RemoveOwnCompositedLayerMapping:
    // PutInSquashingLayer means you might have to remove the composited layer mapping first.
    case PutInSquashingLayer:
        if (layer->hasCompositedLayerMapping()) {
            ......
 
            layer->clearCompositedLayerMapping();
            compositedLayerMappingChanged = true;
        }
 
        break;
    case RemoveFromSquashingLayer:
    case NoCompositingStateChange:
        // Do nothing.
        break;
    }
 
    ......
 
    return compositedLayerMappingChanged || nonCompositedReasonChanged;
}
```

​		RenderLayerCompositor 类的成员函数 allocateOrClearCompositedLayerMapping 主要是根据Render Layer 的 Compositing State Transition Type 决定是要为其创建 Composited Layer Mapping，还是删除 Composited Layer Mapping。

​		对于 Compositing State Transition Type 等于 AllocateOwnCompositedLayerMapping 的 Render Layer，RenderLayerCompositor 类的成员函 数allocateOrClearCompositedLayerMapping 会调用它的成员函数 ensureCompositedLayerMapping 为其创建一个 Composited Layer Mapping

​		对于等于 RemoveOwnCompositedLayerMapping 或者 PutInSquashingLayer 的 Render Layer，RenderLayerCompositor 类的成员函数 allocateOrClearCompositedLayerMapping 会调用它的成员函数clearCompositedLayerMapping 删除原来为它创建的 Composited Layer Mapping。

​		对于 Compositing State Transition Type 其它值的 Render Layer，则不需要进行特别的处理。

​		这一步执行完成之后，回到 CompositingLayerAssigner 类的成员函数assignLayersToBackingsInternal 中，它接下来判断成员变量  m_layerSquashingEnabled 的值是否等于true。如果等于 true，那么就说明浏览器开启了 " Layer Squashing " 机制。这时候就需要调用成员函数updateSquashingAssignment 判断是否需要将参数 layer 描述的 Render Layer 绘制在一个 Squashing Graphics Layer 中。

​		CompositingLayerAssigner 类的成员函数 updateSquashingAssignment 的实现如下所示：

```c++
bool CompositingLayerAssigner::updateSquashingAssignment(RenderLayer* layer, SquashingState& squashingState, const CompositingStateTransitionType compositedLayerUpdate,
    Vector<RenderLayer*>& layersNeedingRepaint)
{
    ......
    if (compositedLayerUpdate == PutInSquashingLayer) {
        ......
 
        bool changedSquashingLayer =
            squashingState.mostRecentMapping->updateSquashingLayerAssignment(layer, squashingState.mostRecentMapping->owningLayer(), squashingState.nextSquashedLayerIndex);
        ......
 
        return true;
    }
    if (compositedLayerUpdate == RemoveFromSquashingLayer) {
        if (layer->groupedMapping()) {
            ......
            layer->setGroupedMapping(0);
        }
 
        ......
        return true;
    }
 
    return false;
}
```

​		CompositingLayerAssigner 类的成员函数 updateSquashingAssignment 也是根据 Render Layer 的Compositing State Transition Type 决定是否要将它绘制在一个 Squashing Graphics Layer 中，或者将它从一个 Squashing Graphics Layer 中删除。

​		对于 Compositing State Transition Type 等于 PutInSquashingLayer 的 Render Layer，它将会绘制在一个 Squashing Graphics Layer 中。这个 Squashing Graphics Layer 保存在一个 Composited Layer Mapping 中。这个 Composited Layer Mapping 关联的 Render Layer 处于要 Squashing 的 Render Layer的下面，并且前者离后者是最近的，记录在参数 squashingState 描述的一个 SquashingState 对象的成员变量 mostRecentMapping 中。通过调用 CompositedLayerMapping 类的成员函数updateSquashingLayerAssignment 可以将一个 Render Layer 绘制在一个 Composited Layer Mapping内部维护的一个 quashing Graphics Layer 中。

​		对于 Compositing State Transition Type 等于 RemoveFromSquashingLayer 的 Render Layer，如果它之前已经被设置绘制在一个 Squashing Graphics Layer 中，那么就需要将它从这个 Squashing Graphics Layer 中删除。如果一个 Render Layer 之前被设置绘制在一个 Squashing Graphics Layer 中，那么调用它的成员函数 groupedMapping 就可以获得一个 Grouped Mapping。这个 Grouped Mapping 描述的也是一个 Composited Layer Mapping，并且 Render Layer 所绘制在的 Squashing Graphics Layer 就是由这个 Composited Layer Mapping 维护的。因此，要将一个 Render Layer 从一个 Squashing Graphics Layer 中删除，只要将它的 Grouped Mapping 设置为 0 即可。这是通过调用 RenderLayer 类的成员函数setGroupedMapping 实现的。

​		再回到 CompositingLayerAssigner 类的成员函数 assignLayersToBackingsInternal 中，它接下来判断参数 layer 描述的 Render Layer 所关联的 Render Object 是否是一个 Stacking Context。如果是的话，那么就递归调用成员函数 assignLayersToBackingsInternal 遍历那些 z-index 为负数的子 Render Object 对应的 Render Layer，确定是否需要为它们创建 Composited Layer Mapping。

​		到目前为止，CompositingLayerAssigner类的成员函数assignLayersToBackingsInternal就处理完成参数layer描述的Render Layer，以及那些z-index为负数的子Render Layer。这时候，参数layer描述的Render Layer可能会作为那些z-index大于等于0的子Render Layer的Grouped Mapping，因此在继续递归处理z-index大于等于0的子Render Layer之前，CompositingLayerAssigner类的成员函数assignLayersToBackingsInternal需要将参数layer描述的Render Layer对应的Composited Layer Mapping记录下来，前提是这个Render Layer拥有Composited Layer Mapping。这是通过调用参数squashingState描述的一个SquashingState对象的成员函数updateSquashingStateForNewMapping实现的，实际上就是记录在该SquashingState对象的成员变量mostRecentMapping中。这样前面分析的CompositingLayerAssigner类的成员函数updateSquashingAssignment就可以知道将其参数layer描述的Render Layer绘制在哪一个Squashing Graphics Layer中。

​		最后，CompositingLayerAssigne r类的成员函数 assignLayersToBackingsInternal 就递归调用自己处理那些 z-index 大于等于 0 的子 Render Layer。递归调用完成之后，整个 Render Layer Tree 就处理完毕了。这时候哪些 Render Layer 具有 Composited Layer Mapping 就可以确定了。

​		前面分析 RenderLayerCompositor 类的成员函数 allocateOrClearCompositedLayerMapping 时提到，调用 RenderLayer 类的成员函数 ensureCompositedLayerMapping 可以为一个 Render Layer 创建一个 Composited Layer Mapping，接下来我们就继续分析这个函数的实现，以便了解 Composited Layer Mapping 的创建过程。

​		RenderLayer 类的成员函数 ensureCompositedLayerMapping 的实现如下所示：

```c++
CompositedLayerMappingPtr RenderLayer::ensureCompositedLayerMapping()
{
    if (!m_compositedLayerMapping) {
        m_compositedLayerMapping = adoptPtr(new CompositedLayerMapping(*this));
        ......
    }
    return m_compositedLayerMapping.get();
}
```

​		RenderLayer 类的成员变量 compositedLayerMapping 描述的就是一个 Composited Layer Mapping。如果这个 Composited Layer Mapping 还没有创建，那么当 RenderLayer 类的成员函数ensureCompositedLayerMapping 被调用时，就会进行创建。

​		Composited Layer Mapping的创建过程，也就是CompositedLayerMapping类的构造函数的实现，如下所示：

```c++
CompositedLayerMapping::CompositedLayerMapping(RenderLayer& layer)
    : m_owningLayer(layer)
    , ......
{
    ......
 
    createPrimaryGraphicsLayer();
}
```

​		正在创建的Composited Layer Mapping被参数layer描述的Render Layer拥有，因此这个Render Layer将会被保存在在创建的Composited Layer Mapping的成员变量m_owningLayer中。

​		从图5可以知道，一个 Composited Layer Mapping 一定存在一个 Main Graphics Layer。这个 Main Graphics Layer 是 CompositedLayerMapping 类的构造函数通过调用另外一个成员函数createPrimaryGraphicsLayer 创建的，如下所示：

```c++
void CompositedLayerMapping::createPrimaryGraphicsLayer()
{
    m_graphicsLayer = createGraphicsLayer(m_owningLayer.compositingReasons());
 
    ......
}
```

​		Composited Layer Mapping 中的 Main Graphics Layer 由成员变量 m_graphicsLayer 描述，并且这个 Main Graphics Layer 是通过调用成员函数 createGraphicsLayer 创建的。如果我们分析CompositedLayerMapping 类的其它代码，就会发现 Composited Layer Mapping 中的其它 Graphics Layer 也是通过调用成员函数 createGraphicsLayer 创建的。

​		CompositedLayerMapping 类的成员函数 createGraphicsLayer 的实现如下所示：

```c++
PassOwnPtr<GraphicsLayer> CompositedLayerMapping::createGraphicsLayer(CompositingReasons reasons)
{
    GraphicsLayerFactory* graphicsLayerFactory = 0;
    if (Page* page = renderer()->frame()->page())
        graphicsLayerFactory = page->chrome().client().graphicsLayerFactory();
 
    OwnPtr<GraphicsLayer> graphicsLayer = GraphicsLayer::create(graphicsLayerFactory, this);
 
    graphicsLayer->setCompositingReasons(reasons);
    if (Node* owningNode = m_owningLayer.renderer()->generatingNode())
        graphicsLayer->setOwnerNodeId(InspectorNodeIds::idForNode(owningNode));
 
    return graphicsLayer.release();
}
```

​		CompositedLayerMapping 类的成员函数 createGraphicsLayer 在创建一个 Graphics Layer 之前，首先会获得一个 GraphicsLayerFactory 对象。这个 GraphicsLayerFactory 对象是由 WebKit 的使用者提供的。在我们这个情景中，WebKit 的使用者就是 Chromium，它提供的 GraphicsLayerFactory 对象的实际类型为 GraphicsLayerFactoryChromium。

​		获得了 GraphicsLayerFactory 对象之后，CompositedLayerMapping 类的成员函数createGraphicsLayer 接下来就以它为参数，调用 GraphicsLayer 类的静态成员函数 create 创建一个 Graphics Layer，如下所示：

```c++
PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerFactory* factory, GraphicsLayerClient* client)
{
    return factory->createGraphicsLayer(client);
}
```

​		GraphicsLayer 类的静态成员函数 create 调用参数 factory 描述的一个GraphicsLayerFactoryChromium 对象的成员函数 createGraphicsLayer 创建一个 Graphics Layer，如下所示：

```c++
PassOwnPtr<GraphicsLayer> GraphicsLayerFactoryChromium::createGraphicsLayer(GraphicsLayerClient* client)
{
    OwnPtr<GraphicsLayer> layer = adoptPtr(new GraphicsLayer(client));
    ......
    return layer.release();
}
```

​		从这里可以看到，GraphicsLayerFactoryChromium 类的成员函数 createGraphicsLayer 返回的是一个GraphicsLayer 对象。

​		这一步执行完成后，一个 Composited Layer Mapping 及其内部的 Main Graphics Layer 就创建完成了。

​		前面分析 CompositingLayerAssigner 类的成员函数 updateSquashingAssignment 时提到，调用CompositedLayerMapping 类的成员函数 updateSquashingLayerAssignment 可以将一个 Render Layer绘制在其内部维护的一个 Squashing Graphics Layer 中。CompositedLayerMapping 类的成员函数updateSquashingLayerAssignment 的实现如下所示：

```c++
bool CompositedLayerMapping::updateSquashingLayerAssignment(RenderLayer* squashedLayer, const RenderLayer& owningLayer, size_t nextSquashedLayerIndex)
{
	......
	GraphicsLayerPaintInfo paintInfo;
	paintInfo.renderLayer = squashedLayer;
	......
	// Change tracking on squashing layers: at the first sign of something changed, just invalidate the layer.
	// FIXME: Perhaps we can find a tighter more clever mechanism later.
	bool updatedAssignment = false;
	if (nextSquashedLayerIndex < m_squashedLayers.size()) {
		if 	(!paintInfo.isEquivalentForSquashing(m_squashedLayers[nextSquashedLayerIndex])) {
			......
			updatedAssignment = true;
			m_squashedLayers[nextSquashedLayerIndex] = paintInfo;
	}
	} else {
		......
		m_squashedLayers.append(paintInfo);
		updatedAssignment = true;
	}
	squashedLayer->setGroupedMapping(this);
	return updatedAssignment;
}
```

​		CompositedLayerMapping 类有一个成员变量 m_squashedLayers，它描述的是一个类型为GraphicsLayerPaintInfo 的 Vector，如下所示:

```c++
class CompositedLayerMapping FINAL : public GraphicsLayerClient {
WTF_MAKE_NONCOPYABLE(CompositedLayerMapping); WTF_MAKE_FAST_ALLOCATED;
......
private:
......
OwnPtr<GraphicsLayer> m_squashingLayer; // Only used if any squashed layers exist, this is the backing that squashed layers paint into.
Vector<GraphicsLayerPaintInfo> m_squashedLayers;
......
};
```

​		上述 Vector 中保存在的每一个 GraphicsLayerPaintInfo 描述的都是一个 Squashing Render Layer，这些 Squashing Render Layer 最终将会绘制在 CompositedLayerMapping 类的成员变量m_squashingLayer 描述的一个 Graphics Layer 中。

​		CompositedLayerMapping 类的成员函数 updateSquashingLayerAssignment 所做的事情就是将参数squashedLayer 描述的一个 Squashing Render Layer 封装在一个 GraphicsLayerPaintInfo 对象，然后将这个 GraphicsLayerPaintInfo 对象保存在 CompositedLayerMapping 类的成员变量 m_squashedLayers描述的一个 Vector 中。

​		这样，我们就分析完成了为 Render Layer Tree 中的 Render Layer 创建 Composited Layer Mapping的过程，也就是 CompositingLayerAssigner 类的成员函数 assign 的实现。回到 RenderLayerCompositor类的成员函数 updateIfNeeded 中，它接下来调用 GraphicsLayerUpdater 类的成员函数 update 为Composited Layer Mapping 创建 Graphics Layer Sub Tree。

​		GraphicsLayerUpdater 类的成员函数 update 的实现如下所示:

```c++
void GraphicsLayerUpdater::update(Vector<RenderLayer*>& layersNeedingPaintInvalidation, RenderLayer& layer, UpdateType updateType, const UpdateContext& context)
{
	if (layer.hasCompositedLayerMapping()) {
		CompositedLayerMappingPtr mapping = layer.compositedLayerMapping();
		......
		if (mapping->updateGraphicsLayerConfiguration(updateType))
		m_needsRebuildTree = true;
		......
	}
	UpdateContext childContext(context, layer);
	for (RenderLayer* child = layer.firstChild(); child; child = child->nextSibling())
		update(layersNeedingPaintInvalidation, *child, updateType, childContext);
}
```

​		从前面的调用过程可以知道，参数 layer 描述的 Render Layer 是 Render Layer Tree 的根节点，GraphicsLayerUpdater 类的成员函数 update 检查它是否拥有 Composited Layer Mapping。如果有的话，那么就会调用这个 Composited Layer Mapping 的成员函数 updateGraphicsLayerConfiguration 更新它内部的 Graphics Layer Sub Tree。GraphicsLayerUpdater 类的成员函数 update 最后还会递归调用自身遍历Render Layer Tree 的根节点的子孙节点，这样就可以对所有的 Graphics Layer Sub Tree 进行更新。

​		接下来我们继续分析 CompositedLayerMapping 类的成员函数 updateGraphicsLayerConfiguration的实现，以便了解每一个 Graphics Layer Sub Tree 的更新过程，如下所示:

```c++
bool CompositedLayerMapping::updateGraphicsLayerConfiguration(GraphicsLayerUpdater::UpdateType updateType)
{
	......
	bool layerConfigChanged = false;
	......
	// The background layer is currently only used for fixed root backgrounds.
	if (updateBackgroundLayer(m_backgroundLayerPaintsFixedRootBackground))
		layerConfigChanged = true;
	if (updateForegroundLayer(compositor->needsContentsCompositingLayer(&m_owningLayer)))
		layerConfigChanged = true;
	bool needsDescendantsClippingLayer = compositor->clipsCompositingDescendants(&m_owningLayer);
	......
	bool needsAncestorClip = compositor->clippedByNonAncestorInStackingTree(&m_owningLayer);
	......
	if (updateClippingLayers(needsAncestorClip, needsDescendantsClippingLayer))
	layerConfigChanged = true;
	......
	if (updateScrollingLayers(m_owningLayer.needsCompositedScrolling())) {
	layerConfigChanged = true;
	......
}
	bool hasPerspective = false;
	if (RenderStyle* style = renderer->style())
		hasPerspective = style->hasPerspective();
	bool needsChildTransformLayer = hasPerspective && (layerForChildrenTransform() == 	m_childTransformLayer.get()) && renderer->isBox();
	if (updateChildTransformLayer(needsChildTransformLayer))
		layerConfigChanged = true;
	......
	if (updateSquashingLayers(!m_squashedLayers.isEmpty()))
		layerConfigChanged = true;
    if (layerConfigChanged)
		updateInternalHierarchy();
	.......
}
```

​		CompositedLayerMapping 类的成员函数 updateGraphicsLayerConfiguration 依次检查它内部维护的 Background Layer、Foreground Layer、Clip Layer、Scrolling Layer、Child Transform Layer 和Squashing Layer 是否需要更新，也就是创建或者删除等。只要其中的一个 Graphics Layer 发生更新，本地变量 layerConfigChanged 的值就会被设置为true，这时候 CompositedLayerMapping 类的另外一个成员函数u pdateInternalHierarchy 就会被调用来更新内部的 Graphics Layer Sub Tree。

​		接下来我们以 Squashing Layer 的更新过程为例，即 CompositedLayerMapping 类的成员函数updateSquashingLayers 的实现，分析 CompositedLayerMapping 类内部维护的 Graphics Layer 的更新过程，如下所示:

```c++
bool CompositedLayerMapping:: updateSquashingLayers(bool needsSquashingLayers)
{
bool layersChanged = false;
    if (needsSquashingLayers) {
......
        if (!m_squashingLayer) {
            m_squashingLayer = createGraphicsLayer(CompositingReasonLayerForSquashingContents);
......
            layersChanged = true;
        }
        if (m_ancestorClippingLayer) {
            if (m_squashingContainmentLayer) {
                m_squashingContainmentLayer -> removeFromParent();
                m_squashingContainmentLayer = nullptr;
                layersChanged = true;
            }
        } else {
            if (!m_squashingContainmentLayer) {
                m_squashingContainmentLayer = createGraphicsLayer(CompositingReasonLayerForSquashingContainer);
                layersChanged = true;
            }
        }
......
    } else {
        if (m_squashingLayer) {
            m_squashingLayer -> removeFromParent();
            m_squashingLayer = nullptr;
            layersChanged = true;
        }
        if (m_squashingContainmentLayer) {
            m_squashingContainmentLayer -> removeFromParent();
            m_squashingContainmentLayer = nullptr;
            layersChanged = true;
        }
......
    }
    return layersChanged;
}
```

​		从前面的调用过程可以知道，当 CompositedLayerMapping 类的成员变量 m_squashedLayers 描述的Vector 不等于空时，这里的参数 needsSquashingLayers 的值就会等于 true，表示需要对CompositedLayerMappin 类内部维护的 Squashing Layer 进行更新。更新过程如下所示:

1. 如果 Squashing Layer 还没有创建，那么就会调用我们前面分析过的 CompositedLayerMapping 类的成员函数 createGraphicsLayer 进行创建，并且保存在成员变量 m_squashingLayer 中。

2. CompositedLayerMapping 类的成员变量 m_ancestorClippingLayer 描述的是图5所示的 Clip Layer。当存在 Squashing Layer 时，但 Clip Layer 又不存在的时候，需要创建一个 Squashing Containment Layer，用来作为 Squashing Layer 的父 Graphics Layer。否则的话，Squashing Layer 的父 Graphics Layer 就是 Clip Layer。

3. 基于上述第2点，当 Clip Layer 存在时，若 Squashing Containment Layer 存在，则需要将它从Graphics Layer Sub Tree 中移除。另一方面，当 Clip Layer 不存在时，若 Squashing Containment Layer也不存在，则需要创建 Squashing Containment Layer。换句话说，当存在 Squashing Layer时，Clip Layer 和 Squashing Containment Layer至少存在一个，并且只能存在一个，Clip Layer 优先Squashing Containment Layer 存在。

如果 CompositedLayerMapping 类的成员变量 m_squashedLayers 描述的 Vector 等于空时，参数needsSquashingLayers 的值就会等于 false，表示 CompositedLayerMapping 类不需要在内部维护一个Squashing Layer。这时候如果存在 Squashing Layer，那么就需要将它从 Graphics Layer Sub Tree 中移除。如果 Squashing Containment Layer 也存在，那么也要将它一起从 Graphics Layer Sub Tree 中移除。这是因为 Squashing Containment Layer 本来就是为 Squashing Layer 创建的，现在既然 Squashing Layer 不需要了，那么它自然也不再需要了。

​		回到 CompositedLayerMapping 类的成员函数 updateGraphicsLayerConfiguration 中，接下来我们继续分析它调用另外一个成员函数 updateInternalHierarchy 更新内部维护的 Graphics Layer Sub Tree 的过程，如下所示:

```c++
void CompositedLayerMapping:: updateInternalHierarchy()
{
    // m_foregroundLayer has to be inserted in the correct order with child layers,
    // so it's not inserted here.
    if (m_ancestorClippingLayer)
        m_ancestorClippingLayer -> removeAllChildren();
    m_graphicsLayer -> removeFromParent();
    if (m_ancestorClippingLayer)
        m_ancestorClippingLayer -> addChild(m_graphicsLayer.get());
    if (m_childContainmentLayer)
        m_graphicsLayer -> addChild(m_childContainmentLayer.get());
    else if (m_childTransformLayer)
        m_graphicsLayer -> addChild(m_childTransformLayer.get());
    if (m_scrollingLayer) {
        GraphicsLayer * superLayer = m_graphicsLayer.get();
        if (m_childContainmentLayer)
            superLayer = m_childContainmentLayer.get();
        if (m_childTransformLayer)
            superLayer = m_childTransformLayer.get();
        superLayer -> addChild(m_scrollingLayer.get());
    }
    // The clip for child layers does not include space for overflow controls, so they exist as
    // siblings of the clipping layer if we have one. Normal children of this layer are set as
    // children of the clipping layer.
    if (m_layerForHorizontalScrollbar)
        m_graphicsLayer -> addChild(m_layerForHorizontalScrollbar.get());
    if (m_layerForVerticalScrollbar)
        m_graphicsLayer -> addChild(m_layerForVerticalScrollbar.get());
    if (m_layerForScrollCorner)
        m_graphicsLayer -> addChild(m_layerForScrollCorner.get());
    // The squashing containment layer, if it exists, becomes a no-op parent.
    if (m_squashingLayer) {
        ASSERT(compositor() -> layerSquashingEnabled());
        ASSERT((m_ancestorClippingLayer && !m_squashingContainmentLayer) || (!m_ancestorClippingLayer && m_squashingContainmentLayer));
        if (m_squashingContainmentLayer) {
            m_squashingContainmentLayer -> removeAllChildren();
            m_squashingContainmentLayer -> addChild(m_graphicsLayer.get());
            m_squashingContainmentLayer -> addChild(m_squashingLayer.get());
        } else {
            // The ancestor clipping layer is already set up and has m_graphicsLayer under it.
            m_ancestorClippingLayer -> addChild(m_squashingLayer.get());
        }
    }
}
```

​		从这里我们就可以看到，通过调用 GraphicsLayer 类的成员函数 addChild，就可以将一个 Graphics Layer 作为另外一个 Graphics Layer 的子 Graphics Layer，这样就可以形成一个 Graphics Layer Sub Tree。CompositedLayerMapping 类的成员函数 updateInternalHierarchy 构建出来的 Graphics Layer Sub Tree 大致就如图5所示。

​		CompositedLayerMapping 类内部维护的 Graphics Layer Sub Tree，除了前面描述的 Clip Layer 和Squashing Containment Layer 不能并存之外，另外两个 Layer，即 Child Containment Layer 和 Child Tranform Layer，也是不能并存的。这些 Graphics Layer 的具体作用，以及什么在情况下会存在，可以通过阅读 CompositedLayerMapping 类的代码获悉，这里就不再展开描述。

​		还有一点需要注意的是，CompositedLayerMapping 类内部维护的 Background Layer 和 Foreground Layer 也是属于 CompositedLayerMapping 类描述的 Graphics Layer Sub Tree 的一部分，但是它们不是由CompositedLayerMapping 类的成员函数 updateInternalHierarchy 插入到 Graphics Layer Sub Tree 中去的。等到将 Graphics Layer Sub Tree 连接在一起形成整个 Graphics Layer Tree 的时候，它们才会插入到各自的 Graphics Layer Sub Tree 中去，因为处理它们需要更多的信息。

​		由于各个 Graphics Layer Sub Tree 需要连接在一起形成一个完整的 Graphics Layer Tree，因此每一个Graphics Layer Sub Tree 都需要提供两个对外的 Graphics Layer，一个作为其父 Graphics Layer Sub Tree的子 Graphics Layer，另一个作为其子 Graphics Layer Sub Tree 的父 Graphics Layer。CompositedLayerMapping 类提供了两个成员函数 childForSuperlayers 和 parentForSublayers，分别提供上述两个 Graphics Layer。

​		CompositedLayerMapping类的成员函数childForSuperlayers的实现如下所示:

```c++
GraphicsLayer* CompositedLayerMapping::childForSuperlayers() const
{
	if (m_squashingContainmentLayer)
		return m_squashingContainmentLayer.get();
	return localRootForOwningLayer();
}
```

​		从这里可以看到，如果存在 Squashing Containment Layer，那么它就会作为父 Graphics Layer Sub Tree 的子 Graphics Layer。另一方面，如果不存在 Squashing Containment Layer，那么CompositedLayerMapping 类的成员函数 childForSuperlayers调用另外一个成员函数localRootForOwningLayer 返回另外一个 Graphics Layer 作为父 Graphics Layer Sub Tree 的子 Graphics Layer。

​		CompositedLayerMapping 类的成员函数 localRootForOwningLayer 的实现如下所示:

```c++
GraphicsLayer* CompositedLayerMapping::localRootForOwningLayer() const
{
	if (m_ancestorClippingLayer)
		return m_ancestorClippingLayer.get();
	return m_graphicsLayer.get();
}
```

​		从这里可以看到，如果存在 Clip Layer，那么它就会作为父 Graphics Layer Sub Tree 的子 Graphics Layer。否则的话，Main Layer 就会作为父 Graphics Layer Sub Tree 的子 Graphics Layer。从图5可以知道，Main Layer 是一定会存在的，因此就一定可以找到一个 Graphics Layer，作为父 Graphics Layer Sub Tree 的子 Graphics Layer。

​		CompositedLayerMapping 类的成员函数 parentForSublayers 的实现如下所示:

```c++
GraphicsLayer* CompositedLayerMapping::parentForSublayers() const
{
	if (m_scrollingBlockSelectionLayer)
		return m_scrollingBlockSelectionLayer.get();
	if (m_scrollingContentsLayer)
		return m_scrollingContentsLayer.get();
	if (m_childContainmentLayer)
		return m_childContainmentLayer.get();
	if (m_childTransformLayer)
		return m_childTransformLayer.get();
	return m_graphicsLayer.get();
}
```

​		CompositedLayerMapping 类的成员函数 parentForSublayers 按照 Scrolling Block Selection Layer、Scrolling Contents Layer、Child Containment Layer、Child Transform Layer 和 Main Layer 的顺序检查，先检查的 Graphics Layer 若存在，那么它就优先作为子 Graphics Layer Sub Tree 的父Graphics Layer。同样，由于最后检查的 Main Layer 是一定存在的，因此就一定可以找到一个 Graphics Layer，作为子 Graphics Layer Sub Tree 的父 Graphics Layer。

​		了解了 Graphics Layer Sub Tree 的构建过程之后，回到 RenderLayerCompositor 类的成员函数updateIfNeeded 中，它最后就可以调用 GraphicsLayerTreeBuilder 类的成员函数 rebuild 将所有的Graphics Layer Sub Tree 连接起来形成一个完整的 Graphics Layer Tree 了。

​		GraphicsLayerTreeBuilder 类的成员函数 rebuild 的实现如下所示:

```c++
void GraphicsLayerTreeBuilder::rebuild(RenderLayer& layer, GraphicsLayerVector& childLayersOfEnclosingLayer) {
	......
	const bool hasCompositedLayerMapping = layer.hasCompositedLayerMapping();
	CompositedLayerMappingPtr currentCompositedLayerMapping = layer.compositedLayerMapping();
	// If this layer has a compositedLayerMapping, then that is where we place subsequent children GraphicsLayers.
	// Otherwise children continue to append to the child list of the enclosing layer.
	GraphicsLayerVector layerChildren;
	GraphicsLayerVector& childList = hasCompositedLayerMapping ? layerChildren : childLayersOfEnclosingLayer;
	......
	if (layer.stackingNode()->isStackingContext()) {
		RenderLayerStackingNodeIterator iterator(*layer.stackingNode(), NegativeZOrderChildren);
		while (RenderLayerStackingNode* curNode = iterator.next())
		rebuild(*curNode->layer(), childList);
		// If a negative z-order child is compositing, we get a foreground layer which needs to get parented.
		if (hasCompositedLayerMapping && currentCompositedLayerMapping->foregroundLayer())
		childList.append(currentCompositedLayerMapping->foregroundLayer());
	}
	RenderLayerStackingNodeIterator iterator(*layer.stackingNode(), NormalFlowChildren | PositiveZOrderChildren);
	while (RenderLayerStackingNode* curNode = iterator.next())
	rebuild(*curNode->layer(), childList);
	if (hasCompositedLayerMapping) {
		bool parented = false;
		if (layer.renderer()->isRenderPart())
		parented = RenderLayerCompositor::parentFrameContentLayers(toRenderPart(layer.renderer()));
		if (!parented)
		currentCompositedLayerMapping->parentForSublayers()->setChildren(layerChildren);
		......
		if (shouldAppendLayer(layer))
		childLayersOfEnclosingLayer.append(currentCompositedLayerMapping->childForSuperlayers());
	}
}
```

​		从前面的调用过程可以知道，参数 layer 描述的是网页 Render Layer Tree 的根节点，另外一个参数childLayersOfEnclosingLayer 是一个输出参数，用来保存参数 layer 描述的 Render Layer 的子 Render Layer 的 Child For Super Layers，也就是前面描述的一个 Composited Layer Mapping 中作为父 Graphics Layer Sub Tree 的子 Graphics Layer。

​		GraphicsLayerTreeBuilder 类的成员函数 rebuild 根据 Stacking Context 顺序从 Render Layer Tree的根节点开始，不断地递归调用自己，不过只会处理那些具有 Composited Layer Mapping 的 Render Layer。对于那些不具有 Composited Layer Mapping 的 Render Layer，仅仅是用作跳板找到具有Composited Layer Mapping 的 Render Layer。这一点容易理解，因为 GraphicsLayerTreeBuilder 类的成员函数 rebuild 是用来构建整个网页的 Graphics Layer Tree 的，只有具有 Composited Layer Mapping 的Render Layer 才对应的 Graphics Layer。

​		GraphicsLayerTreeBuilder 类的成员函数 rebuild 处理具有 Composited Layer Mapping 的 Render Layer 的过程如下所示:

1. 收集z-index为负数的子Render Layer的Child For Super Layers，并且保存在本地变量layerChildren描述的一个Vector中。

2. 如果正在处理的Render Layer具有z-index为负数的子Render Layer，那么根据前面的分析可以知道，正在处理的Render Layer的Composited Layer Mapping内部有一个Foreground Layer。这个Foreground Layer也会保存在本地变量layerChildren描述的一个Vector中，并且是位于那些z-index为负数的子Render Layer的Child For Super Layers之后。这就是为什么Foreground Layer不是由CompositedLayerMapping类的成员函数updateInternalHierarchy直接插入到Graphics Layer Sub Tree去的原因，因为CompositedLayerMapping类不知道一个Render Layer有哪些z-index为负数的子Render Layer。

3. 收集z-index为0和正数的子Render Layer的Child For Super Layers，并且保存在本地变量layerChildren描述的一个Vector中。

4. 经过前面三个收集操作，当前正在处理的Render Layer的Foreground Layer，以及它所有的子Render Layer的Child For Super Layers，就都保存在了本地变量layerChildren描述的一个Vector中。这时候只要找到当前正在处理的Render Layer的Parent For Sub Layers，再将前者作为后者的Children，就可以将具有父子关系的Graphics Layer Sub Tree连接起来。从前面的分析可以知道，当前正在处理的Render Layer的Parent For Sub Layers，可以通过调用它的Composited Layer Mapping的成员函数parentForSublayers获得。

5. 当前正在处理的Render Layer的Child For Super Layers，要保存在参数childLayersOfEnclosingLayer描述的Vector中，以便作为其父Render Layer的Parent For Sub Layers的Children。

其中，第4步对应的代码为:

```c++
bool parented = false;
if (layer.renderer()->isRenderPart())
	parented = RenderLayerCompositor::parentFrameContentLayers(toRenderPart(layer.renderer()));
if (!parented)
	currentCompositedLayerMapping->parentForSublayers()->setChildren(layerChildren);
```

​		它的执行有一个前提条件，就是本地变量parented的值为false。这是什么意思呢?当一个Render Layer的宿主Render Object对应的HTML Element是一个frame/iframe或者embed标签时，该Render Object是从RenderPart类继承下来的，称为Render Part。Render Part可能会具有自己的Render Layer Compositor，这可以通过调用RenderLayerCompositor类的静态成员函数parentFrameContentLayers进行判断。如果一个Render Part具有自己的Render Layer Compositor，那么它的子Render Object就由这个Render Layer Compositor进行具体的绘制，绘制好之后再交给Render Part的父Render Object对应的Render Layer Compositor进行合成。因此，在这种情况下，Render Part的子Render Object所对应的Graphics Layer就不会插入在Render Part的父Render Object所对应的Graphics Layer Tree中。从另外一个角度理解就是，每一个Render Layer Compositor都有一个Graphics Layer Tree，而一个Graphics Layer不能同时位于两个Graphics Layer Tree中。

第5步对应的代码为:

```c++
if (shouldAppendLayer(layer))
	childLayersOfEnclosingLayer.append(currentCompositedLayerMapping->childForSuperlayers());
```

​		它的执行也有一个前提条件，就是当前正在处理的Render Layer对应的Graphcis Layer需要插入Graphics Layer Tree的时候才会执行，这可以通过调用函数shouldAppendLayer进行判断，如下所示:

```c++
static bool shouldAppendLayer(const RenderLayer& layer) {
	if (!RuntimeEnabledFeatures::overlayFullscreenVideoEnabled())
	return true;
	Node* node = layer.renderer()->node();
	if (node && isHTMLMediaElement(*node) && toHTMLMediaElement(node)->isFullscreen())
	return false;
	return true;
}
```

​		当一个 Render Layer 的宿主 Render Object 对应的 HTML Element 是一个 audio 或者 video 标签时，并且它们是全屏播放、以及浏览器允许全屏播放时，那么它对应的 Graphcis Layer 就不需要插入 Graphics Layer Tree 中去，因为毕竟就只有它是需要渲染的，其他的网页内容都是不可见的。

​		GraphicsLayerTreeBuilder 类的成员函数 rebuild 递归执行完成后，网页的 Graphics Layer Tree 就创建完成了。不过细心的读者会发现，还有一种类型的 Graphics Layer 还没有被插入到 Graphics Layer Tree 中去，就是图5所示的 Background Layer。

​		前面提到，只有根 Render Layer 的 Composited Layer Mapping，才可能存在 Background Layer。也就是只有当 body 标签的 CSS 属性 background-attachment 被设置为 “fixed” 时，根 Render Layer 的Composited Layer Mapping 才会存在 Background Layer。其他的 Render Layer 的 Composited Layer Mapping，都不可能存在 Background Layer。

​		前面还提到，一个Graphics Layer Tree是由一个Render Layer Compositor进行管理。Render Layer Compositor内部也维护有一个Graphics Layer Sub Tree，充当根Graphics Layer Sub Tree的角色。这个Graphics Layer Sub Tree的结构如下所示:

```
+Overflow Controls Host Layer
+Container Layer
+Background Layer
+Scroll Layer
+Root Content Layer
```

​		其中，GraphicsLayerTreeBuilder类的成员函数rebuild构建的Graphics Layer Tree的根节点是Root Content Layer，而Container Layer充当图5所示的Clip Layer的角色，这时候Background Layer就作为它的子Graphics Layer。Render Layer Compositor对应的Graphics Layer Sub Tree是由RenderLayerCompositor类的成员函数ensureRootLayer构建的，如下所示:

```c++
void RenderLayerCompositor::ensureRootLayer() {
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
}
```

​		当网页的 body 标签的 CSS 属性 background-attachment 被设置为“fixed”时，RenderLayerCompositor 类的成员函数 rootFixedBackgroundsChanged 就会被调用，用来插入Background Layer，如下所示:

```c++
void RenderLayerCompositor::rootFixedBackgroundsChanged()
{
	......
	if (GraphicsLayer* backgroundLayer = fixedRootBackgroundLayer())
		m_containerLayer->addChildBelow(backgroundLayer, m_scrollLayer.get());
}
```

​		RenderLayerCompositor 类的成员函数 rootFixedBackgroundsChanged 调用另外一个成员函数fixedRootBackgroundLayer 获得 Background Layer，然后再将它作为上述 Container Layer 的 Child，并且位于 Scroll Layer 的下面。

​		这样，我们就分析完成网页 Graphics Layer Tree 的构建过程了，它是通过连接 Graphics Layer Sub Tree 得来的。每一个 Graphics Layer Sub Tree 又是由一个 Composited Layer Mapping 维护的。每一个需要 Compositing 的 Render Layer 都具有一个 Composited Layer Mapping。这就意味着网页的 Graphics Layer Tree 是根据 Render Layer Tree 的内容构建的，并且 Render Layer 和 Graphics Layer 是多对一的关系。

​		至此，我们就学习完成Chromium网页加载过程这个系列的文章，重新学习可以参考Chromium网页加载过程简要介绍和学习计划一文。这个过程主要是由WebKit完成的，一共构建了五个Tree，分别为Frame Tree、DOM Tree、Render Object Tree、Render Layer Tree和Graphics Layer Tree。其中，最终输出给Chromium的是Graphics Layer Tree。有了Graphics Layer Tree之后，Chromium就可以绘制/渲染网页的UI了

































