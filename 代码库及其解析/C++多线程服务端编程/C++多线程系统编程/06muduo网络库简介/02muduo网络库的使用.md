# 使用教程

[TOC]

​		本章主要介绍网络库的使用，设计与实现将在第 8 章讲解。

## TCP 网络编程本质论

​		基于事件的**非阻塞**网络编程是编写高性能并发网络服务程序的主流模式。

头一次使用这种方式编程通常需要转换思维模式。把原来 
`主动调用 recv(2) 来接收数据，主动调用 accept(2) 来接受新连接，主动调用 send(2) 来发送数据`
的思路换成
`注册一个收数据的回调，网络库收到数据会调用我，直接把数据提供给我，供我消费。`
`注册一个接受连接的回调，网络库接受了新连接会回调我，直接把新的连接对象传给我，供我使用。`
`需要发送数据的时候，只管往连接中写，网络库会负责无阻塞地发送。`

​		这种编程方式有点像 Win32 的消息循环，消息循环中的代码应该避免阻塞，否则会让整个窗口失去响应，同理，事件处理函数也应该避免阻塞，否则会让网络服务失去响应。



TCP 网络编程最本质的是处理三个半事件:

1. 连接的建立，包括服务端接受 ( accept ) 新连接和客户端成功发起 ( connect ) 连接。TCP 连接一旦建立， 客户端和服务端是平等的，可以各自收发数据。

2. 连接的断开，包括主动断开 ( close、shutdown ) 和被动断开 ( read(2) 返回 0 )。

3. 消息到达，文件描述符可读。这是最为重要的-一个事件，对它的处理方式决定了网络编程的风格 ( 阻塞还是非阻塞，如何处理分包，应用层的缓冲如何设计，等等 ) 。

   3.5. 消息发送完毕，这算半个。对于低流量的服务，可以不必关心这个事件;另外，这里的“发送完毕”是指将数据写人操作系统的缓冲区，将由 TCP 协议栈负责数据的发送与重传，不代表对方已经收到数据。

这其中有很多难点，也有很多细节需要注意，比方说:

​		如果要主动关闭连接，如何保证对方已经收到全部数据 ? 如果应用层有缓冲 ( 这在非阻塞网络编程中是必需的，见下文 ) ，那么如何保证先发送完缓冲区中的数据，然后再断开连接 ? 直接调用 close(2) 恐怕是不行的。

​		如果主动发起连接，但是对方主动拒绝，如何定期 ( 带 back-off 地 ) 重试 ?

​		非阻塞网络编程该用边沿触发 ( edge trigger ) 还是电平触发 ( level trigger) ?  如果是电平触发，那么什么时候关注 EPOLLOUT 事件 ? 会不会造成 busy-loop ? 如果是边沿触发，如何防止漏读造成的饥饿 ?  epoll(4) 一定比 poll(2) 快吗?

​		在非阻塞网络编程中，为什么要使用应用层发送缓冲区 ? 
​		假设应用程序需要发送 40kB 数据，但是操作系统的 TCP 发送缓冲区只有 25kB 剩余空间，那么剩下的 15kB 数据怎么办 ? 
​		如果等待 OS 缓冲区可用，会阻塞当前线程，因为不知道对方什么时候收到并读取数据。因此网络库应该把这 15kB 数据缓存起来，放到这个 TCP 链接的应用层发送缓冲区中，等 socket 变得可写的时候立刻发送数据，这样 “ 发送 ” 操作不会阻塞。如果应用程序随后又要发送 50kB 数据，而此时发送缓冲区中尚有未发送的数据 ( 若干 kB )，那么网络库应该将这 50kB 数据追加到发送缓冲区的末尾，而不能立刻尝试 write() ，因为这样有可能打乱数据的顺序。

​		在非阻塞网络编程中，为什么要使用应用层接收缓冲区？假如一次读到的数据不够一个完整的数据包，那么这些已经读到的数据是不是应该先暂存在某个地方，等剩余的数据收到之后再一并处理？
见 lighttpd 关于 \r\n\r\n 分包的 bug。假如数据是一个字节一个字节地到达，间隔 10ms，每个字节触发一次文件描述符可读 ( readable ) 事件，程序是否还能正常工作？ lighttpd 在这个问题上出过安全漏洞。

​		在非阻塞网络编程中，如何设计并使用缓冲区？一方面我们希望减少系统调用，一次读的数据越多越划算，那么似乎应该准备一个大的缓冲区。另一方面，我们希望减少内存占用。如果有 10000 个并发连接，每个连接一建立就分配各 50kB 的读写缓冲区( s )的话，将占用 1GB 内存，而大多数时候这些缓冲区的使用率很低。muduo 用 readv(2) 结合栈上空间巧妙地解决了这个问题。

