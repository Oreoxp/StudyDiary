

[TOC]

# Chromium 网页 DOM Tree 创建过程分析

​		在 Chromium 中，Render 进程是通过 Browser 进程下载网页内容的，后者又是通过共享内存将下载回来的网页内容交给前者的。Render 进程获得网页内容之后，会交给 WebKit 进行处理。WebKit 所做的第一个处理就是对网页内容进行解析，解析的结果是得到一棵 DOM Tree。DOM Tree 是网页的一种结构化描述，也是网页渲染的基础。本文接下来就对网页 DOM Tree 的创建过程进行详细分析

​		网页的 DOM Tree 的根节点是一个 Document。Document 是依附在一个 DOM Window 之上。DOM Window 又是和一个 Frame 关联在一起的。Document、DOM Window 和 Frame 都是 WebKit 里面的概念，其中 Frame 又是和 Chromium 的 Content 模块中的 Render Frame 相对应的。Render Frame 是和网页的 Frame Tree 相关的一个概念。关于网页的 Frame Tree，可以参考前面 Chromium Frame Tree 创建过程分析一文。

​		上面描述的各种对象的关系可以通过图1描述，如下所示：

![img](markdownimage/20160122015711070)

​		从前面 Chromium Frame Tree 创建过程分析一文可以知道，有的 Render Frame 只是一个 Proxy，称为 Render Frame Proxy。Render Frame Proxy 描述的是在另外一个 Render 进程中进行加载和渲染的网页。这种网页在 WebKit 里面对应的 Frame 和 DOM Window 分别称为 Remote Frame 和 Remote DOM Window。由于 Render Frame Proxy 描述的网页不是在当前 Render 进程中加载和渲染，因此它是没有Document 的。

​		相应地，Render Frame 描述的是在当前 Render 进程中进行加载和渲染的网页，它是具有 Document的，并且这种网页在 WebKit 里面对应的 Frame 和 DOM Window 分别称为 Local Frame 和 Local DOM Window。

​		从图1我们还可以看到，在 Render Frame 和 Local Frame 之间，以及 Render Frame Proxy 和 Remote Frame 之间，分别存在一个 Web Local Frame 和 Web Remote Frame。Web Local Frame 和 Web Remote Frame 是属于 WebKit Glue 层的概念。从前面 Chromium 网页加载过程简要介绍和学习计划一文可以知道，WebKit Glue 层的作用是将 WebKit 的对象类型转化为 Chromium 的对象类型，这样 Chromium 的Content 层就可以用统一的、自有的方式管理所有的对象。关于 Chromium 的层次划分和每一个层次的作用，可以参考前面 Chromium 网页加载过程简要介绍和学习计划一文。

​		除了根节点，也就是 Document 节点，**<u>DOM Tree 的每一个子结点对应的都是网页里面的一个 HTML 标签。并不是所有的 HTML 标签都是需要渲染的，例如 script 标签就不需要进行渲染。对于需要渲染的 HTML标签，它们会关联有一个 Render Object。这些 Render Object 会形成一个 Render Object Tree</u>**，如图2所示：

![img](markdownimage/20160131191258205)

​		**<u>为了便于执行绘制操作，具有相同坐标空间的 Render Object 会绘制在同一个 Render Layer 中。这些Render Layer又会形成一个Render Layer Tree</u>**。<u>绘制操作是由图形渲染引擎执行的。对于图形渲染引擎来说，Layer 是一个具有后端存储的概念。在软件渲染模式中，Layer 的后端存储实际上就是一个内存缓冲区。在硬件渲染模式中，Layer 的后端存储实际上就是一个 FBO。为了节约资源，WebKit 不会为每一个 Render Layer 都分配一个后端存储，而是会让某些 Render Layer 共用其它的 Render Layer 的后端存储。那些具有自己的后端存储的 Render Layer，又称为 Graphics Layer。这些 Graphics Layer 又形成了一个 Graphics Layer Tree。</u>

​		Render Object Tree、Render Layer Tree 和 Graphics Layer Tree 都是和网页渲染相关概念，它们是从DOM Tree 发展而来的。因此，在分析网页的渲染机制之前，有必要了解网页的 DOM Tree 的创建过程。

​		DOM Tree的创建发生在WebKit解析网页内容的过程中。WebKit在解析网页内容的时候，会用到一个栈。每当碰到一个HTML标签的起始Token，就会将其压入栈中，而当碰到该HTML标签的结束Token时，就会将其弹出栈。在这些HTML标签的压栈和出栈过程中，就可以得到一棵DOM Tree。以图2所示的DOM Tree片段为例，它对应的网页内容为：

```html
<div>
    <p>
        <div></div>
    </p>
    <span></span>
</div>
```

各个标签的压栈和出栈过程如图3所示：

![img](markdownimage/20160122024318511)

​		接下来，我们就结合源码分析 WebKit 在解析网页内容的过程中创建 DOM Tree 的过程。从前面Chromium 网页 URL 加载过程分析一文可以知道，**Browser 进程一边下载网页的内容，一边将下载回来的网页交给 Render 进程的 Content 模块。Render 进程的 Content 模块经过简单的处理之后，又会交给 WebKit进行解析**。WebKit 是从 ResourceLoader 类的成员函数 didReceiveData 开始接收 Chromium 的 Content 模块传递过来的网页内容的，因此我们就从这个函数开始分析 WebKit 解析网页内容的过程，也就是网页 DOM Tree 的创建过程。

