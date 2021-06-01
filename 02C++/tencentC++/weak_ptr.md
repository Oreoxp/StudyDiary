shared_ptr， 多个对象指向同一个原始对象，每一个shared_ptr 对象的构造和析构都会对这个原始对象有影响， 只要还有地方持有shared_ptr 对象，那原始对象就不会被释放。所以这里就会引入一个比较经典的问题  `循环引用`，  还有就是另一个问题 `生命周期`。 再讨论这两个问题之前，还是先看看weak_ptr的源码实现，已经分析了shared_ptr，weak 的实现和shared_ptr由很多相似


	他们有共同的基类  _Ptr_base, weak_ptr  本身也没有成员变量

```c++

// CLASS TEMPLATE weak_ptr
template <class _Ty>
class weak_ptr : public _Ptr_base<_Ty> { // class for pointer to reference counted resource
public:
}
```

	基类中有一个引用计数的成员变量,这个对象里面有两个关键成员
	_Uses 和 _Weaks

```c++
// CLASS _Ref_count_base
class __declspec(novtable) _Ref_count_base {
private:
	_Atomic_counter_t _Uses  = 1;
    _Atomic_counter_t _Weaks = 1;
}
```

`_Uses: 表示当前有多少个shared_ptr 指向 同一个原始对象`
`_Weaks: 表示有(_Weaks -1)个weak_ptr 指向同一个对象。  这里需要减1 是因为初始化为1，每次有一个weka_ptr 对象生成就累加了`


接下来可以看看weak_ptr的使用
```c++
  //声明一个weak 对象
  std::weak_ptr<int> weak_test;  		//1
  //构造一个shared 对象           
  std::shared_ptr<int> test = std::make_shared<int>();//2
  weak_test = test;  					 //3
  //从weak中取出shared 对象
  auto shared_test = weak_test.lock();   //4
  test.reset();  						//5
  shared_test.reset();					//6
  shared_test = weak_test.lock();  		//7
```
`先来看看第三句代码发生了什么` weak_test = test;

```c++
//为了简化，把一些不相干的都去掉了
    weak_ptr(const shared_ptr<int>& _Other) { 
        this->_Weakly_construct_from(_Other);
    }
	//指针赋值
   void _Weakly_construct_from(const _Ptr_base<int>& _Other) { 
        if (_Other._Rep) {
            _Ptr = _Other._Ptr;
            _Rep = _Other._Rep;
            _Rep->_Incwref();
        } else {
            _STL_INTERNAL_CHECK(!_Ptr && !_Rep);
        }
    }
//增加weak 引用计数
  void _Incwref() noexcept { // increment weak reference count
        _MT_INCR(_Weaks);
    }
```

这里需要注意，指针赋值操作也不是线程安全的，也就是第三句代码在实际项目中是需要考虑多线程问题的。

`第四句代码  auto shared_test = weak_test.lock();   源码`

```c++
    shared_ptr<int> lock() const noexcept { // convert to shared_ptr
        shared_ptr<int> _Ret;
        (void) _Ret._Construct_from_weak(*this);
        return _Ret;
    }

    //如果原始对象没有被析构，那就增加引用计数，并且指针赋值，否则就是个empty对象
    bool _Construct_from_weak(const weak_ptr<int>& _Other) {
        if (_Other._Rep && _Other._Rep->_Incref_nz()) {
            _Ptr = _Other._Ptr;
            _Rep = _Other._Rep;
            return true;
        }
        return false;
    }
```
从源码也可以看出，lock 也不是多线程安全的，lock实现里面指针也是直接赋值的。

## 小结一下：
1） weak_ptr 派生自 _Ptr_base，shared_ptr 也是派生自这个类
2） _Ptr_base 有一个指针的成员 _Ref_count_base 用来控制引用计数
3） _Ref_count_base  有两个成员_ Atomic_counter_t _Uses  = 1;
    _Atomic_counter_t _Weaks = 1; 分别用来记录 shared_ptr 以及weak_Ptr的引用计数。
  4) _Weaks  引用计数会比 weak_ptr 多1. 因为默认是1，每来一个都会累加1.
  5） weak_ptr本身也不是多线程安全的。和shared_ptr 类似

## 接下来看看weak_ptr的使用

```c++

class Apple {
 public:
  Apple() {

  }

  void Cancel() { 
    delegate_ = nullptr;
  }

  virtual ~Apple() { 
    printf("dec");
  }

  void Eat(std::function<void()> delegate) { 
    delegate_ = std::move(delegate);
  }
  
private:
  std::function<void()> delegate_;
};
//lambda 捕获 对象a, 导致循环引用。lambda 后面会详细介绍
std::shared_ptr<Apple> a = std::make_shared<Apple>();
  a->Eat([a]() { 
  });
```
`上面的调用Eat 函数，传入一个lambda 表达式，lambda 捕获了 自己。
本质上lambda 会构造一个std::function 对象， 并且把 捕获的参数当成这个对象中的一个成员，伪代码如下`


```c++

class Lambda {
  Lambda(const std::shared_ptr<Apple>& a) 
  		: object_1(a) {

  }

public:
  const std::shared_ptr<Apple> object_1;
};

std::shared_ptr<Apple> a = std::make_shared<Apple>();
//a 对象持有了Lambda 对象，Lambda 对象持有了 a 对象，所以出现了循环引用
a->Eat(Lambda(a));

```
`使用weak 来解决这种循环引用，weak 对象不会增加_Uses   引用计数，所以不会影响原始对象的生命周期 `
```c++
  std::shared_ptr<Apple> a = std::make_shared<Apple>();
  std::weak_ptr<Apple> weak_a = a;
  //捕获weak 解决循环应用问题
  a->Eat([weak_a] (){
    if (auto lock = weak_a.lock()) {
    }
  });
  ```


还可以直接将weak_ptr作为回调参数传给函数，再回调之前调用lock判断对象是否还存在，然后决定是否需要回调

````c++
class CallbackTest {

};

class Apple {
 public:
  Apple() {

  }

  virtual ~Apple() { 
    printf("dec");
  }

  void SetDelegate(std::weak_ptr < CallbackTest> delegate) { 
    delegate_ = std::move(delegate);
  }

  void OnXXX() {
    if (auto delegate = delegate_.lock()) {
      //do something
    }
  }

private:
  std::weak_ptr<CallbackTest> delegate_;
};

std::shared_ptr<Apple> a = std::make_shared<Apple>();
std::shared_ptr<CallbackTest> delegate = std::make_shared<CallbackTest>();
//传入一个weka_ptr, 如果外部的delegate 被释放了的话，调用lock 就会返回空
a->SetDelegate(delegate);
````