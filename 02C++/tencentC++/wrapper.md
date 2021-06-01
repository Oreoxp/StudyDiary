# Wrapper 揭秘

Base 库中很多类都是通过一个头文件提供给外部使用，头文件只有类的声明，真正的实现是封装在cc 文件里面，一般会有一个Impl 类来对应， 可能还会有Wrapper 类进行一个简单的封装。

`HttpClient 接口声明`
````c++
class BASE_EXPORT HttpClient {
public:
  virtual ~HttpClient() = default;

  virtual bool Execute(base::unique_ptr<HttpRequest> request) = 0;

  virtual void Close() = 0;
};
````

`HttpClientImpl 实现`

````c++
class HttpClientImpl : XXX {
}
````
`HttpClientWrapper 实现`

````c++
class HttpClientWrapper :
  public HttpClient {
public:
  explicit HttpClientWrapper(
    base::shared_ptr<BASE_THREADING::Looper> looper,
    HttpClientDelegate* delegate) {
    impl_ = base::make_shared_s<HttpClientImpl>(looper, this, delegate);
  }

  virtual ~HttpClientWrapper() {
    Close();
  }

public: // HttpClient
  bool Execute(base::unique_ptr<HttpRequest> request) override {
    if (auto impl = impl_.lock()) {
      return impl->Execute(std::move(request));
    }
    return false;
  }


  void Close() override {
    if (auto impl = impl_.move()) {
      impl->Close();
    }
  }
private:
  base::shared_ptr_s<HttpClientImpl> impl_;
};

````

 1. 接口声明。
 2. 接口实现。
 3. 对实现的一个封装。 

 ***1, 2 是标准做法，那为什么我们的项目中会有一个Wrapper？*

 ## 先来个入门的，C++ 的析构函数。
  c++ 析构函数是用来释放资源的， 比如上面那个HttpclientImpl类，我们可能会把资源释放的工作放到析构函数。但是 却并没有。

````c++
HttpClientImpl::~HttpClientImpl() {
  //引发异常
  Close();
}
````

## 析构函数为什么没有做任何清理工作？
我们的异步请求都是基于libuv， libuv 的异步请求都是依赖looper， 在我们的项目中，比如IO 请求，无论是连接，收发数据，断开等等操作都是需要抛到相应的looper 去执行。所以这里有个意外。`不能在析构里面关闭连接`

````c++
void HttpClientImpl::Close() {
    //省略掉了一些干扰代码
    auto self = shared_from_this();
    looper_->PostRunnable([self]() {
      self->DoClose();
    });
}

````
看看上面的代码，如果在析构函数里面调用 `auto self = shared_from_this();`就会出现异常。

所以我们不能依赖析构函数，而是`必须`要调用者手动调用 Close 函数,否则就会出现crash等异常。 ？


## 调用者忘记调用Close怎么办？
为了防止调用者忘记调用Close 函数导致的异常，所以Wrapper 出现了

**Wrapper 有一个Impl的实例，Wrapper的析构调用了Close函数，完美解决**

````c++
//删掉了一些干扰代码
class HttpClientWrapper :
  public HttpClient {
public:
  virtual ~HttpClientWrapper() {
    Close();
  }

  void Close() override {
    if (auto impl = impl_.move()) {
      impl->Close();
    }
  }
  
  base::shared_ptr_s<HttpClientImpl> impl_;
};
````
