假如我有一个 Point3d 的指针和对象：

```c++
Point3d obj:
Point3d *ptr = &obj ;
```

当我这样做：

```c++
obj.normalize () ;
ptr->normalize () ;
```

时，会发生什么事呢？

其中的 Point3d::normalize() 定义如下:

```c++
Point3d Point3d::normalize() const 
{
		register float mag = magnitude () ;
		Point3d		normal ;
		normal._x = _x/mag;
		normal._y = _y/mag;
  	normal._z = _z/mag;
		return normal ;
}
  
  
  
float Point3d:magnitude () const
{
	return  sqrt(_x*_x + _y*_y + _z*_z) ;
}
```

​		答案是：我不知道！

​		 <u>C++ 支持三种类型的 member functions : **static**、**nonstatic** 和 **virtual**，每一种类型被调用的方式都不相同。</u>

​		其间差异正是下一节的主题。不过，我们虽然不能够确定 normalize() 和magnitude() 两函数是否为 virtual 或 nonvirtual，但可以确定它一定不是 static，原因有二: (1) 它直接存取 nonstatic 数据; (2) 它被声明为 const. 是的，static member functions不可能做到这两点。



























