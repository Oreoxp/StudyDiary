#ifndef MUDUO_NET_DCHANNEL_H
#define MUDUO_NET_DCHANNEL_H


#include <boost/function.hpp>
#include <boost/noncopyable.hpp>


class dEventLoop;


class dChannel : boost::noncopyable
{
 public:
  typedef boost::function<void()> EventCallback;

  dChannel(dEventLoop* loop, int fd);

  void handleEvent();
  void setReadCallback(const EventCallback& cb)
  { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading() { events_ |= kReadEvent; update(); }
  // void enableWriting() { events_ |= kWriteEvent; update(); }
  // void disableWriting() { events_ &= ~kWriteEvent; update(); }
  // void disableAll() { events_ = kNoneEvent; update(); }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  dEventLoop* ownerLoop() { return loop_; }

 private:
  void update();

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  dEventLoop* loop_;
  const int  fd_;
  int        events_;
  int        revents_;
  int        index_; // used by Poller.

  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback errorCallback_;
};
#endif  // MUDUO_NET_DCHANNEL_H