​		ResourceLoader 类的成员函数 didReceiveData 的实现如下所示：

```c++
void ResourceLoader::didReceiveData(blink::WebURLLoader*, const char* data, int length, int encodedDataLength)
{
    ......
 
    m_resource->appendData(data, length);
}
```

​		ResourceLoader 类的成员变量 m_resource 描述的是一个 RawResource 对象。这个 RawResource 对象的创建过程可以参考前面 Chromium 网页 URL 加载过程分析一文。ResourceLoader 类的成员函数didReceiveData 调用这个 RawResource 对象的成员函数 appendData 处理下载回来的网页内容。

​		RawResource 类的成员函数 appendData 的实现如下所示：

```c++
void RawResource::appendData(const char* data, int length)
{
    Resource::appendData(data, length);
 
    ResourcePtr<RawResource> protect(this);
    ResourceClientWalker<RawResourceClient> w(m_clients);
    while (RawResourceClient* c = w.next())
        c->dataReceived(this, data, length);
}
```

​		RawResource 类的成员函数 appendData 主要是调用保存在成员变量 m_clients 中的每一个RawResourceClient 对象的成员函数 dataReceived，告知它们从 Web 服务器中下载回来了新的数据。

​		从前面 Chromium 网页 URL 加载过程分析一文可以知道，在 RawResource 类的成员变量 m_clients中，保存有一个 DocumentLoader 对象。这个 DocumentLoader 对象是从 RawResourceClient 类继承下来的，它负责创建和加载网页的文档对象。接下来我们就继续分析它的成员函数 dataReceived 的实现，如下所示：

```c++
void DocumentLoader::dataReceived(Resource* resource, const char* data, int length)
{
    .....
 
    commitData(data, length);
 
    ......
}
```

​		DocumentLoader 类的成员函数 dataReceived 主要是调用另外一个成员函数 commitData 处理从 Web服务器下载回来的网页数据，后者的实现如下所示：

```c++
void DocumentLoader::commitData(const char* bytes, size_t length)
{
    ensureWriter(m_response.mimeType());
    ......
    m_writer->addData(bytes, length);
}
```

​		DocumentLoader 类的成员函数 commitData 首先调用成员函数 ensureWriter 确定成员变量m_writer 指向了一个 DocumentWriter 对象，因为接下来要调用这个 DocumentWriter 对象的成员函数addData 对下载回来的网页数据进行解析。

​		接下来，我们首先分析 DocumentLoader 类的成员函数 ensureWriter 的实现，接下来再分析DocumentWriter 类的成员函数 addData 的实现。

​		DocumentLoader 类的成员函数 ensureWriter 的实现如下所示：

```c++
void DocumentLoader::ensureWriter(const AtomicString& mimeType, const KURL& overridingURL)
{
    if (m_writer)
        return;
 
    ......
    m_writer = createWriterFor(m_frame, 0, url(), mimeType, encoding, false, false);
    
    ......
}
```

​		DocumentLoader 类的成员函数 ensureWriter 首先检查成员变量 m_writer 是否指向了一个DocumentWriter 对象。如果已经指向，那么就什么也不用做就直接返回。否则的话，就会调用另外一个成员函数 createWriterFor 为当前正在加载的 URL 创建一个 DocumentWriter 对象，并且保存在成员变量m_writer 中。 

​		DocumentLoader 类的成员函数 createWriterFor 的实现如下所示：

```c++
PassRefPtrWillBeRawPtr<DocumentWriter> DocumentLoader::createWriterFor(LocalFrame* frame, const Document* ownerDocument, const KURL& url, const AtomicString& mimeType, const AtomicString& encoding, bool userChosen, bool dispatch)
{
    ......
 
    // In some rare cases, we'll re-used a LocalDOMWindow for a new Document. For example,
    // when a script calls window.open("..."), the browser gives JavaScript a window
    // synchronously but kicks off the load in the window asynchronously. Web sites
    // expect that modifications that they make to the window object synchronously
    // won't be blown away when the network load commits. To make that happen, we
    // "securely transition" the existing LocalDOMWindow to the Document that results from
    // the network load. See also SecurityContext::isSecureTransitionTo.
    bool shouldReuseDefaultView = frame->loader().stateMachine()->isDisplayingInitialEmptyDocument() && frame->document()->isSecureTransitionTo(url);
    ......
 
    if (!shouldReuseDefaultView)
        frame->setDOMWindow(LocalDOMWindow::create(*frame));
 
    RefPtrWillBeRawPtr<Document> document = frame->domWindow()->installNewDocument(mimeType, init);
    ......
 
    return DocumentWriter::create(document.get(), mimeType, encoding, userChosen);
}
```

​		从前面的调用过程可以知道，参数 frame 描述的 LocalFrame 对象来自于 DocumentLoader 类的成员变量 m_frame，这个 LocalFrame 对象描述的是一个在当前 Render 进程中进行加载的网页。