​		如果使用发送缓冲区，万一接收方处理缓慢，数据会不会一直堆积在发送方， 造成内存暴涨？如何做应用层的流量控制？

​		如何设计并实现定时器？并使之与网络 IO 共用一个线程，以避免锁。

​		这些问题在 muduo 的代码中可以找到答案。



## echo 服务的实现

​		muduo 的使用非常简单，不需要从指定的类派生，也不用覆写虚函数，只需要注册几个回调函数去处理前面提到的三个半事件就行了。

​		下面以经典的 echo 回显服务为例:

		1. 定义 EchoServer class，不需要派生自任何基类。

```c++
4 #include <muduo/net/TcpServer.h>
5
6 // RFC 862
7 class EchoServer
8 {
9  public:
10	EchoServer (muduo::net::EventLoop* loop,
11							const muduo::net::InetAddress& listenAddr) ;
12
13	void start();// calls  server_.start();
14
15 private:
16	void onConnection(const muduo::net::TcpConnectionPtr& conn) ;
17
18	void onMessage(const muduo::net::TcpConnectionPtr& conn ,
19								 muduo::net::Buffer*  buf ,
20								 muduo::Timestamp  time) ;
21
22	muduo::net::EventLoop*  loop_ ;
23	muduo::net::TcpServer   server_ ;
24};
```

​		在构造函数里注册回调函数。

```c++
10	EchoServer::EchoServer(muduo::net::EventLoop* loop,
11									const muduo::net::InetAddress& listenAddr)
12		:loop_(loop),
13		 server_(loop, listenAddr, "EchoServer")
14 {
15		server_.setConnectionCallback(
16				boost::bind(&EchoServer::onConnection, this, _1));
17		server_.setMessageCallback(
18				boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
19 ]
```

2. 实现 EchoServer :: onConnection() 和 EchoServer :: onMessage() 。

```c++
26 void EchoServer::onConnection(
  			const muduo::net:: TcpConnectionPtr& conn)
27 {
28		LOG_INFO << "EchoServer - " 
  						 << conn->peerAddress().toIpPort() 
  						 << " -> "
29	 					 << conn->localAddress().toIpPort() << " is "
30						 << (conn->connected() ? "UP" : "DOWN");
31 }
32
33 void EchoServer::onMessage(
  			const muduo::net::TcpConnectionPtr& conn,
34			muduo::net::Buffer* buf ,
35			muduo::Timestamp time)
36 {
37		muduo::string  msg(buf->retrieveAllAsString());
38		LOG_INFO << conn->name() << " echo " 
  						 << msg.size() << " bytes, "
39						 << "data received at " << time.toString();
40		conn->send(msg) ;
41 }
```

​		37 行和 40 行是 echo 服务的 “ 业务逻辑 ” ：把收到的数据原封不动地发回客户端。注意我们不用担心 40 行的 send(msg) 是否完整地发送了数据，因为 muduo 网络库会帮我们管理发送缓冲区。

​		这两个函数体现了 “ 基于事件编程 ” 的典型做法，即**程序主体是被动等待事件发生，事件发生之后网络库会调用 ( 回调 ) 事先注册的事件处理函数 ( event handler )。**

​		在 onConnection() 函数中，conn 参数是 TcpConnection 对象的 shared_ptr ，TcpConnection::connected() 返回一个 bool 值，表明目前连接是建立还是断开，TcpConnection 的 peerAddress() 和 localAddress() 成员函数分别返回对方和本地的地址 ( 以 InetAddress 对象表示的 IP 和 port )。

​		在 onMessage() 函数中，conn 参数是收到数据的那个 TCP 连接； buf 是已经收到的数据，buf 的数据会累积，直到用户从中**取走(retrieve)** 数据。注意 buf 是指针，表明用户代码可以修改(消费) buffer； time 是收到数据的确切时间，即epoll_wait(2)返回的时间，注意这个时间通常比 read(2) 发生的时间略早，可以用于正确测量程序的消息处理延迟。另外，Timestamp 对象采用 pass-by-value ，而不是 pass- by-(const)reference，这是有意的，因为在 x86-64 上可以直接通过寄存器传参。

3. 在 main() 里用 EventLoop 让整个程序跑起来。

```c++
#include "echo. h"
#include <muduo/base/Logging. h>
#include <muduo/ net/EventLoop. h>
// using namespace muduo;
// using namespace muduo: :net ;
int main()
{
	LOG_INFO << "pid = " << getpid();
	muduo::net::EventLoop loop;
	muduo::net::InetAddress listenAddr(2007) ;
	EchoServer  server(&loop, listenAddr) ;
	server.start() ;
	loop.loop();
}
```





















