#include "dEventloop.h"
#include "dChannel.cc"
#include "dPoller.cc"


__thread dEventLoop* t_loopInThisThread = 0;
const int kPollTimeMs = 10000;


dEventLoop* dEventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

dEventLoop::dEventLoop()
    : threadId_(CurrentThread::tid()),
      poller_(new dPoller(this)) {
  std::cout << "dEventLoop created " << this << " in thread " << threadId_ <<std::endl;
  if (t_loopInThisThread)
  {
    std::cout << "Another dEventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_<<std::endl;
  }
  else
  {
    t_loopInThisThread = this;
  }
}


dEventLoop::~dEventLoop()
{
  std::cout  << "dEventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << CurrentThread::tid()<<std::endl;
  
  t_loopInThisThread = nullptr;
}

void dEventLoop::loop(){
    std::cout  << "dEventLoop " << this << " of thread " << threadId_
               << " loop start " << CurrentThread::tid()<<std::endl;

    if(looping_)
        return ;

    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_) {
        activeChannels_.clear();
        poller_->poll(kPollTimeMs, &activeChannels_);

        for (ChannelList::iterator it = activeChannels_.begin();
                it != activeChannels_.end(); ++it)
        {
          (*it)->handleEvent();
        }
    }
    std::cout  << "dEventLoop " << threadId_ << " stop looping" << std::endl;
    looping_ = false;
}

void dEventLoop::abortNotInLoopThread()
{
  std::cout << "dEventLoop::abortNotInLoopThread - dEventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid() << std::endl;
}

void dEventLoop::quit()
{
  quit_ = true;
  // wakeup();
}

void dEventLoop::updateChannel(dChannel* channel)
{
  if (channel->ownerLoop() != this){
      return ;
  }
  assertInLoopThread();

  poller_->updateChannel(channel);
}