​		如果当前正在加载的网页是通过 JavaScript 接口 window.open 打开的，那么参数 frame 描述的LocalFrame 对象已经关联有一个默认的 DOM Window。在符合安全规则的情况下，这个默认的 DOM Window 将会被使用。如果不符合安全规则，或者当前加载的网页不是通过 JavaScript 接口 window.open 打开的，那么就需要为参数 frame 描述的 LocalFrame 对象创建一个新的 DOM Window。这是通过调用LocalDOMWindow 类的静态成员函数 create 创建的，如下所示：

```c++
namespace WebCore {
    ......
 
    class LocalDOMWindow FINAL : public RefCountedWillBeRefCountedGarbageCollected<LocalDOMWindow>, public ScriptWrappable, public EventTargetWithInlineData, public DOMWindowBase64, public FrameDestructionObserver, public WillBeHeapSupplementable<LocalDOMWindow>, public LifecycleContext<LocalDOMWindow> {
        ......
 
        static PassRefPtrWillBeRawPtr<LocalDOMWindow> create(LocalFrame& frame)
        {
            return adoptRefWillBeRefCountedGarbageCollected(new LocalDOMWindow(frame));
        }
 
        ......
    };
 
    ......
}
```

​		LocalDOMWindow 类的静态成员函数 create 为参数 frame 指向的一个 LocalFrame 对象创建的是一个类型为 LocalDOMWindow 的 DOM Window，这是因为参数 frame 指向的 LocalFrame 对象描述的是一个在当前 Render 进程加载的网页。

​		回到 DocumentLoader 类的成员函数 createWriterFor 中，它调用 LocalDOMWindow 类的静态成员函数 create 创建了一个 LocalDOMWindow 对象之后，会将这个 LocalDOMWindow 对象设置给参数 frame 描述的 LocalFrame 对象。这是通过调用 LocalFrame 类的成员函数 setDOMWindow 实现的。

​		LocalFrame 类的成员函数 setDOMWindow 的实现如下所示：

```c++
void LocalFrame::setDOMWindow(PassRefPtrWillBeRawPtr<LocalDOMWindow> domWindow)
{
    ......
    Frame::setDOMWindow(domWindow);
}
```

​		LocalFrame 类的成员函数 setDOMWindow 会将参数 domWindow 描述的一个 LocalDOMWindow 对象交给父类 Frame 处理，这是通过调用父类 Frame 的成员函数 setDOMWindow 实现的。

​		Frame 类的成员函数 setDOMWindow 的实现如下所示：

```c++
void Frame::setDOMWindow(PassRefPtrWillBeRawPtr<LocalDOMWindow> domWindow)
{
    ......
    m_domWindow = domWindow;
}
```

​		Frame 类的成员函数 setDOMWindow 将参数domWindow描述的一个LocalDOMWindow对象保存在成员变量m_domWindow中。以后就可以通过调用Frame类的成员函数domWindow获得这个LocalDOMWindow对象，如下所示：

```c++
inline LocalDOMWindow* Frame::domWindow() const
{
    return m_domWindow.get();
}
```

​		这一步执行完成之后，WebKit 就为一个类型为 LocaFrame 的 Frame 创建了一个类型为LocalDOMWindow 的 DOM Window，正如图1所示。回到 DocumentLoader 类的成员函数createWriterFor 中，接下来它会继续为上面创建的类型为 LocalDOMWindow 的 DOM Window 创建一个Document。这是通过调用 LocalDOMWindow 类的成员函数 installNewDocument 实现的，如下所示：

```c++
PassRefPtrWillBeRawPtr<Document> LocalDOMWindow::installNewDocument(const String& mimeType, const DocumentInit& init, bool forceXHTML)
{
    ......
 
    m_document = createDocument(mimeType, init, forceXHTML); 
    ......
    m_document->attach();
 
    ......
}
```

​		LocalDOMWindow 类的成员函数 installNewDocument 首先调用另外一个成员函数 createDocument创建一个 HTMLDocument 对象，并且保存在成员变量 m_document 中，接下来又调用这个HTMLDocument 对象的成员函数 attach 为其创建一个 Render View 。这个 Render View 即为图2所示的Render Object Tree 的根节点。

​		接下来我们首先分析 LocalDOMWindow 类的成员函数 createDocument 的实现，接着再分析HTMLDocument 类的成员函数 attach 的实现。

​		LocalDOMWindow类的成员函数createDocument的实现如下所示：

```c++
PassRefPtrWillBeRawPtr<Document> LocalDOMWindow::createDocument(const String& mimeType, const DocumentInit& init, bool forceXHTML)
{
    RefPtrWillBeRawPtr<Document> document = nullptr;
    if (forceXHTML) {
        // This is a hack for XSLTProcessor. See XSLTProcessor::createDocumentFromSource().
        document = Document::create(init);
    } else {
        document = DOMImplementation::createDocument(mimeType, init, init.frame() ? init.frame()->inViewSourceMode() : false);
        ......
    }
 
    return document.release();
}
```

​		当参数 forceXHTML 的值等于 true 的时候，表示当前加载的网页的 MIME Type 为 **“text/plain”**，这时候 LocalDOMWindow 类的成员函数 createDocument 调用 Document 类的静态成员函数 create 为其创建一个类型 Document 的 document。 我们考虑当前加载的网页的 MIME Type 为 **"text/html"**，这时候LocalDOMWindow 类的成员函数 createDocument 调用 DOMImplementation 类的成员函数createDocument 为当前正在加载的网页创建一个类型为 HTMLDocument 的 Document。

