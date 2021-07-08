# 借 shared_ptr 实现写时拷贝

[TOC]

本章解决前面几章留下的一些问题：

- post() 和 traverse() 死锁。
- 把  Request: :print()  移出  Inventory: : printAll()  临界区。
- 解决 Request 对象析构的 race condition.

然后示范用普通 mutex 替换读写锁。

解决办法都基于同一个思路，那就是 shared_ptr 来管理共享数据。原理如下：

- shared_ptr 是引用计数型智能指针，如果当前只有一个观察者，那么引用计数的值为1。
- 对于 write 端，如果发现引用计数为 1 , 这时可以安全地修改共享对象，不必担心有人正在读它。
- 对于 read 端，在读之前把引用计数加 1 ，读完之后减 1 , 这样保证在读的期间其引用计数大于 1 ，可以阻止并发写。
- 比较难的是，对于 write 端，如果发现引用计数大于1,该如何处理? sleep()一小段时间肯定是错的。

​        先看一个简单的例子，解决之前的 post() 和 traverse() 死锁。 数据结构改成 : 

```c++
typedef std::vector<Foo> FooList;
typedef boost::shared_ptr<FooList> FooListPtr;
MutexLock mutex ;
FooListPtr g_foos;
```

​		 在 read 端，用一个栈上局部 FooListPtr 变量当做“观察者”，它使得g_ foos的引用计数增加。traverse()  函数的临界区是下面 4 - 8 行 , 临界区内只读了一次共享变量 g_foos ( 这里多线程并发读写shared_ptr，因此必须用 mutex 保护)，比原来的写法大为缩短。而且多个线程同时调用 traverse() 也不会相互阻塞。

```c++
1 void traverse()
2 {
3 	FooListPtr foos;
4		{
5			MutexLockGuard  lock(mutex) ;
6			foos = g_foos;
7			assert(!g_foos.unique());
8		}
9
10 // assert(!foos.unique()); 这个断言不成立
l1	for (std::vector<Foo>::const_iterator it = foos->begin();
12			it != foos->end(); ++it)
13	{
14		it->doit();
15	}
16 }
```

​		关键看 write 端的 post() 该如何写。按照前面的描述，如果  g_foos.unique() 为 true , 我们可以放心地在原地( in-place) 修改 FooList。如果 g_foos.unique() 为 false ，说明这时别的线程正在读取 FooList , 我们不能原地修改，而是**<u>复制一份 ( 23行 ), 在副本上修改( 27行)。这样就避免了死锁。</u>**

```c++
17 void post(const FOO& f)
18 {
19	printf("postIn");
20	MutexLockGuard lock(mutex);
21	if (!g_foos.unique())
22	{
23		g_foos.reset(new FooList(*g_foos));
24		printf("copy the whole listln"); //练习：将这句话移出临界区
25	}
26	assert(g_foos.unique());
27	g_foos->push_back(f);
28 }
```







​		解决第二个问题把 Request: :print() 移出 Inventory: :printA1l() 临界区有两个做法。其一很简单，把 requests_ 复制一份，在临界区之外遍历这个副本。

```c++
void Inventory::printAll() const
{
	std::set<Request*> requests
	{
		muduo::MutexLockGuard  lock(mutex_) ;
		requests = requests_ ;
	}
	// 遍历局部变量 requests,调用Request::print()
}
```

​		这么做有一个明显的缺点，它复制了整个 std::set 中的每个元素，开销可能会比较大。如果遍历期间没有其他人修改  requests_  ，那么我们可以减小开销，这就引出了第二种做法。

​		第二种做法的要点是用 shared_ptr 管理 std: :set，在遍历的时候先增加引用计
数，阻止并发修改。当然 Inventory: : add() 和 Inventory: :remove() 也要相应修改，采用本节前面 post() 和 traverse() 的方案。



## 用普通 mutex 替换读写锁的一个例子

​		场景 : 一个多线程的 C++ 程序，24h x 5.5d 运行。有几个工作线程 Thread Worker { 0, 1, 2, 3 }，处理客户发过来的交易请求 ; 另外有一个背景线程 ThreadBack-ground ,  不定期更新程序内部的参考数据。这些线程都跟一个hash表打交道，工作线程只读，背景线程读写，必然要用到一些同步机制，防止数据损坏。这里的示例代码用 std: :map 代替 hash 表，意思是一样的:

