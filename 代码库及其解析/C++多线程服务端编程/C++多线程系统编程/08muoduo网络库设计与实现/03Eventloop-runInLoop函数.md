# Eventloop :: runInLoop() 函数

​		EventLoop 有一个非常有用的功能：在它的 IO 线程内执行某个用户任务回调，即 EventLoop: :runInLoop(const Functor& cb)，其中 Functor 是 boost: : function<void()>。

​		如果用户在当前 IO 线程调用这个函数，回调会同步进行；如果用户在其他线程调用 runInLoop()，cb 会被加入队列，IO 线程会被唤醒来调用这个 Functor。

```c++
void EventLoop::runInLoop( const Functor& cb) {
	if (isInLoopThread()) {
		cb();
	} else {
		queueInLoop(cb) ;
  }
}
```

​		有了这个功能，我们就能轻易地在线程间调配任务，比方说把 TimerQueue 的成员函数调用移到其 IO 线程，这样可以在不用锁的情况下保证线程安全性。

​		由于 IO 线程平时阻塞在事件循环 EventLoop: :1oop() 的 poll(2) 调用中，为了让 IO 线程能立刻执行用户回调，我们需要设法唤醒它。传统的办法是用 pipe(2) ，IO 线程始终监视此管道的 readable 事件，在需要唤醒的时候，其他线程往管道里写一个字节，这样 IO 线程就从 IO multiplexing 阻塞调用中返回。(原理类似 HTTP long polling。)现在 Linux 有了 eventfd(2) ， 可以更高效地唤醒，因为它不必管理缓冲区。以下是 EventLoop 新增的成员。

```c++
private:
	void handleRead(); // waked up
	void doPendingFunctors();
	
	bool callingPendingFunctors;
	
	int wakeupFd_;
	// unlike in TimerQueue, which is an internal class,
	// we don't expose Channel to client .
	boost::scoped_ptr<Channel> wakeupChannel_ ;
	
	MutexLock mutex_ ;
	std::vector<Functor> pendingFunctors_; // @BuardedBy mutex_
};
```

​		wakeupChannel_ 用于处理 wakeupFd_ 上的 readable 事件，将事件分发至 handleRead() 函数。其中只有 pendingFunctors_ 暴露给了其他线程，因此用 mutex 保护。

​		queueInLoop() 的实现很简单，将 cb 放入队列，并在必要时唤醒 IO 线程。

```c++
void EventLoop::queueInLoop(const Functor& cb)
{
  {
		MutexLockGuard lock (mutex_);
		pendingFunctors_.push_back(cb);
  }
  
	if (!isInLoopThread() || callingPendingFunctors_) {
		wakeup();
	}
}
```

​		“必要时”有两种情况，如果调用 queueInLoop() 的线程不是 IO 线程，那么唤醒是必需的；如果在 IO 线程调用 queueInLoop()，而此时正在调用 pending functor，那么也必须唤醒。换句话说，只有在 IO 线程的**<u>事件回调</u>**中调用queueInLoop() 才无须 wakeup() 。看了下面 doPendingFunctors() 的调用时间点，想必读者就能明白为什么。

​		现在需要在 EventLoop::loop() 中需要增加一行代码，指向 pendingFunctors_ 中的任务回调。

```c++
while (!quit_)
{
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(kPollTimeMs,
																		&activeChannels_)
		for (ChannelList::iterator it = activeChannels_.begin() ;
			it != activeChannels_.end(); ++it) {
			(*it)->handleEvent();
		}
+		doPendingFunctors();
}
```

​		**<u>EventLoop: : doPendingFunctors() 不是简单地在临界区内依次调用 Functor，而是把回调列表 swap() 到局部变量 functors 中，这样一方面减小了临界区的长度(意味着不会阻塞其他线程调用 queueInLoop() ) ，另一方面也避免了死锁( 因为Functor 可能再调用 queueInLoop() )。</u>**

​		由于 doPendingFunctors() 调用的 Functor 可能再调用 queueInLoop(cb)，这时 queueInLoop() 就必须 wakeup()，否则这些新加的 cb 就不能被及时调用了。muduo 这里没有反复执行 doPendingFunctors() 直到 pendingFunctors_ 为空，这是有意的，否则 IO 线程有可能陷入死循环，无法处理 IO 事件。



## EventLoopThread class

​		IO 线程不一定是主线程，我们可以在任何一个线程创建并运行 EventLoop 。一个程序也可以有不止一个 IO 线程，我们可以按优先级将不同的 socket 分给不同的 IO 线程，避免优先级反转。

​		为了方便将来使用，我们定义 EventLoopThread class，这正是 one loop per thread 的本意。

​		EventLoopThread 会启动自己的线程，并在其中运行 EventLoop: :loop() 。其中关键的 startLoop() 函数定义如下，这个函数会返回新线程中 EventLoop 对象的地址，因此用条件变量来等待线程的创建与运行。

```c++
EventLoop* EventLoopThread::startLoop()
{
	assert(! thread_.started());
	thread_.start(); 
	
	{
			MutexLockGuard lock (mutex_) ;
			while (loop_ == NULL) {
				cond_.wait() ;
			}
	}

	return loop_ ;
}
```

​		线程主函数在 stack 上定义 EventLoop 对象，然后将其地址赋值给 loop_ 成员变量，最后 notify() 条件变量，唤醒 startLoop( )。

```c++
void EventLoopThread::threadFunc() {
	EventLoop loop ;
  
  {
		MutexLockGuard lock(mutex_) ;
		loop_ = &loop;
		cond_.notify();
  }
  
	loop.loop();
	//assert(exiting_ .);
}
```

​		由于 EventLoop 的生命期与线程主函数的作用域相同，因此在 threadFunc() 退出之后这个指针就失效了。好在服务程序一般不要求能安全地退出，这应该不是什么大问题。

















