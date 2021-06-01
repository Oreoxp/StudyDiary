C++11开始引入了std::move，然后又引入了一个右值得概念， 之前又有一个左值得概念。左值和右值网上特别多，引入std::move主要是为了优化对象的生命周期，以及优化函数参数传递方式。
# C++参数传递方式
`值传递`
````c++
//值传递
void SetName(std::string name) {

}
 std::string name = "123";
 SetName(name);
````
上面的函数参数是值传递，一般对于标准数据类型，会采用值传递，对于复杂类型，结构体，类对象等并不会用值传递，因为会多一份对象的拷贝

`引用传递`
````c++
//值传递
void SetName(const std::string& name) {

}
 std::string name = "123";
 SetName(name);
````
引用传递可以看成是指针的传递，并不会有临时对象的构造和析构，所以性能上会好一些。

**但是无论是哪种传递方式，调用者外部的变量 name 有时候感觉会有一次 "多余的构造和析构"**
````c++
class Apples {
public:
  void Add(const std::string& name) {
   //push_back 会拷贝构造一份，然后放入vector中
    names_.push_back(name);
  }

private:
  std::vector<std::string> names_;
};

  Apples apple;
  std::string one = "one";
  //使用 one 变量做一些操作 
  .......
  //引用传递
  apple.Add(one);
````
调用者定义的变量 'one' 通过Add 传递进去之后， one变量已经没有了用处，自然就会被析构掉。但是push_back 又拷贝了一份。是不是感觉这里多了一份呢？ 是不是直接把one 变量放入到 vector 中就不会多构造一份呢？ 确实是可以，我们可以用指针的方式。
````c++
class Apples {
public:
  void Add(std::string* name) {
    names_.push_back(name);
  }

private:
  std::vector<std::string*> names_;
};
 Apples apple;
  std::string *one = new std::string("one");
  ......
  apple.Add(one);
````
`上面通过指针的方式，将one 指针直接放入到了vecotr中，这样就不会有多余对象的构造，但是面临内存泄漏风险，使用不方便`
<br/>
# std::move　解决什么问题
从语法上支持`动态转移对象`，而不用做一些骚操作。

 ````c++
class Apples {
public:
  void Add(const std::string& name) {
   //push_back 会拷贝构造一份，然后放入vector中
    names_.push_back(name);
  }

private:
  std::vector<std::string> names_;
};

  Apples apple;
  std::string one = "one";
  //使用 one 变量做一些操作 
  .......
  apple.Add(std::move(one));
````
那是不是直接在 调用的地方使用 std::move 就可以了，当然不是了。因为从源码可以看出 std::move没有做任何实质性的操作，仅仅是个强制类型转换而已.`std::move源码：`
````c++
template <class _Ty>
_NODISCARD constexpr remove_reference_t<_Ty>&& move(_Ty&& _Arg) noexcept { // forward _Arg as movable
    return static_cast<remove_reference_t<_Ty>&&>(_Arg);
}
````
所以还需要自己来实现一些东西才能真正的`动态转移对象`。标准模板库的 std::string, std::vector,std::map ,std::shared_ptr等等都自己实现了move 语义。改造一下
````c++
class Apples {
public:
  void Add(const std::string& name) {
    names_.push_back(name);
  }
//新增一个右值参数的函数。
  void Add(std::string&& name) {
    names_.push_back(std::move(name));
  }

private:
  std::vector<std::string> names_;
};

 Apples apple;
  std::string one = "one";
  //std::move之后会强制转换成右值对象，这样就可以匹配到Apples 类中的带有右值得函数。
  apple.Add(std::move(one));
````
**所以要支持std::move 我们的类也需要提供一些带有右值得函数**

 1. 转移构造
 	````c++
 	  Apples(Apples&& other) {
 	  //do something
  	 }
 	````
 2. 转移赋值
	````c++
	  Apples& operator=(Apples&& other) {
		//do somthing
	  }
	````

Apples 对象实现了转移构造和转移赋值，所以就可以使用std::move　`动态转移`， 一定要记住 **std::move仅仅是强制类型转换，真正的转移是在转移构造以及转移赋值中完成的**

`那是不是所有的自定义结构体或者类都需要来实现那两个转移函数呢？`　
<br/>
当然并不是，我自己总结了一下，如果`你的类或者结构体中所有的成员都已经实现了std::move 语义，并且没有自定义析构函数(还没有搞清楚为什么有自定义的析构就不行)， 那可以不用显式提供，编译器会自动有一个。否则只要成员中有一个没有实现，都需要自己去实现`。

不需要自己实现的代码示例:
````c++
class Apples {
public:
  Apples() {
    company = "test";
    names_.push_back("abc");
  }

private:
//下面两个成员变量 是stl中的，已经实现了std::move， 所以Apples 类不需要自己实现
  std::string company;
  std::vector<std::string> names_;
};

 Apples apple1;
 // 将apple1 直接转移到了 apple2. 其实是调用了 转移构造，但是Apples并没有自己实现转移构造函数。 因为编译器有一个默认的。就像默认的拷贝构造函数一样。
 Apples apple2 = std::move(apple1);
````
`Apples apple2 = std::move(apple1);  这句代码是调用了转移构造。 并不是真的就直接转移了`
需要自己实现的代码示例:

````c++

class Apples {
public:
  Apples() {
    company_ = "conpany";
   config_ = (char*)malloc(10);
   strcpy_s(config_, 9, "test");
  }

  ~Apples() {
    if (nullptr != config_) {
      free(config_);
      config_ = nullptr;
    }
  }

  //转移构造。 转移构造的目的很多时候是为了避免"类似深拷贝"， 直接使用"类似浅拷贝"的方式。
  Apples(Apples&& right) {
    company_ = std::move(right.company_);
    config_ = right.config_;
    right.config_ = nullptr;
  }

private:
 char* config_ = nullptr;
  std::string company_;
};

  Apples apple1;
  //会调用到转移构造
  Apples apple2 = std::move(apple1);
  
Apples apple3;
//这里还需要实现一个转移赋值，否则也会出问题。
apple3 = std::move(apple2)
````



# 总结

 1. std::move 简化了一些编程，可以动态转移对象的生命周期，减少不必要的对象构造和析构。
 2. 自定义的结构体或者类 根据 成员变量是否实现了 std::move，以及是否有析构，来决定要不要实现std::move。
 3. std::move没有实质性的move, 这是一个强制类型转换，在调用的时候，匹配相应的函数。


# 参考Variant 实现

````c++

void test(){
  int a=1;
  std::stirng name="";
  postrun([=]{
    
  });


}


````