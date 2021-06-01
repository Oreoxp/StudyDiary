# CONST

## 1.const指针

​		const 修饰指针变量有以下三种情况。

- **A:**  const 修饰指针指向的内容，则内容为不可变量。
- **B:**  const 修饰指针，则指针为不可变量。
- **C:**  const 修饰指针和指针指向的内容，则指针和指针指向的内容都为不可变量。

**对于 A:**

```c++
const int *p = 8;
```

​		则指针指向的内容 8 不可改变。简称左定值，因为 const 位于 * 号的左边。

**对于 B:**

```c++
int a = 8;
int* const p = &a;
*p = 9; // 正确
int  b = 7;
p = &b; // 错误
```

​		对于 const 指针 p 其指向的内存地址不能够被改变，但其内容可以改变。简称，右定向。因为 const 位于 * 号的右边。

**对于 C:** **则是 A 和 B的合并**

```c++
int a = 8;
const int * const  p = &a;
```

​		这时，const p 的指向的内容和指向的内存地址都已固定，不可改变。

​		对于 A，B，C 三种情况，根据 const 位于 * 号的位置不同，我总结三句话便于记忆的话：**"左定值，右定向，const修饰不变量"**。

## 2.constexpr和常量表达式

​		**常量表达式（const expression）**是指<u>不会改变并且在编译过程就能得到计算结果的表达式</u>。

#### constexpr变量

​		**constexpr**类型可以让编译器验证变量的值是否是一个常量表达式。

```c++
constexpr int mf = 20;						//20是常量表达式
constexpr int limit = mf + 1;			//mf+1是常量表达式
constexpr int sz = size();				//只有当size是一个constexpr函数时，这才是一条正确的语句
```

​		一般来说，如果你认定了变量是一个常量表达式，那就把它声明成constexpr类型。

#### 指针和constexpr

​		在constexpr声明中如果定义了一个指针，限定符constexpr仅对指针有效，与指针所指的对象无关：

```c++
const int *p = nullptr;				//p是一个指向整型常量的指针
constexpr int *q = nullptr;		//q是一个指向整数的常量指针
```

​		p和q的类型相差甚远，p是一个指向常量的指针，而q是一个常量指针，其中的关键在于constexpr把它所定义的对象设置为顶层const。

​		与其他常量指针类似，constexpr指针既可以指向常量也可以指向一个非常量：

```c++
constexpr int *np = nullptr;		//np是一个指向整数的常量指针，其值为空
int j = 0;
constexpr int i = 42;						//i的类型是整数常量
//i和j都必须定义在函数体之外
constexpr const int *p = &i;		//p是常量指针，指向整型常量i
constexpr int *p1 = &j;					//p1是常量指针，指向整数j
```











