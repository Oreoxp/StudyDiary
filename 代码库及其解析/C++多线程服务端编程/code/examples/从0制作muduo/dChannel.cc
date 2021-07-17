#include "dChannel.h"
#include "dEventloop.h"

#include <sstream>

#include <poll.h>

using namespace muduo;


const int dChannel::kNoneEvent = 0;
const int dChannel::kReadEvent = POLLIN | POLLPRI;
const int dChannel::kWriteEvent = POLLOUT;


dChannel::dChannel(dEventLoop* loop, int fdArg)
  : loop_(loop),
    fd_(fdArg),
    events_(0),
    revents_(0),
    index_(-1)
{
}

void dChannel::update()
{
  loop_->updateChannel(this);
}

void dChannel::handleEvent()
{
  if (revents_ & POLLNVAL) {
    cout << "dChannel::handle_event() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL)) {
    if (errorCallback_) errorCallback_();
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (readCallback_) readCallback_();
  }
  if (revents_ & POLLOUT) {
    if (writeCallback_) writeCallback_();
  }
}


