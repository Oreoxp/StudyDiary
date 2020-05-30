# 为什么要使用 std::enable_shared_from_this
我们要实例化一个智能指针有多种方法，但是最常见的有三种

```c++
#include <memory>
class Apple {
 public:
  Apple() {
  }

  virtual ~Apple() {
  }

//shared_ptr初始化
  std::shared_ptr<Apple> a = std::make_shared<Apple>();
  std::shared_ptr<Apple> b = std::shared_ptr<Apple>(new Apple);
  std::shared_ptr<Apple> c = b;
```
	上面是比较常用的方法,但是如果我想在一个类的成员函数中获取这个类对象的一个 shared_ptr, 或者weak_ptr，那要怎么做呢？比如下面
	
```c++
#include <memory>
class Apple {
public:
  void Test() {
    //获取对象本身的shared_ptr，或者weak_ptr
    std::shared_ptr<Apple>self = xxx();
  }
}
```
<br/>

这里又引申出另一个问题，为什么需要在成员函数里面去获取对象自己的智能指针呢？这里使用场景有很多

 - 声明周期保护。 比如我们设计了一个回调。当完成之后，调用回调，然后有人在回调里面把对象给删掉了。
  ```c++
#include <memory>
class Apple : public std::enable_shared_from_this<Apple> {
	 public:
	  void Test() {
	   // 下面一行代码保护生命周期
	    auto self = shared_from_this();
	    {
	      std::lock_guard<std::mutex> lock(mutex_);
	      //回调的时候，如果对象被上层给释放了，就一定会造成crash， 应该回调完之后,成员变量 mutex_ 还在，对象却被析构了。
	      delegete_->OnComplete();
	    }
	  }
}
```
- Lambda 表达式捕获this 指针，如果this 指向的对象被析构了，那再次使用this 肯定就崩溃了。
```c++
#include <memory>
class Apple {
 public:
  void Test() {
    //lambda 捕获this 指针
    http_client_->Request([this] {
      // Do Something
    });
  }

 private:
  std::unique_ptr<HttpClientt> http_client_;
}

//改进:
class Apple : public std::enable_shared_from_this<Apple> {
public:
  Apple() {

  }
 void Test() {
   auto weak_self = weak_from_this();
   //这里捕获weak，所以如果Apple 对象被析构了。 weak_self.lock将返回一个null
   http_client_->Request([weak_self] {
     if (auto self = weak_self.lock()) {
       //Do Something
     }
   });
 }

 private:
  std::unique_ptr<HttpClientt> http_client_;
};
```

<br/>
再来看看 源码实现,为了描述问题，去掉了一些不要的模板参数

````c++
class enable_shared_from_this { 
public:
    _NODISCARD shared_ptr<Apple> shared_from_this() {
        return shared_ptr<Apple>(_Wptr);
    }

    _NODISCARD shared_ptr<const Apple> shared_from_this() const {
        return shared_ptr<const Apple>(_Wptr);
    }

    _NODISCARD weak_ptr<Apple> weak_from_this() noexcept {
        return _Wptr;
    }

    _NODISCARD weak_ptr<const Apple> weak_from_this() const noexcept {
        return _Wptr;
    }
private:
	mutable weak_ptr<Apple> _Wptr;
}
````
`源码中，只有一个私有成员，是一个weak_ptr, 那这个weak_ptr是在什么时候初始化的呢？还记得之前shared_ptr的初始化里面`
````c++
 template <class _Ux>
    void _Set_ptr_rep_and_enable_shared(_Ux* const _Px, _Ref_count_base* const _Rx) noexcept { // take ownership of _Px
        this->_Ptr = _Px;
        this->_Rep = _Rx;
        //就是这里赋值的 weak_ptr
        if (_Px && _Px->_Wptr.expired()) {
             _Px->_Wptr = shared_ptr<remove_cv_t<_Ux>>(*this, const_cast<remove_cv_t<_Ux>*>(_Px));
         }
   }
````
## 那为啥要是一个weka_ptr,不是一个shared_ptr呢？
	如果是一个shared,那对象本身就持有了自己，这样就永远无法释放了。 使用weak并不会增加强引用计数，所以对象的生命周期不会受影响。


上面分析了源码以及好处，那我们肯定是要用的，使用的时候还是需要注意一些细节

 1. 不要在构造函数里面用。
````c++
class Apple : public std::enable_shared_from_this<Apple> {
public:
  Apple() { 
    //构造函数中使用 shared_from_this  会发生异常
    auto self = shared_from_this();
  }
};
````
`对象的构造函数中， 基类的_Wptr 成员变量还没有初始化，从一个empty 想要转换成 shared_ptr 肯定是有异常的。`
<br/>
 2. 不要在析构函数中使用
 ````c++
 class Apple : public std::enable_shared_from_this<Apple> {
public:
  ~Apple() { 
   //析构函数中使用，发生异常
    auto self = shared_from_this();
  }
};
 ````
`这个比较容易理解，进入到对象的析构函数，说明强引用计数已经变为0了，这个时候转换成shared_ptr肯定是不行的，`

