# 指向 Member Function 的指针

[TOC]

​		在第三章中我们已经看到了，取一个 nonstatic data member 的地址，得到的结果是该 member 在 class 布局中的 bytes 位置 ( 再加 1 )。你可以想象它是一个不完整的值，它需要被绑定于某个 class object 的地址上，才能够被存取。

​		取一个 nonstatic member function 的地址，如果该函数是 nonvirtual,  则得到的结果是它在内存中真正的地址。然而这个值也是不完全的，它也需要被绑定于某个 class object 的地址上，才能够通过它调用该函数。所有的 nonstatic member functions 都需要对象的地址 ( 以参数 this 指出 ) 。

回顾一下，一个指向 member function 的指针，其声明语法如下:

```c++
double						// return type
( Point:: *			  // class the function is member .
		pmf )					// name of pointer to member
();		 					  // argument list
```

然后我们可以这样定义并初始化该指针:

`double (Point:: *coord) () = &Point::x ;`

也可以这样指定其值:

`coord  = &Point::y;`

想调用它，可以这么做:

` origin.coord*()`

或

`(ptr->*coord) ()`

这些操作会被编译器转化为:

`//虚拟C++码
(coord)(&origin);`

和

`//虚拟C++码
(coord)(ptr);`

​		指向 member function 的指针的声明语法，以及指向 “ member selection 运算
符 ” 的指针 , 其作用是作为 this 指针的空间保留者。这也就是为什么 static member functions ( 没有 this 指针 ) 的类型是 “ 函数指针 ”，而不是 “ 指向 member function 之指针 ” 的原因.

​		使用一个 “ member function 指针 ”，如果并不用于 virtual function、 多重继承、virtual base class 等情况的话，并不会比使用一个 “ nonmember function 指针 ” 的成本更高。上述三种情况对于 “ member function 指针 ” 的类型以及调用都太过复杂。事实上 , 对于那些没有 virtual functions 或 virtual base class , 或 multiple base classes 的 classes 而言，编译器可以为它们提供相同的效率。下一节要讨论为 “ 什么 virtual functions 的出现，会使得 member function 指针 ” 更复杂化。







































