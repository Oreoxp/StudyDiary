[TOC]

# 网页 Layer Tree 创建过程分析

​		在 Chromium 中，WebKit 会创建一个 Graphics Layer Tree 描述网页。Graphics Layer Tree 是和网页渲染相关的一个 Tree。网页渲染最终由 Chromium 的 CC 模块完成，因此 CC 模块又会根据 Graphics Layer Tree 创建一个 Layer Tree，以后就会根据这个 Layer Tree 对网页进行渲染。本文接下来就分析网页 Layer Tree 的创建过程。

​		从前面Chromium网页Graphics Layer Tree创建过程分析一文可以知道，网页的 Graphics Layer Tree 是根据 Render Layer Tree 创建的，Render Layer Tree 又是根据 Render Object Tree 创建的。Graphics Layer Tree 与 Render Layer Tree、Render Layer Tree与  Render Object Tree的节点是均是一对多的关系，然而 Graphics Layer Tree 与 CC 模块创建的 Layer Tree 的节点是**一一对应的关系**，如图1所示：

![img](markdownimage/20160323014354693)

​		也就是说，每一个 Graphics Layer 都对应有一个 CC Layer。不过，Graphics Layer 与 CC Layer 不是直接的一一对应的，它们是透过另外两个 Layer 才对应起来的，如图2所示：

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



