​		DOMImplementation 类的成员函数 createDocument 的实现如下所示：

```c++
PassRefPtrWillBeRawPtr<Document> DOMImplementation::createDocument(const String& type, const DocumentInit& init, bool inViewSourceMode)
{
    ......
 
    if (type == "text/html")
        return HTMLDocument::create(init);
 
    ......
}
```

​		从这里可以看到，如果当前正在加载的网页的 MIME Type 为 "text/html"，那么 DOMImplementation 类的成员函数 createDocument 就会调用 HTMLDocument 类的静态成员函数create 创建一个 Document。

​		HTMLDocument 类的静态成员函数 create 的实现如下所示：

```c++
class HTMLDocument : public Document, public ResourceClient {
public:
    static PassRefPtrWillBeRawPtr<HTMLDocument> create(const DocumentInit& initializer = DocumentInit())
    {
        return adoptRefWillBeNoop(new HTMLDocument(initializer));
    }
  
    ......
};
```

​		从这里可以看到，HTMLDocument 类的静态成员函数 create 创建的 Document 的类型为HTMLDocument。

​		回到 LocalDOMWindow 类的成员函数 installNewDocument 中，它调用成员函数 createDocument 创建了一个 HTMLDocument 对象之后，接下来会调用这个 HTMLDocument 对象的成员函数 attach 为其创建一个 Render View。

​		HTMLDocument 类的成员函数 attach 是从父类 Document 继承下来的，它的实现如下所示：

```c++
void Document::attach(const AttachContext& context)
{
    ......
 
    m_renderView = new RenderView(this);
    setRenderer(m_renderView);
 
    ......
}
```

​		Document 类的成员函数 attach 首先是创建了一个 RenderView 对象保存在成员变量 m_renderView中。这个 RenderView 对象就是图2所示的 Render Object Tree 的根节点。接下来又调用另外一个成员函数setRenderer 将上述 RenderView 对象作为与当前正在处理的 Document 对象对应的 Render Object。以后我们分析网页的渲染过程时，再详细分析 Render View 的作用。

​		这一步执行完成之后，WebKit 就为一个类型为 LocalDOMWindow 的 DOM Window 创建了一个类型为HTMLDocument 的 Document，正如图1所示。回到 DocumentLoader 类的成员函数 createWriterFor中，它最后调用 DocumentWriter 类的静态成员函数 create 为前面创建的类型为 HTMLDocument 的Document 创建一个 DocumentWriter 对象。这个 DocumentWriter 对象负责解析从 Web 服务器下载回来的网页数据。

​		DocumentWriter 类的静态成员函数 create 的实现如下所示：

```c++
PassRefPtrWillBeRawPtr<DocumentWriter> DocumentWriter::create(Document* document, const AtomicString& mimeType, const AtomicString& encoding, bool encodingUserChoosen)
{
    return adoptRefWillBeNoop(new DocumentWriter(document, mimeType, encoding, encodingUserChoosen));
}
```

​		从这里可以看到，DocumentWriter 类的静态成员函数 create 创建的是一个 DocumentWriter 对象。这个 DocumentWriter 对象的创建过程，即 DocumentWriter 类的构造函数的实现，如下所示：

```c++
DocumentWriter::DocumentWriter(Document* document, const AtomicString& mimeType, const AtomicString& encoding, bool encodingUserChoosen)
    : m_document(document)
    , ......
    , m_parser(m_document->implicitOpen())
{
    ......
}
```

​		DocumentWriter 类的构造函数首先将参数 document 描述的 HTMLDocument 对象保存在成员变量m_document 中，接下来又调用这个 HTMLDocument 对象的成员函数 implicitOpen 创建了一个HTMLDocumentParser 对象。这个 HTMLDocumentParser 对象就是用来解析从 Web 服务器下载回来网页数据的。

​		HTMLDocument 类的成员函数 implicitOpen 是从父类 Document 继承下来的，它的实现如下所示：

```c++
PassRefPtrWillBeRawPtr<DocumentParser> Document::implicitOpen()
{
    ......
 
    m_parser = createParser();
    ......
 
    return m_parser;
}
```

​		Document 类的成员函数 implicitOpen 调用另外一个成员函数 createParser 建了一个HTMLDocumentParser 对象保存在成员变量 m_parser 中，并且这个 HTMLDocumentParser 对象会返回给调用者。

​		Document 类的成员函数 createParser 的实现如下所示：

```c++
PassRefPtrWillBeRawPtr<DocumentParser> Document::createParser()
{
    if (isHTMLDocument()) {
        bool reportErrors = InspectorInstrumentation::collectingHTMLParseErrors(page());
        return HTMLDocumentParser::create(toHTMLDocument(*this), reportErrors);
    }
    ......
}
```

​		由于当前正在处理的实际上是一个 HTMLDocument 对象，因此 Document 类的成员函数 createParser调用另外一个成员函数 isHTMLDocument 得到的返回值会为 true，这时候 Document 类的成员函数就会调用 HTMLDocumentParser 类的静态成员函数 create 创建一个 HTMLDocumentParser 对象，如下所示：

