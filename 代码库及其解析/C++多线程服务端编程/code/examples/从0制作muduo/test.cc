#include <utility>

#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include <boost/any.hpp>

#include "muduo/base/Thread.h"
#include "dEventloop.cc"


using namespace std;



void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n",
         getpid(), muduo::CurrentThread::tid());

  dEventLoop loop;
  loop.loop();
}

int main(){
    std::cout<<"main start id="<< muduo::CurrentThread::tid() << endl;
    printf("main(): pid = %d, tid = %d\n",
         getpid(), muduo::CurrentThread::tid());

    dEventLoop loop;

    muduo::Thread thread(threadFunc);
    thread.start();

    loop.loop();

    std::cout<<"main   end id="<< muduo::CurrentThread::tid() <<endl;
    pthread_exit(NULL);
}


dEventLoop* g_loop;

void threadFunc2()
{
  printf("threadFunc(): pid = %d, tid = %d\n",
         getpid(), muduo::CurrentThread::tid());
  g_loop->loop();
}

int main2(){
    std::cout<<"main start id="<< muduo::CurrentThread::tid() << endl;
    dEventLoop loop;
    g_loop = &loop;

    muduo::Thread thread(threadFunc2);
    thread.start();
    loop.loop();
    thread.join();

    std::cout<<"main   end id="<< muduo::CurrentThread::tid() <<endl;
    pthread_exit(NULL);
}