```c++
using namespace std;
typedef   map< string, vector< pair<string, int> > >    Map;
```

​		Map 的 key 是用户名，value 是一个 vector , 里边存的是不同 stock 的最小交易间隔，vector 已经排好序，可以用二分查找。

​       我们的系统要求工作线程尽可能的小，可以容忍背景线程的延迟略大。系统对背景线程的及时性不敏感。

​		最简单的同步办法是用读写锁 : 工作线程加读锁，背景线程加写锁。但是读写锁的开销比普通 mutex 要大，而且是写锁优先，会阻塞后面的读锁。

​		如果工作线程能用最普通的非重入 mutex 实现同步，就不必用读写锁，这能降低工作线程延迟。我们借助 shared_ptr 做到了这一点: 

```c++
class CustomerData : boost::noncopyable
{
public:
	CustomerData() : data_(new Map)
  {}
  
	int query(const string& customer, const string& stock) const;
private:
	typedef std::pair<string, int>     Entry;
	typedef std::vector<Entry>     EntryList;
	typedef std::map<string, EntryList>  Map;
	typedef boost::shared_ptr<Map>    MapPtr;
  
  void update(const string& customer, const EntryList& entries);
  
	// 用 lower_bound 在 entries 里找 stock
	static int findEntry(const EntryList& entries, const string& stock);
  
	MapPtr getData() const
  {
		MutexLockGuard lock(mutex_);
		return data_ ;
  }
  
	mutable  MutexLock  mutex_ ;
	MapPtr  data_;
};
```

​		CustomerData: :query() 就用前面说的引用计数加 1 的办法，用局部 MapPtr data 变量来持有 Map，防止并发修改。

```c++
int CustomerData::query(const string& customer, const string& stock) const
{
	MapPtr data = getData();
	// data 一旦拿到，就不再需要锁了。
	//取数据的时候只有	getData() 内部有锁，多线程并发读的性能很好。
	Map::const_iterator entries = data->find(customer) ;
	if (entries != data->end())
			return findEntry(entries->second, stock);
	else
			return -1;
}
```

​		关键看 CustomerData::update() 怎么写。**<u>既然要更新数据，那肯定得加锁，如果这时候其他线程正在读，那么不能在原来的数据上修改，得创建一个副本，在副本上修改，修改完了再替换。如果没有用户在读，那么就能直接修改，节约一次Map拷贝。</u>**

```c++
//每次收到一个customer 的数据更新
void CustomerData::update(const string& cus tomer, const EntryList& entries)
{
		MutexLockGuard lock(mutex_ ); // update 必须全程持锁
		if (!data_.unique())
    {
			MapPtr newData(new Map(*data_));
  		//在这里打印日志，然后统计日志来判断worst case 发生的次数
			data_.swap(newData) ;
    }
		assert(data_.unique());
		(*data_ )[customer] = entries;
}
```

​		注意其中用了 shared_ptr::unique() 来判断是不是有人在读，如果有人在读,那么我们不能直接修改，因为 query() 并没有全程加锁，只在 getData() 内部有锁。

​		shared_ptr::swap() 把 data_ 替换为新副本**<u>，而且我们还在锁里，不会有别的线程来读，可以放心地更新</u>**。如果别的 reader 线程已经刚刚通过 getData() 拿到了 MapPtr ,它会读到稍旧的数据。这不是问题，因为数据更新来自网络，如果网络稍有延迟，反正 reader 线程也会读到旧的数据。

​		如果每次都更新全部数据，而且始终是在同一个线程更新数据，临界区还可以进一步缩小。

```c++
MapPtr parseData(const string& message); // 解析收到的消息，返回新的 MapPtr

// 函数原型有变，此时网络，上传来的是完整的 Map 数据
void CustomerData::update(const string& message)
{
	//解析新数据，在临界区之外
	MapPtr newData = parseData(message);
	if (newData)
  {
		MutexLockGuard 1ock(mutex_);
		data_.swap(newData); // 不要用data_ = newData;
  }
	//旧数据的析构也在临界区外，进一步缩短了临界区
}
```

​		准确地说，这不是 copy-on- write , 而是 copy-on-other-reading。





