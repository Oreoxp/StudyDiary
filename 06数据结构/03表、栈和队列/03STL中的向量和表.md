## STL中的向量和表

​		在 C++ 语言的库中包含有公共数据结构的实现。C++ 中的这部分内容就是众所周知**的标准模板库（Standard Template Library，STL)**。表ADT就是在STL中实现的数据结构之一。其他的数据结构我们将在第 4 章中介绍。一般来说，这些数据结构称为**集合（collection)**或**容器（container)**。 

​		表ADT有两个流行的实现。vector 给出了表 ADT 的可增长的数组实现。使用 vector 的优点在于其在常量的时间里是可索引的。缺点是插入新项或删除己有项的代价是昂贵的，除非是这些操作发生在 vector 的末尾。list 提供了表 ADT 的双向链表实现。使用 list 的优点是，如果变化发生的位置已知的话，插入新项和删除已有项的代价是很小的。缺点是 list 不容易索引。vector 和 list 两者在查找时效率都很低。在本讨论中，list 总是指 STL 中的双向链表，而 “ 表 ” 则是指更一般的 ADT 表。

​		vector 和 list 两者都是用其包含的项的类型来例示的类模板。两者都有几个公共的方法。 所示的前三个方法事实上对所有的 STL 容器都适用：

* int size () const : 返回容器内的元素个数。

* void clear () : 删除容器中所有的元素。

* bool empty() : 如果容器没有元素，返回 true，否则返回 false。

​       vector 和 list 两者都支持在常量的时间内在表的末尾添加或删除项。vector 和 list 两者都支持在常量的时间内访问表的前端的项。这些操作如下： 

* void push_back( const Object & x ):在表的末尾添加 x。

* void pop_back() : 删除表的末尾的对象。

* const Object & back() const : 返回表的末尾的对象（也提供返回引用的修改函数）。

* const Object & front () const : 返回表的前端的对象（也提供返回引用的修改函数）。

​        因为双向链表允许在表的前端进行高效的改变，但是 vector 不支持，所以，下面的两个方法仅对 list 有效：

* void push_front( const Object & x):在list的前端添加X。

* void pop_front():在list的前端删除对象。

​        vector 也有 list 所不具有的特有的方法。有两个方法可以进行高效的索引。另外两个方法允许程序员观察和改变 vector 的内部容量。这些方法是：

* Object & operator [] (int idx) : 返回 vector 中 idx 索引位置的对象，不包含边界检测（也提供返回常量引用的访问函数）。

* Object & at ( int idx) : 返回 vector 中 idx 索引位置的对象，包含边界检测（也提供返回常量引用的访问函数）。

* int: capacity() const : 返回 vector 的内部容量（详细信息参见3*4节）。

* void reserve ( int new Capacity ) : 设定 vector 的新容量。如果己有良好的估计的话，这可以避免对 vector 进行扩展（详细信息见3.4节）。