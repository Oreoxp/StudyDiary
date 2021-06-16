# Data member 的存取

[TOC]

已知下面这段代码：

```c++
Point3d origin;
origin.x = 0.0;
```

​		你可能会问 x 的存取成本是什么？

​		答案视 x 和 Point3d 如何声明而定。x 可能是个 static member，也可能是个 nonstatic member。Point3d 可能是个独立 ( 非派生 ) 的 class，也可能是从另一个单一的 base class 派生而来；虽然可能性不高，但它甚至可能是从多重继承或虚拟继承而来。下面数节将依次检验每一种可能性.

​		在开始之前，让我先抛出一个问题。如果我们有两个定义，origin 和 pt：

```c++
Point3d origin ,*pt = &origin;
```

​		我们用他们来存取data members，像这样：

```c++
origin.X = 0.0;
pt->x = 0.0;
```

​		通过 origin 存取和通过 pt 存取，有什么重大差异吗？如果你的回答是 yes，请你从 class Point3d 和 data member x 的角度来说明差异的发生因素。我会在这一节结束前重返这个问题并提出我的答案。



## Static Data Members

​		Static data members ，按其字面意义，被编译器提出于 class 之外，一如我在1.1节所说，并**<u>被视为一个 global 变量 ( 但只在 class 生命范围之内可见 ) 。每一个member的存取许可 ( private 或 protected 或 public ) ，以及与 class 的关联，并不会导致任何空间上或执行时间上的额外负担——不论是在个别的 class objects 或是在 static data member 本身。</u>**

​		每一个 static data member只有一个实体，存放在程序的 data segment 之中（数据段，处于静态区）。每次程序参阅 ( 取用 ) static member，就会被内部转化为对该唯一的 extern 实体的直接参考操作。例如: 

```c++
// origin.chunkSize = 250;
Point3d::chunkSize = 250;

// pt->chunkSize = 250;
Point3d::chunkSize = 250;
```

​		从指令执行的观点来看，这是 C++ 语言中 “ 通过一个指针和通过一个对象来存取 member， 结论完全相同 ” 的唯一一种情况。这是因为 “ 经由 member selection  operators ( 也就是 . 运算符 ) 对一个 static data member 进行存取操作 ” **只是语法上的一种便宜行事而已**。member 其实并不在 class object 之中，因此存取 static members 并不需要通过 class object。

​		但如果 chunkSize 是一个从复杂继承关系中继承而来的 member， 又当如何?或许它是一个 “ virtual base class 的 virtual base class ” ( 或其它同等复杂的继承结构 ) 的 member 也说不定。哦，那无关紧要,程序之中对于 static members 还是只有唯一一个实体，而其存取路径仍然是那么直接。



## Nonstatic Data Members 

​		Nonstatic data members直接存放在每一个 class object 之中。除非经由明确的 ( explicit ) 或暗喻的 ( implicit ) class object，没有办法直接存取它们。只要程序员在一个 member function 中直接处理一个 nonstatic data member，所谓“ implicit class object ” 就会发生。例如下面这段码:

```c++
Point3d Point3d::translate ( const Point3d &pt) {
	x += pt.x;
	y += pt.y;
	z += pt.z;
}
```

​		表面上所看到的对于 x,  y,  z 的直接存取，事实上是经由一个 " implicit class object "  ( 由 this 指针表达 ) 完成。事实上这个函数的参数是:

```c++
// member function 的内部转化
Point3d Point3d::translate(Point3d *const this, const Point3d &pt) {
	this->x += pt.X;
	this->y += pt.y;
	this->z += pt.z;
}
```

​		欲对一个 nonstatic data member 进行存取操作,编译器需要把 class object 的起始地址加上 data member 的偏移量( offset ) 。举个例子，如果:

```c++
origin._y = 0.0;
```

那么地址 &origin. _y  将等于:

```c++
&origin + (&Point3d::_y - 1) ;
```

​		请注意其中的 -1 操作。指向 data member 的指针，其 offset 值总是被加上 1，这样可以使编译系统区分出 “ 一个指向 data member 的指针，用以指出 class 的第一个 member ” 和 “ 一个指向 data member 的指针，没有指出任何 member ”两种情况。(后面有详细讨论)

​		每一个 nonstatic data member 的偏移量 ( offset ) 在编译时期即可获知，甚至如果 member 属于一个 base class subobject ( 派生自单一或多重继承串链 ) 也是一样。因此，存取一个 nonstatic data member，其效率和存取一个 C struct member 或一个 nonderived class 的 member 是一样的。

​		现在让我们看看虚拟继承。虚拟继承将为 “ 经由 base class subobject 存取 class members ” 导入一层新的间接性，譬如:

```c++
Point3d *pt3d;
pt3d->_x = 0.0;
```

​		其执行效率在 x 是一个 struct member、 一个 class member、 单一继承、多重继承的情况下都完全相同。

​		但如果 _x 是一个 virtual base class 的 member，存取速度会比较慢一点。下一节我会验证 “ 继承对于 member 布局的影响 ”。在我们尚未进行到那里之前，请回忆本节一开始的一个问题：以两种方法存取 x 坐标，像这样:

```c++
origin.x = 0.0;
pt->x = 0.0;
```

​		“ 从 origin 存取 ” 和 “ 从 pt 存取 ” 有什么重大的差异？答案是 “ 当 Point3d 是一个 derived class，而在其继承结构中有一个 virtual base class，并且被存取的member ( 如本例的 x )是一个从该 virtual base class 继承而来的 member 时，就会有重大的差异 ” 。

​		这时候我们不能够说 pt 必然指向哪一种 class type ( 因此我们也就不知道编译时期这个 member 真正的 offset 位置 ) ，所以这个存取操作必须延迟至执行期，经由一个额外的间接导引，才能够解决。但如果使用 origin，就不会有这些问题，其类型无疑是 Point3d class， 而即使它继承自 virtual base class，members 的 offset 位置也在编译时期就固定了。一个积极进取的编译器甚至可以静态地经由origin 就解决淖对 x 的存取。

























