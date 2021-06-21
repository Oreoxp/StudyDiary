# Member 的各种调用方式

[TOC]

## Nonstatic Member Functions ( 非静态成员函数)

​		C++ 的设计准则之一就是: **nonstatic member function 至少必须和一般的nonmember function 有相同的效率**。也就是说，如果我们要在以下两个函数之间作选择:

```c++
float magnitude3d( const Point3d * this ) { ... }

float Point3d::magnitude3d () const { ... }
```

那么选择 member function 不应该带来什么额外负担。这是因为编译器内部已将
member 函数实体 ” 转换为对等的 “ nonmember 函数实体 ”。

​		下面就是转化步骤 :

1. 改写函数的 signature ( 意指函数原型 ) 以安插一个额外的参数到 member function 中，用以提供一个存取管道，使 class object 得以调用该函数。该额外参数被称为 this 指针:

   ```c++
   // non-const nonstatic member 之增长过程
   Point3d Point3d::magnitude( Point3d * const this )
   ```

   如果 member function是 const , 则变成:

   ```c++
   // const nonstatic member 之扩张过程
   Point3d Point3d::magnitude( const Point3d *const this )
   ```

2. 将每一个 “ 对 nonstatic data member 的存取操作 ” 改为经由 this 指针来存取。

   ```c++
   {
   return sqrt (
   	this~>_x * this->_x +
   	this->_y * this->_y +
   	this->_z * this->_z ) ;
   }
   ```

3. 将 member function 重新写成一个外部函数。对函数名称进行 “ mangling ” 处理，使它在程序中成为独一无二的语汇:

   ```c++
   extern magnitude_7Point3dFv (
   		register Point3d * const this ) ;
   ```

​        现在这个函数已经被转换好了，而其每一个调用操作也都必须转换。于是:

```c++
obj.magnitude()
```

变成了：

```c++
magnitude_7Point3dFv (&obj);
```

而

```c++
ptr- >magnitude () ;
```

变成了：

```c++
magnitude_7Point3dFv( ptr );
```

​		本章一开始所提及的 normalize ( 函数会被转化为下面的形式，其中假设已经声明有一个 Point3d copy constructor， 而 named returned value ( NRV ) 的优化也已施行:

```c++
//以下描述 " named return value 函数”的内部转化
//使用C++伪码
void
normalize_7Point3dFv ( register const Point3d *const this ,
Point3d & result)
{
	register float mag = thi s->magnitude () ;
  
	// default constructor
	_result. Point3d::Point3d () ;
  
	_resu1t._x = this->_x/mag;
	_result._y = this->_y/mag;
	_result._z = this->_z/mag;
  
	return;
}
```

​		一个比较有效率的做法是直接建构 “ normal ” 值，像这样:

```c++
Point3d Point3d::normalize() const
{
	register float mag = magnitude () ;
	return Point3d(_ x/mag,_ y/mag, z/mag ) ;
}
```

​		它会被转化为以下的码(我再一次假设 Point3d 的 copy constructor 已经声明好了，而 NRV 的优化也已实施):

```c++
//以下描述内部转化
//使用C++伪码
void
nocmalize_7Point3dFv ( register const Point3d *const this,
											Point3d &_ result)
{
	register float mag = this->magnitude() ;
  
	//result 用以取代返回值 ( return value )
	result.Point3d::Point3d (
    this->_ x/mag, this->_y/mag, this->_z/mag ) ;
  
	return;
}
```

这可以节省 default constructor 初始化所引起的额外负担。



## Virtual Member Functions ( 虚拟成员函数)

​		如果 normalize() 是一个 virtual member function，那么以下的调用:

```c++
ptr->normalize();
```

将会被内部转化为:

```c++
(* ptr->vptr[1] ) ( ptr );
```

其中:

- vptr 表示由编译器产生的指针, 指向 virtual table。它被安插在每一个“声明有 ( 或继承自 ) 一个或多个 virtual functions "  的 class object 中。事实上其名称也会被 “ mangled ”，因为在一个复杂的 class 派生体系中，可能存在有多个 vptrs。
- 1 是 virtual table slot 的索引值，关联到 normalize() 函数.
- 第二个 ptr 表示 this 指针。

​        类似的道理 , 如果 magnitude()  是一个 virtual function , 它在 normalize()  之中的调用操作将被转换如下:

```c++
// register float mag = magnitude() ;
register float mag = ( *this->vptr[ 2 ] ) ( this );
```

​		此时，由于 Point3d::magnitude() 是在 Point3d::normalize() 中被调用，而后者已经由虚拟机制而**决议 ( resolved )** 妥当，所以明确地调用 “ Point3d 实体 ” 会比较有效率，并因此压制由于虚拟机制而产生的不必要的重复调用操作:

```c++
//明确的调用操作 ( explicitly invocation ) 会压制虚拟机制
register float mag = Point3d::magnitude () ;
```

​		如果 magnitude() 声明为 inline 函数会更有效率。使用 class scope operator 明确调用一个 virtual function， 其决议(resolved) 方式会和 nonstatic member function 一样：

​		`register float mag = magnitude_7Point3dFv( this ) ;`

对于以下调用:

`//Point3d obj ;
obj.normalize() ;`

如果编译器把它转换为:

`( * obj.vptr [1]) (&obj);`

​		虽然语意正确，却没有必要。请回忆那些并不支持多态 ( polymorphism ) 的对象(1.3 节)。所以上述经由 obj 调用的函数实体只可以是 Point3d::normalize()，“ 经由一个 class object 调用一个 virtual function ”，这种操作应该总是被编译器像对待一般的 nonstatic member function 一样地加以决议 ( resolved ) :

`normalize_7Point3dFv (&obj)`

​		这项优化工程的另一利益是，virtual function 的一个 inline 函数实体可以被扩展 ( expanded ) 开来，因而提供极大的效率利益。

​		Virtual functions , 特别是它们在继承机制下的行为，将在4.2节有比较详细
的讨论。











