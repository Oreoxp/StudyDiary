# 这两个东西统称为返回值优化，为什么C++ 要有这个特性，先来看看两段代码
````c++
//代码片段1
std::string GetName() {
  return std::string("name"); 
}
//代码片段2
std::string GetName() {
  std::string name("abc");
  return name;
}
````
上面两个代码片段返回 std::string对象，在RVO 之前，编译器对上面的代码片段做了很多额外的工作。
`代码片段1`
````c++
//函数定义
std::string GetName() {
  return std::string("name");  //1
}
//函数调用
std::string name = GetName(); //2
//1  函数实现里面构建一个匿名的临时对象，这样会触发调用对象的构造函数
//2  函数返回的时候调用对象的拷贝构造函数。
//3  匿名临时对象析构。
````

`代码片段2`
````c++
//函数定义
std::string GetName() {
  std::string name("abc"); //1
  return name;  //2
}
//函数调用
std::string name = GetName(); //3
//1  函数实现里面构建临时对象(name)，这样会触发调用对象的构造函数
//2  函数返回的时候又会构造一个临时对象。
//3  然后将临时对象拷贝构造给调用者。
````
`代码片段2  多了两次构造和两次析构`

C++很早的版本返回对象会有这些副作用，所以为了性能很多都不会直接返回对象，而是通过函数传参的方式，将返回值 "带出去"。
````c++
//函数传入 对象的引用，这样就避免临时对象的拷贝和释放了。
void GetName(std::string& name) {
  name = "name";
}
````
`上述代码就是通过函数参数，将值带出去。`


后来估计也知道这样写太无聊了，需要先定义一个变量，然后将变量传进去，原本一行代码，必须要两行。 而且语义也不明确。所以才有了RVO, NRVO 这两个优化。这样我们就直接可以像`代码片段1`和`代码片段2`那样写了。编译器会帮我们优化掉不必要的临时对象的创建和销毁。

# 我自己是这么总结的：函数内操作一个 "返回的对象" 就像直接操作函数外部的变量一样。
````c++
//函数定义
std::string GetName() {
  std::string name_result("abc"); 
  name_result= "111111";
  name_result= "4444";
  return name_result;  
}
//函数调用
std::string name = GetName(); //
````
`GetName 函数内部操作 name_result 这个变量，就像是直接操作 调用者 name 对象一样。不会有任何副作用`