```c++
class HTMLDocumentParser : public ScriptableDocumentParser, private HTMLScriptRunnerHost {
    WTF_MAKE_FAST_ALLOCATED_WILL_BE_REMOVED;
    WILL_BE_USING_GARBAGE_COLLECTED_MIXIN(HTMLDocumentParser);
public:
    static PassRefPtrWillBeRawPtr<HTMLDocumentParser> create(HTMLDocument& document, bool reportErrors)
    {
        return adoptRefWillBeNoop(new HTMLDocumentParser(document, reportErrors));
    }
 
    ......
};
```

​		从这里可以看到，HTMLDocumentParser 类的静态成员函数 create 创建的是一个HTMLDocumentParser 对象，这个 HTMLDocumentParser 对象会返回给调用者。

​		这一步执行完成之后，回到 DocumentLoader 类的成员函数 dataReceived 中，它调用成员函数ensureWriter 确定成员变量 m_writer 指向了一个 DocumentWriter 对象之后，接下来要调用这个DocumentWriter 对象的成员函数 addData 对下载回来的网页数据进行解析。

​		DocumentWriter 类的成员函数 addData 的实现如下所示：

```c++
void DocumentWriter::addData(const char* bytes, size_t length)
{
    ......
 
    m_parser->appendBytes(bytes, length);
}
```

​		从前面的分析可以知道，DocumentWriter 类的成员变量 m_parser 指向的是一个HTMLDocumentParser 对象，DocumentWriter 类的成员函数 addData 调用这个HTMLDocumentParser 对象的成员函数 appendBytes 对下载回来的网页数据进行解析。

​		HTMLDocumentParser 类的成员函数 appendBytes 的实现如下所示：

```c++
void HTMLDocumentParser::appendBytes(const char* data, size_t length)
{
    ......
 
    if (shouldUseThreading()) {
        ......
 
        OwnPtr<Vector<char> > buffer = adoptPtr(new Vector<char>(length));
        memcpy(buffer->data(), data, length);
        ......
 
        HTMLParserThread::shared()->postTask(bind(&BackgroundHTMLParser::appendRawBytesFromMainThread, m_backgroundParser, buffer.release()));
        return;
    }
 
    DecodedDataDocumentParser::appendBytes(data, length);
}
```

​		HTMLDocumentParser 类的成员函数 appendBytes 调用另外一个成员函数 shouldUseThreading 判断是否需要在一个专门的线程中对下载回来的网页数据进行解析。如果需要的话，那么就把下载回来的网页数据拷贝到一个新的缓冲区中去交给专门的线程进行解析。否则的话，就在当前线程中调用父类DecodedDataDocumentParser 类的成员函数 appendBytes 对下载回来的网页数据进行解析。为了简单起见，我们分析后一种情况，也就是分析 DecodedDataDocumentParser 类的成员函数 appendBytes 的实现。

​		DecodedDataDocumentParser 类的成员函数 appendBytes 的实现如下所示：

```c++
void DecodedDataDocumentParser::appendBytes(const char* data, size_t length)
{
    ......
 
    String decoded = m_decoder->decode(data, length);
    updateDocument(decoded);
}
```

​		DecodedDataDocumentParser 类的成员变量 m_decoder 指向一个 TextResourceDecoder 对象。这个 TextResourceDecoder 对象负责对下载回来的网页数据进行解码。解码后得到网页数据的字符串表示。这个字符串将会交给由另外一个成员函数 updateDocument 进行处理。

​		DecodedDataDocumentParser 类的成员函数 updateDocument 的实现如下所示：

```c++
void DecodedDataDocumentParser::updateDocument(String& decodedData)
{
    ......
 
    if (!decodedData.isEmpty())
        append(decodedData.releaseImpl());
}
```

​		DecodedDataDocumentParser 类的成员函数 updateDocument 又将参数 decodedData 描述的网页内容交给由子类 HTMLDocumentParser 实现的成员函数 append 处理。

​		HTMLDocumentParser 类的成员函数 append 的实现如下所示：

```c++
void HTMLDocumentParser::append(PassRefPtr<StringImpl> inputSource)
{
    ......
 
    String source(inputSource);
 
    ......
 
    m_input.appendToEnd(source);
 
    ......
 
    if (m_isPinnedToMainThread)
        pumpTokenizerIfPossible(ForceSynchronous);
    else
        pumpTokenizerIfPossible(AllowYield);
 
    ......
}
```

​		HTMLDocumentParser 类的成员函数 append 首先将网页内容附加在成员变量 m_input 描述的一个输入流中，接下来再调用成员函数 pumpTokenizerIfPossible 对该输入流中的网页内容进行解析。

​		在调用成员函数 pumpTokenizerIfPossible 的时候，根据成员变量 m_isPinnedToMainThread 的值的不同而传递不同的参数。当成员变量 m_isPinnedToMainThread 的值等于 true 的时候，传递的参数为ForceSynchronous，表示要以同步方式解析网页的内容。当成员变量 m_isPinnedToMainThread 的值等于 false 的时候，传递的参数为 AllowYield ，表示要以异步方式解析网页的内容。

​		在同步解析网页内容方式中，当前线程会一直运行到所有下载回来的网页内容都解析完为止，除非遇到有JavaScript 需要运行。在异步解析网页内容方式中，在遇到有 JavaScript 需要运行，或者解析的网页内容超过一定量时，如果当前线程花在解析网页内容的时间超过预设的阀值，那么当前线程就会自动放弃 CPU，通过一个定时器等待一小段时间后再继续解析剩下的网页内容。

