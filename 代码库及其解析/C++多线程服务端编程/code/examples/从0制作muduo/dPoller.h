#ifndef MUDUO_NET_DPOLLER_H
#define MUDUO_NET_DPOLLER_H
#include <map>
#include <vector>

#include "muduo/base/Timestamp.h"
#include "dEventloop.h"

using namespace std;
using namespace muduo;
struct pollfd;


class dChannel;

class dPoller : boost::noncopyable
{
 public:
  typedef std::vector<dChannel*> ChannelList;

  dPoller(dEventLoop* loop);
  ~dPoller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  Timestamp poll(int timeoutMs, ChannelList* activeChannels);

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  void updateChannel(dChannel* channel);

  void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }

 private:
  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;

  typedef std::vector<struct pollfd> PollFdList;
  typedef std::map<int, dChannel*> ChannelMap;

  dEventLoop* ownerLoop_;
  PollFdList pollfds_;
  ChannelMap channels_;
};
#endif  // MUDUO_NET_DPOLLER_H