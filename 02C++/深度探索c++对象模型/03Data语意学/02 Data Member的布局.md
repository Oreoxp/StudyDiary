# Data Member 的布局 ( Data Member Layout)

[TOC]

已知下面一组 data members：

```c++
class Point3d {
public :
	//...
private :
	float  x;
	static List<Point3d*>  *freeList ;
	float  y;
	static const int  chunkSize = 250;
	float  z;
};
```

​		<u>**Nonstatic data members 在 class object 中的排列顺序将和其被声明的顺序一样，任何中间介入的 static data members 如 freeList 和 chunkSize 都不会被放进对象布局之中**</u>。

​		在上述例子里，每一个 Point3d 对象是由三个 float 组成，次序是 x,  y,  z。static data members 存放在程序的 data segment 中，和个别的 class objects 无关。

​		C++ Standard 要求,在同一个 access section ( 也就是 private、 public、protected 等区段)中，members 的排列只需符合 “ 较晚出现的 members 在 class object 中有较高的地址 ” 这一条件即可。也就是说，各个 members 并不一定得连续排列。什么东西可能会介于被声明的 members 之间呢？ members 的**边界调整 ( alignment )** 可能就需要填补一些 bytes。对于 C 和 C++ 而言，这的确是真的，对当前的 C++ 编译器实现情况而言，这也是真的。

​		编译器还可能会合成一些内部使用的 data members，以支持整个对象模型，vptr 就是这样的东西，当前所有的编译器都把它安插在每一个 “ 内含 virtual function 之 class ”  的 object 内。vptr 会被放在什么位置呢 ? 传统上它被放在所有明确声明的 members 的最后。不过如今也有一些编译器把 vptr 放在一个 class object 的最前端。