​		接下来我们就继续分析 HTMLDocumentParser 类的成员函数 pumpTokenizerIfPossible 的实现，如下所示：

```c++
void HTMLDocumentParser::pumpTokenizerIfPossible(SynchronousMode mode)
{
    ......
 
    // Once a resume is scheduled, HTMLParserScheduler controls when we next pump.
    if (isScheduledForResume()) {
        ASSERT(mode == AllowYield);
        return;
    }
 
    pumpTokenizer(mode);
}
```

​		HTMLDocumentParser 类的成员函数 pumpTokenizerIfPossible 首先调用成员函数isScheduledForResume 判断当前正在处理的 HTMLDocumentParser 对象是否处于等待重启继续解析网页内容的状态中。如果是的话，等到定时器超时时，当前线程就会自动调用当前正在处理的HTMLDocumentParser 对象的成员函数 pumpTokenizer 对剩下未解析的网页内容进行解析。这种情况必须要确保参数 mode 的值为 AllowYield，也就是确保当前正在处理的 HTMLDocumentParser 对象使用异步方式解析网页内容。

​		如果当前正在处理的 HTMLDocumentParser 对象是以同步方式解析网页内容，那么HTMLDocumentParser 类的成员函数 pumpTokenizerIfPossible 接下来就会马上调用成员函数pumpTokenizer 对刚才下载回来的网页内容进行解析。

​		HTMLDocumentParser 类的成员函数 pumpTokenizer 的实现如下所示：

```c++
void HTMLDocumentParser::pumpTokenizer(SynchronousMode mode)
{
    ......
 
    PumpSession session(m_pumpSessionNestingLevel, contextForParsingSession());
    ......
 
    while (canTakeNextToken(mode, session) && !session.needsYield) {
        ......
 
        if (!m_tokenizer->nextToken(m_input.current(), token()))
            break;
 
        ......
 
        constructTreeFromHTMLToken(token());
        ......
    }
 
    ......
}
```

​		HTMLDocumentParser类的成员函数 pumpTokenizer 通过成员变量 m_tokenizer 描述的一个HTMLTokenizer 对象的成员函数 nextToken 对网页内容进行字符串解析。**网页内容被解析成一系列的Token。每一个 Token 描述的要么是一个标签，要么是一个标签的内容，也就是文本。有了这些 Token 之后，HTMLDocumentParser 类的成员函数 pumpTokenizer 就可以构造 DOM Tree 了**。这是通过调用另外一个成员函数 constructTreeFromHTMLToken 进行的。

​		注意，HTMLDocumentParser 类的成员函数 pumpTokenizer 通过一个 while 循环依次提取网页内容的 Token，并且每提取一个 Token，都会调用一次 HTMLDocumentParser 类的成员函数constructTreeFromHTMLToken。这个 while 循环在三种情况下会结束。

-  第一种情况是所有的Token均已提取并且处理完毕。
- 第二种情况是在解析的过程中遇到JavaScript脚本需要执行，这时候调用HTMLDocumentParser类的成员函数canTakeNextToken的返回值会等于false。
- 第三种情况出现在异步方式解析网页内容时，这时候HTMLDocumentParser类的成员函数canTakeNextToken会将本地变量session描述的一个PumpSession对象的成员变量needsYield的值设置为true，表示当前线程持续解析的网页内容已经达到一定量并且持续的时间也超过了一定值，需要自动放弃使用CPU。

​        接下来我们继续分析 HTMLDocumentParser 类的成员函数 constructTreeFromHTMLToken 的实现，如下所示：

```c++
void HTMLDocumentParser::constructTreeFromHTMLToken(HTMLToken& rawToken)
{
    AtomicHTMLToken token(rawToken);
 
    ......
 
    m_treeBuilder->constructTree(&token);
 
    ......
}
```

​		HTMLDocumentParser 类的成员函数 constructTreeFromHTMLToken 所做的事情就是根据参数rawToken 描述的一个 Token 来不断构造网页的 DOM Tree。这个构造过程是通过调用成员变量m_treeBuilder 描述的一个 HTMLTreeBuilder 对象的成员函数 constructTree 实现的。

​		HTMLTreeBuilder 类的成员函数 constructTree 的实现如下所示：

```c++
void HTMLTreeBuilder::constructTree(AtomicHTMLToken* token)
{
    if (shouldProcessTokenInForeignContent(token))
        processTokenInForeignContent(token);
    else
        processToken(token);
 
    ......
 
    m_tree.executeQueuedTasks();
    // We might be detached now.
}
```

​		HTMLTreeBuilder 类的成员函数 constructTree 首先调用成员函数shouldProcessTokenInForeignContent 判断参数 token 描述的 Token 是否为 Foreign Content，即不是HTML 标签相关的内容，而是 MathML 和 SVG 这种外部标签相关的内容。如果是的话，就调用成员函数processTokenInForeignContent 对它进行处理。

​		如果参数 token 描述的是一个 HTML 标签相关的内容，那么 HTMLTreeBuilder 类的成员函数constructTree 就会调用成员函数 processToken 对它进行处理。接下来我们只关注 HTML 标签相关内容的处理过程。

