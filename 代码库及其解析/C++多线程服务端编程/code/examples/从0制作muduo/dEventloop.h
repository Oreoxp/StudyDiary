#ifndef MUDUO_NET_DEVENTLOOP_H
#define MUDUO_NET_DEVENTLOOP_H
#include <functional>
#include <utility>

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <boost/any.hpp>
#include <boost/scoped_ptr.hpp>

#include "muduo/base/CurrentThread.h"

using namespace std;
using namespace boost;
using namespace muduo;

class dChannel;
class dPoller;

class dEventLoop {
public:
    dEventLoop();
    ~dEventLoop();

    void loop();

    void quit();

    // pid_t threadId() const { return threadId_; }
    void assertInLoopThread() {
        if (!isInLoopThread()) {
         abortNotInLoopThread();
        }
    }
    bool isInLoopThread() const { 
        return threadId_ == CurrentThread::tid();
    }

    void abortNotInLoopThread();
    void updateChannel(dChannel* channel);


    static dEventLoop* getEventLoopOfCurrentThread();
private:

  typedef std::vector<dChannel*> ChannelList;
    bool looping_ = false;
    bool quit_ = false;
    const pid_t threadId_;
  boost::scoped_ptr<dPoller> poller_;
  ChannelList activeChannels_;
};

#endif  // MUDUO_NET_DEVENTLOOP_H