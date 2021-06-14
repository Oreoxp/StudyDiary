# Default Corstructor的建构操作

[TOC]

Default Corstructor要在需要的时候被编译器产生出来

那到底什么时候才是需要的时候呢？？？

现有以下代码：

```c++
class Foo { 
public: 
  int val; 
  Foo *pnext;
};

void foobar()
{
	//程序要求bar‘s members 都被清为0 
	Foo bar ;
	if ( bar.val || bar.pnext )
			//do Some thing
	//..
}
```

​		在这个例子中，想要的到的语意是要求 Foo 有个 Default Corstructor，可以初始化他的两个 member 。这个符合 “ 在需要的时候 ” ？

​		答案是 no。其间的差距在于一个是程序需要，一个是编译器需要。如果程序需要，那就是程序员的责任；本例要承担责任的是设计 class Foo 的人，是的，上述程序**<u>不会</u>**合成一个 Default Corstructor。







