本章主要从**零**开始逐步实现一个类似muduo的基于reactor模式的C++网络库！



# 什么都不做的 eventloop

​		首先定义 EventLoop class 的基本接口：构造函数、析构函数、loop() 成员函数。注意 EventLoop 是<u>不可拷贝的</u>，因此它继承了 boost::noncopyable。muduo中的大多数 class 都是不可拷贝的，因此以后只会强调某个 class 是可拷贝的。

```c++
class EventLoop : boost::noncopyable
{
public:
	EventLoop() ;
	~EventLoop() ; 
	void loop();
	void assertInLoopThread(){
		if (!isInLoopThread())
			abortNotInLoopThread() ;
  }
  	
	bool isInLoopThread() const {
    return threadId_ == CurrentThread::tid(); 
  }
private:
	void abortNotInLoopThread() ;
	bool looping_; /* atomic */
	const pid_t  threadId_;
}; 
```

​		one loop per thread 顾名思义每个线程只能有一个 EventLoop 对象，因此EventLoop 的构造函数会检查当前线程是否已经创建了其他 EventLoop 对象，遇到错误就终止程序( LOG_ FATAL)。

​		EventLoop 的构造函数会记住本对象所属的线程( threadId_ )。创建了EventLoop 对象的线程是 IO 线程，其主要功能是运行事件循环 EventLoop::
loop() 。EventLoop 对象的生命期通常和其所属的线程一样长，它不必是 heap 对象。

```c++
__thread  EventLoop*  t_loopInThisThread = 0;

EventLoop::EventLoop()
		:looping_(false),
		 threadId_(CurrentThread::tid())
{
		LOG_ TRACE << "EventLoop created " << this 
       				 << " in thread " << threadId_ ;
		if (t_ loopInThisThread)
    {
			LOG_ FATAL << "Another EventLoop " << t_loopInThisThread
								 <<"exists in this thread " << threadId_ ;
    } else {
      loopInThisThread = this;
    }
}

EventLoop::~EventLoop()
{
	assert(! looping_);
	t_loopInThisThread = NULL;
}
```

​		既然每个线程至多有一个 EventLoop 对象，那么我们让 EventLoop 的 static 成员函数   getEventLoopOfCurrentThread() 返回这个对象。返回值可能为 NULL,如果当前线程不是 IO 线程的话。( 这个函数是 muduo 后来新加的，因此前面头文件中没有它的原型。)

```c++
EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
		return t_loopInThisThread;
}
```

​			muduo 的接口设计会明确哪些成员函数是线程安全的，可以跨线程调用;哪些成员函数只能在某个特定线程调用 ( 主要是 IO 线程 ) 。为了能在运行时检查这些 pre-condition，EventLoop 提供了 isInLoopThread() 和assertInLoopThread() 等函数( EventLoop.h L25~L33)，其中用到的 EventLoop: : abortNotInLoopThread() 函数的定义从略。

​		事件循环必须在 IO 线程执行，因此 EventLoop: :loop() 会检查这一 pre condition ( C44)。本节的 loop() 什么事都不做，等 5 秒就退出。.

```c++
void EventLoop::loop()
{
	assert(!looping_);
	assertInLoopThread() ;
	looping_ = true;
	::poll(NULL, 0, 5*1000);
	LOG_ TRACE << "EventLoop " << this << " stop looping" ;
	looping_ = false;
}
```