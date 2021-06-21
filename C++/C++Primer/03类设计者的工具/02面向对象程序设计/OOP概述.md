# OOP概述

[TOC]

​		**面向对象程序设计(object-oriented programming)**的核心思想是**<u>数据抽象</u>**、**<u>继承</u>**和**<u>动态绑定</u>**。

​		通过使用<u>数据抽象</u>，我们可以将类的接口与实现分离;
​		使用<u>继承</u>，可以定义相似的类型并对其相似关系建模;
​		使用<u>动态绑定</u>，可以在一定程度上忽略相似类型的区别，而以统一的方式使用它们的对象。



## 继承

​		通过**继承(inheritance)**联系在一起的类构成一种层次关系。通常在层次关系的根部有一个**基类(baseclass)**，其他类则直接或间接地从基类继承而来，这些继承得到的类称为**派生类( derived class)**。基类负责定义在层次关系中所有类共同拥有的成员，而每个派生类定义各自特有的成员。

​		为了对之前提到的不同定价策略建模，我们首先定义一个名为 Quote 的类，并将它作为层次关系中的基类。Quote 的对象表示按原价销售的书籍。Quote 派生出另一个名为 Bulk_ quote 的类，它表示可以打折销售的书籍。

​		这些类将包含下面的两个成员函数:

- isbn( )，返回书籍的 ISBN 编号。该操作不涉及派生类的特殊性，因此只定义在 Quote 类中。
- net_price(size_t)，返回书籍的实际销售价格，前提是用户购买该书的数量达到一定标准。这个操作显然是类型相关的，Quote 和 Bulk_ quote 都应该包含该函数。

​        在 C++ 语言中，基类将类型相关的函数与派生类不做改变直接继承的函数区分对待。对于某些函数，基类希望它的派生类各自定义适合自身的版本，此时基类就将这些函数声明成**虚函数(virtual function)**。因此，我们可以将 Quote 类编写成:

```c++
class Quote {
public:
		std::string isbn() const;
		virtual double net_price (std::size_t n) const;
};
```

​		派生类必须通过使用**类派生列表( class derivation list)**明确指出它是从哪个(哪些)基类继承而来的。类派生列表的形式是：首先是一个冒号，后面紧跟以逗号分隔的基类列表，其中每个基类前面可以有访问说明符:

```c++
class Bulk_ quote : public Quote {  // Bulk_quote 继承了 Quote
public:
		double net_price (std::size_t) const override;
);
```

因为 Bulk_ quote 在它的派生列表中使用了 public 关键字，因此我们完全可以把Bulk_quote 的对象当成 Quote 的对象来使用。

​		<u>派生类必须在其内部对所有重新定义的虚函数进行声明</u>。派生类可以在这样的函数之前加上 virtual 关键字，但是并不是非得这么做。C++11新标准允许派生类显式地注明它将使用哪个成员函数改写基类的虚函数，具体措施是在该函数的形参列表之后增加一个 **override** 关键字。



## 动态绑定

​		通过使用**动态绑定( dynamic binding)**， 我们能用同一段代码分别处理 Quote 和 Bulk_quote 的对象。例如，当要购买的书籍和购买的数量都已知时，下面的函数负责打印总的费用:

```c++
//计算并打印销售给定数量的某种书籍所得的费用
double print_total (ostream &OS,
					const Quote &item, size_t n)
{
		//根据传入 item 形参的对象类型调用 Quote::net_price
		//或者Bulk_quote::net_price
		double ret = item.net_price(n);
		os << "ISBN: " << item.isbn()		// 调用Quote::isbn
			 <<"#sold:" << n
       <<"totaldue:" << ret
       << endl;
		return ret;
}
```

该函数非常简单：它返回调用 net_price() 的结果，并将该结果连同调用 isbn() 的结果一起打印出来。

​		关于上面的函数有两个有意思的结论：因为函数 print_total 的 item 形参是基类 Quote 的一个引用，我们既能使用基类 Quote 的对象调用该函数,也能使用派生类 Bulk_quote 的对象调用它：又因为 print_total 是使用引用类型调用 net_price 函数的，实际传入 print_total 的对象类型将决定到底执行 net_price 的哪个版本：

```c++
// basic的类型是Quote;  bulk的类型是Bulk_quote
print_total(cout, basic, 20) ;//调用 Quote 的 net_price
print_total(cout, bulk, 20) ;//调用 Bulk_quote 的 net_price
```

​		第一条调用句将 Quote 对象传入 print_total， 因此当 print_total 调用 net_ price 时，执行的是 Quote 的版本；在第二条调用语句中，实参的类型是 Bulk_quote，因此执行的是 Bulk_quote 的版本(计算打折信息)。因为在上述过程中函数的运行版本由实参决定，即在运行时选择函数的版本，所以动态绑定有时又被称为**运行时绑定( run-time binding)**。

Note：在C++语言中，当我们使用基类的引用(或指针)调用一个虚函数时将发生动态绑定。