​		处理完成参数 token 描述的 Token 之后，HTMLTreeBuilder 类的成员函数 constructTree 会调用成员变量 m_tree 描述的一个 HTMLConstructionSite 对象的成员函数 executeQueuedTasks 执行保存其内部的一个事件队列中的任务。这些任务是处理参数 token 描述的标签的过程中添加到事件队列中去的，主要是为了处理那些在网页中没有正确嵌套的格式化标签的。HTML 标准规定了处理这些没有正确嵌套的格式化标签的算法，具体可以参考标准中的12.2.3.3小节：[HTML Standard (whatwg.org)](https://html.spec.whatwg.org/multipage/parsing.html#list-of-active-formatting-elements)。WebKit 在实现这个算法的时候，就用到了上述的事件队列。

​		接下来我们继续分析 HTMLTreeBuilder 类的成员函数 processToken 的实现，如下所示：

```c++
void HTMLTreeBuilder::processToken(AtomicHTMLToken* token)
{
    if (token->type() == HTMLToken::Character) {
        processCharacter(token);
        return;
    }
 
    // Any non-character token needs to cause us to flush any pending text immediately.
    // NOTE: flush() can cause any queued tasks to execute, possibly re-entering the parser.
    m_tree.flush();
    m_shouldSkipLeadingNewline = false;
 
    switch (token->type()) {
    case HTMLToken::Uninitialized:
    case HTMLToken::Character:
        ASSERT_NOT_REACHED();
        break;
    case HTMLToken::DOCTYPE:
        processDoctypeToken(token);
        break;
    case HTMLToken::StartTag:
        processStartTag(token);
        break;
    case HTMLToken::EndTag:
        processEndTag(token);
        break;
    case HTMLToken::Comment:
        processComment(token);
        break;
    case HTMLToken::EndOfFile:
        processEndOfFile(token);
        break;
    }
}
```

​		如果参数 token 描述的 Token 的类型是 HTMLToken::Character，就表示该 Token 代表的是一个普通文本。这些普通文本不会马上进行处理，而是先保存在内部的一个 Pending Text 缓冲区中，这是通过调用HTMLTreeBuilder 类的成员函数 processCharacter 实现的。等到遇到下一个 Token 的类型不是HTMLToken::Character 时，才会对它们进行处理，这是通过调用成员变量 m_tree 描述的一个HTMLConstructionSite 对象的成员函数 flush 实现的。

​		对于非 HTMLToken::Character 类型的 Token，HTMLTreeBuilder 类的成员函数 processToken 根据不同的类型调用不同的成员函数进行处理。在处理的过程中，就会使用图3所示的栈构造 DOM Tree，并且会遵循 HTML 规范，具体可以参考这里：[HTML Standard (whatwg.org)](https://html.spec.whatwg.org/multipage/syntax.html)。例如，**对于HTMLToken::StartTag类型的 Token，就会调用成员函数 processStartTag 执行一个压栈操作，而对于 HTMLToken::EndTag 类型的 Token，就会调用成员函数 processEndTag 执行一个出栈操作。**

​		接下来我们主要分析 HTMLTreeBuilder 类的成员函数 processStartTag 的实现，主要是为了解 WebKit在内部是如何描述一个 HTML 标签的。

​		HTMLTreeBuilder 类的成员函数 processStartTag 的实现如下所示：

```c++
void HTMLTreeBuilder::processStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    switch (insertionMode()) {
    ......
    case InBodyMode:
        ASSERT(insertionMode() == InBodyMode);
        processStartTagForInBody(token);
        break;
    ......
    }
}
```

​		HTMLTreeBuilder 类在构造网页的 DOM Tree 时，根据当前所处理的网页内容而将内部状态设置为不同的 Insertion Mode。这些 Insertion Mode 是由 HTML 规范定义的，具体可以参考12.2.3.1小节：[HTML Standard (whatwg.org)](https://html.spec.whatwg.org/multipage/parsing.html#insertion-mode)  例如，当处理到网页的 body 标签里面的内容时，Insertion Mode 就设置为InBodyMode，这时候 HTMLTreeBuilder 类的成员函数 processStartTag 就调用另外一个成员函数processStartTagForInBody 按照 InBodyMode 的 Insertion Mode 来处理参数 token 描述的 Token。

​		接下来我们继续分析 HTMLTreeBuilder 类的成员函数 processStartTagForInBody 的实现，如下所示：

```c++
void HTMLTreeBuilder::processStartTagForInBody(AtomicHTMLToken* token)
{
    ......
 
    if (token->name() == addressTag
        || token->name() == articleTag
        || token->name() == asideTag
        || token->name() == blockquoteTag
        || token->name() == centerTag
        || token->name() == detailsTag
        || token->name() == dirTag
        || token->name() == divTag
        || token->name() == dlTag
        || token->name() == fieldsetTag
        || token->name() == figcaptionTag
        || token->name() == figureTag
        || token->name() == footerTag
        || token->name() == headerTag
        || token->name() == hgroupTag
        || token->name() == mainTag
        || token->name() == menuTag
        || token->name() == navTag
        || token->name() == olTag
        || token->name() == pTag
        || token->name() == sectionTag
        || token->name() == summaryTag
        || token->name() == ulTag) {
        ......
        m_tree.insertHTMLElement(token);
        return;
    }
 
    ......
}
```

​		我们假设参数 token 描述的 Token 代表的是一个 \<div\> 标签，那么 HTMLTreeBuilder 类的成员函数processStartTagForInBody 就会调用成员变量 m_tree 描述的一个 HTMLConstructionSite 对象的成员函数 insertHTMLElement 为其创建一个 HTMLElement 对象，并且将这个 HTMLElement 对象压入栈中去构造 DOM Tree。

​		HTMLConstructionSite 类的成员函数 insertHTMLElement 的实现如下所示：

```c++
void HTMLConstructionSite::insertHTMLElement(AtomicHTMLToken* token)
{
    RefPtrWillBeRawPtr<Element> element = createHTMLElement(token);
    attachLater(currentNode(), element);
    m_openElements.push(HTMLStackItem::create(element.release(), token));
}
```

​		HTMLConstructionSite 类的成员函数 insertHTMLElement 首先调用成员函数 createHTMLElement创建一个 HTMLElement 对象描述参数 token 代表的 HTML 标签，接着调用成员函数 attachLater 稍后将该HTMLElement 对象设置为当前栈顶 HTMLElement 对象的子 HTMLElement 对象，最后又将该HTMLElement 对象压入成员变量 m_openElements 描述的栈中去。

​		接下来我们主要分析 HTMLConstructionSite 类的成员函数 createHTMLElement 的实现，以便了解WebKit 是如何描述一个 HTML 标签的。

​		HTMLConstructionSite 类的成员函数 createHTMLElement 的实现如下所示：

```c++
PassRefPtrWillBeRawPtr<Element> HTMLConstructionSite::createHTMLElement(AtomicHTMLToken* token)
{
    Document& document = ownerDocumentForCurrentNode();
    // Only associate the element with the current form if we're creating the new element
    // in a document with a browsing context (rather than in <template> contents).
    HTMLFormElement* form = document.frame() ? m_form.get() : 0;
    // FIXME: This can't use HTMLConstructionSite::createElement because we
    // have to pass the current form element.  We should rework form association
    // to occur after construction to allow better code sharing here.
    RefPtrWillBeRawPtr<Element> element = HTMLElementFactory::createHTMLElement(token->name(), document, form, true);
    setAttributes(element.get(), token, m_parserContentPolicy);
    ASSERT(element->isHTMLElement());
    return element.release();
}
```

​		从这里可以看到，HTMLConstructionSite 类的成员函数 createHTMLElement 是调用HTMLElementFactory 类的静态成员函数 createHTMLElement 为参数 token 描述的 HTML 标签创建一个HTMLElement 对象的，并且接下来还会调用另外一个成员函数 setAttributes 根据 Token 的内容设置该HTMLElement 对象的各个属性值。

​		HTMLElementFactory 类的静态成员函数 createHTMLElement 的实现如下所示：

```c++
typedef HashMap<AtomicString, ConstructorFunction> FunctionMap;
 
static FunctionMap* g_constructors = 0;
 
......
 
static void createHTMLFunctionMap()
{
    ASSERT(!g_constructors);
    g_constructors = new FunctionMap;
    // Empty array initializer lists are illegal [dcl.init.aggr] and will not
    // compile in MSVC. If tags list is empty, add check to skip this.
    static const CreateHTMLFunctionMapData data[] = {
        { abbrTag, abbrConstructor },
        ......
        { divTag, divConstructor },
        ......
        { wbrTag, wbrConstructor },
    };
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(data); i++)
        g_constructors->set(data[i].tag.localName(), data[i].func);
}
 
PassRefPtrWillBeRawPtr<HTMLElement> HTMLElementFactory::createHTMLElement(
    const AtomicString& localName,
    Document& document,
    HTMLFormElement* formElement,
    bool createdByParser)
{
    ......
 
    if (ConstructorFunction function = g_constructors->get(localName))
        return function(document, formElement, createdByParser);
 
    ......

```

​		HTMLElementFactory 类的静态成员函数 createHTMLElement 根据 HTML 标签的名称在全局变量g_constructors 描述的一个 Function Map 中找到指定的函数为该 HTML 标签创建一个 HTMLElement 对象。例如，用来描述 HTML 标签 \<div\> 的 HTMLElement 对象是通过调用函数 divConstructor 进行创建的。

​		函数 divConstructor 的实现如下所示：

```c++
static PassRefPtrWillBeRawPtr<HTMLElement> divConstructor(
    Document& document,
    HTMLFormElement* formElement,
    bool createdByParser)
{
    return HTMLDivElement::create(document);
}
```

​		函数 divConstructor 调用 HTMLDivElement 类的静态成员函数 create 创建了一个 HTMLDivElement对象，并且返回给调用者。这样以后我们需要了解 \<div\> 标签的更多细节时，就可以参考 HTMLDivElement类的实现。





​		这样，我们就分析完成网页的 DOM Tree 的创建过程了。我们没有很详细地描述这个创建过程，因为这涉及到很多实现细节，以及极其繁琐的 HTML 规范。我们提供了一个 DOM Tree 创建的框架。有了这个框架之后，以后当我们需要了解某一个细节时，就可以方便地找到相关源码进行分析。

​		网页内容下载完成之后，DOM Tree 的构造过程就结束。接下来 WebKit 就会根据 DOM Tree 创建Render Object Tree。在接下来一篇文章中，我们就详细分析 Render Object Tree 的创建过程



















