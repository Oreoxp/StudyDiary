# std::shared_ptr
C++11 开始已经支持了 shared_ptr, 字面意思就是共享指针。 多个对象可以持有同一块内存。和shared_ptr 相关的C++11类比较多，比如weak_ptr, 
enable_shared_from_this 都和他有关系。
先看一下如何初始化 `初始化代码`。

```c++
// 自定义class
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

`shared_ptr 用起来非常简单,非侵入式的实现，几乎没有成本。`

<br/>

## 源码实现
shared_ptr内部持有一个引用计数，严格来说是他的基类持有一个引用计数，这个引用计数在增加和减少的时候使用的原子操作，保证引用计数是线程安全，这里仅仅是保证引用计数的线程安全，对于对象本身并不是线程安全的。
<br/>

```c++
//为了演示只保留了基本的代码
//shared_ptr weak_ptr 的基类
template <class _Ty>
class _Ptr_base { // base class for shared_ptr and weak_ptr
private:
	element_type* _Ptr{nullptr}; //原始指针
    _Ref_count_base* _Rep{nullptr}; //引用计数指针
}
```

<br/>

_Ref_count_base* _Rep{nullptr}  引用计数为什么要用指针，而不是一个对象？
因为所有的对象(指向同一块内存)都需要感知到引用计数的变化，每一个对象都有_Rep 指针，这样所有的对象都可以访问引用计数，进行增加和减少操作。


```c++
// CLASS _Ref_count_base
class __declspec(novtable) _Ref_count_base {
private:
	_Atomic_counter_t _Uses  = 1;
    _Atomic_counter_t _Weaks = 1;
}
```
`操作 _Uses和_Weaks 两个成员都是通过原子操作，保证线程安全`



```c++
#define _MT_INCR(x) _INTRIN_RELAXED(_InterlockedIncrement)(reinterpret_cast<volatile long*>(&x))
#define _MT_DECR(x) _INTRIN_ACQ_REL(_InterlockedDecrement)(reinterpret_cast<volatile long*>(&x))
```
<br/>

```c++
template <class _Ty>
class shared_ptr : public _Ptr_base<_Ty> { // class for reference counted resource management

}
```
`shared_ptr  自身并没有成员变量，实现了很多的成员函数给外部调用。`


<br/><br/>
## shared_ptr初始化的故事

```c++
//下面的两种初始化到底有什么不一样呢？
  std::shared_ptr<Apple> a = std::make_shared<Apple>();
  std::shared_ptr<Apple> b = std::shared_ptr<Apple>(new Apple);

//先看看 std::make_shared 源码，一堆的模板，为了好看我直接去掉了模板参数，用我上面的Apple类替代，参数我也直接去掉了
shared_ptr<Apple> make_shared() { // make a shared_ptr
    const auto _Rx = new _Ref_count_obj2<Apple>();
    shared_ptr<Apple> _Ret;
    //把裸指针和引用计数设置进去，然后直接返回
    _Ret._Set_ptr_rep_and_enable_shared(_Rx->_Storage._Value, _Rx);
    return _Ret;
}


class _Ref_count_obj2 : public _Ref_count_base { 
public:
    explicit _Ref_count_obj2() : _Ref_count_base() {
        _Construct_in_place(_Storage._Value, _STD forward<_Types>(_Args)...);
    }
	//union 类型
    union {
        _Wrap<Apple> _Storage;
    };
}
//_Storage 成员变量用的非常巧妙，union 类型的成员是不会构造的，一般大家不会放对象进去，都是放的指针，原因就是union的对象不会调用构造函数，没办法使用。源码里面放一个union 也是一样的道理，源码就是不想让它自动构造，而是手动构造。

void _Construct_in_place(Apple& _Obj) {
    ::new (const_cast<void*>(static_cast<const volatile void*>(_STD addressof(_Obj))));
}
//上面的代码就是就是在某一块内存上面，调用某一个类的构造函数， 估计很多人都是第一次见这种语法。
```

`_Storage 成员变量用的非常巧妙，union 类型的成员是不会构造的，一般大家不会放对象进去，都是放的指针，原因就是union的对象不会调用构造函数，没办法使用。源码里面放一个union 也是一样的道理，源码就是不想让它自动构造，而是手动构造`

<br/>


`还有一个细节，既然 union 是不会调用构造，那同样也不会调用析构。如果不调用析构的话肯定会造成资源泄露，源码里面是手动调用了析构函数，估计也是很多人没有真正在项目中用过`


```c++
template <class _Ty>
void _Destroy_in_place(_Ty& _Obj) noexcept {
    _Obj.~_Ty(); //手动调用析构释放资源。
}
```


<br/>

绕了一大圈那源码为什么要这么做呢？
`因为源码里面只用了一次new。new 出来的对象包含了Apple对象以及引用计数，并且也只调用了一次 Apple对象的构造函数。简易内存模型如下:`
![在这里插入图片描述](https://img-blog.csdnimg.cn/20200514210813813.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dhbmxpYWl4aWU=,size_16,color_FFFFFF,t_70#pic_center)


```c++
下面的初始化就会有两个new 调用。并且这两个对象的内存并不是黏在一起的。
std::shared_ptr<Apple> b = std::shared_ptr<Apple>(new Apple);

explicit shared_ptr(Apple* _Px) { // construct shared_ptr object that owns _Px
	_Temporary_owner<Apple> _Owner(_Px);
	_Set_ptr_rep_and_enable_shared(_Owner._Ptr, new _Ref_count<_Ux>(_Owner._Ptr));
	_Owner._Ptr = nullptr;
}

```
`从源码可以看出了，除了自己new 了一个Apple对象之外，shared_ptr 也new 了一个_Ref_count 对象。 这里就有两次new 操作。 对应的就会有两次delete 操作。 `
<br/>
<br/>

## shared_ptr 多线程是否安全

```c++
	std::shared_ptr<Apple> b = std::shared_ptr<Apple>(new Apple);
	std::shared_ptr<Apple> c = b;
 	//b 赋值给a 并不是线程安全的，如果有另一个线程 正在释放的话，这个赋值可能是有问题的。

	shared_ptr(const shared_ptr& _Other) noexcept { // construct shared_ptr object that owns same resource as _Other
        this->_Copy_construct_from(_Other);
    }


    void _Copy_construct_from(const shared_ptr<Apple>& _Other) noexcept {
        // implement shared_ptr's (converting) copy ctor
        if (_Other._Rep) {
            _Other._Rep->_Incref();
        }

        _Ptr = _Other._Ptr;
        _Rep = _Other._Rep;
    }
```
从 _Copy_construct_from 实现可以看出。 引用计数调用 _Incref，这个是原子操作，多线程安全，但是下面的指针赋值却是没有锁保护的。如果另一个线程走到了delete 对象的时候，下面的指针也赋值了。就会导致crash发生。 为什么源码不设计成多线程安全的呢？ 个人觉得没必要，加一把锁增加性能消耗，如果仅仅是单线程的话，太浪费，真的有多线程，让使用者自己加锁就可以。

<br/>

# 总结
**1） 可以节省一次new 和一次 delete 操作** 
**2） std::make_shared 方式使得 Apple对象和引用计数对象内存是在一起的，可以减少内存碎片** 
**3） shared_ptr 本身不是线程安全的** 