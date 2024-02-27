[TOC]



# 网页渲染调度器（Scheduler）实现分析

​		在采用线程化渲染方式渲染网页时，Chromium 依赖一个调度器协调 Main 线程和 Compositor 线程的执行，同时也通过这个调度器决定它们什么时候该执行什么操作。调度器将 Main 线程和 Compositor 线程的当前状态记录在一个状态机中，然后通过这个状态机决定下一个要执行的操作。这个操作在满足当前设置条件下是最优的，因此可以使网页渲染更快更流畅。本文接下来就分析 Chromium 网页调度器的实现。

​		调度器实现在 Chromium 的 CC 模块中，并且运行在 Compositor 线程中。Compositor 线程的职责是将网页的内容渲染出来。从这个角度看，调度器只不过是在调度 Compositor 线程的执行。不过由于要渲染的网页内容是由 Main 线程提供给 Compositor 线程的，因此调度器也会在必要的时候调度 Main 线程执行，使得它可以提供最新的网页内容给 Compositor 线程渲染。

​		网页是一帧一帧地渲染出来的。从前面Android应用程序UI硬件加速渲染技术简要介绍和学习计划这个系列的文章我们学习到，Android 应用程序 UI 的每一帧的最佳渲染时机是下一个屏幕 VSync 信号到来时。Chromium 也不例外，它在渲染网页的时候，也是利用了屏幕的 VSync 信号。这一点在调度器的时间轴中可以得到体现，如图1所示：

![img](markdownimage/20160408020056830)

​		从图可以看到，调度器并没有严格在 VSync 到来时就去渲染网页的下一帧，而是为网页的下一帧渲染时机设置了一个 Deadline。在 Deadline 到来前，调度器可以调度执行其它的渲染操作。

​		在继续分析上述的 Deadline 机制之前，我们要先搞清楚网页的一帧渲染涉及到哪些操作。这些操作如图2所示：

![img](markdownimage/20160404024430755)

​		图2的完整分析可以参考前面Chromium网页渲染机制简要介绍和学习计划一文。我们前面说的 Deadline，是针对第 6 个操作 ACTION_DRAW_AND_SWAP_FORCED 而言的。也就是说，当 VSync 信号到来时，ACTION_DRAW_AND_SWAP_FORCED 操作最迟必须在设置的 Deadline 到来时执行。

​		**这个 Deadline 是怎么计算出来的呢？**我们先来看网页的渲染过程。首先是 Render 进程进行渲染，然后交给Browser 进程进行合成。因此，网页的渲染过程可以看作由两部分时间组成：estimated_draw_duration + estimated_browser_composite_time。其中，estimated_draw_duration 表示 Render 进程的渲染时间，estimated_browser_composite_time 表示 Browser 进程的合成时间。

​		假设下一个 VSync 到来的时间为 frame_time，VSync 信号时间周期为 interval，那么就可以计算出 `Deadline = frame_time + (interval - estimated_draw_duration - estimated_browser_composite_time)`。剩下来的时间区间 [frame_time, deadline) 可以用做其它事情，例如执行图2所示的第 2 个操作 ACTION_SEND_BEGIN_MAIN_FRAME，也就是通知 Main 线程对 CC Layer Tree 进行绘制。

​		时间区间 [frame_time, deadline) 称为 BEGIN_IMPL_FRAME 时间。在 BEGIN_IMPL_FRAME 时间内，存在四个 BeginImplFrameState 状态，如下所示：

```c++
class CC_EXPORT SchedulerStateMachine {
 public:
  ......
 
  // Note: BeginImplFrameState will always cycle through all the states in
  // order. Whether or not it actually waits or draws, it will at least try to
  // wait in BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME and try to draw in
  // BEGIN_IMPL_FRAME_STATE_INSIDE_DEADLINE
  enum BeginImplFrameState {
    BEGIN_IMPL_FRAME_STATE_IDLE,
    BEGIN_IMPL_FRAME_STATE_BEGIN_FRAME_STARTING,
    BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME,
    BEGIN_IMPL_FRAME_STATE_INSIDE_DEADLINE,
  };
 
  ......
 
 protected:
  ......
 
  BeginImplFrameState begin_impl_frame_state_;
  
  ......
};
```

​		调度器通过类 SchedulerStateMachine 描述内部的状态机。状态机的 BeginImplFrameState 状态记录在SchedulerStateMachine 类的成员变量 begin_impl_frame_state_ 中。

​		四个 BeginImplFrameState 状态分别为：

1. BEGIN_IMPL_FRAME_STATE_IDLE

2. BEGIN_IMPL_FRAME_STATE_BEGIN_FRAME_STARTING

3. BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME

4. BEGIN_IMPL_FRAME_STATE_INSIDE_DEADLINE

它们的变迁关系如图3所示：

![img](markdownimage/20160405023640597)

下一个 VSync 信号到来之前，状态机处于 BEGIN_IMPL_FRAME_STATE_IDLE 状态。

​		下一个 VSync 信号到来之时，调度器调用 SchedulerStateMachine 类的成员函数 OnBeginImplFrame 将状态机的 BeginImplFrameState 状态设置为 BEGIN_IMPL_FRAME_STATE_BEGIN_FRAME_STARTING。这时候调度器通过调用 Scheduler 类的成员函数 ProcessScheduledActions 调度计算网页中的动画，或者执行图2所示的第2个操作 ACTION_SEND_BEGIN_MAIN_FRAME，也就是通知 Main 线程对网页内容进行绘制。

​		从 Scheduler 类的成员函数 ProcessScheduledActions 返回后，BeginImplFrameState 状态就从BEGIN_IMPL_FRAME_STATE_BEGIN_FRAME_STARTING 变为BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME。这时候调度器等待 Deadline 的到来。

​		Deadline 到来之时，调度器调用 SchedulerStateMachine 类的成员函数 OnBeginImplFrameDeadline 将BeginImplFrameState 状态从 BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME 设置为 BEGIN_IMPL_FRAME_STATE_INSIDE_DEADLINE。这时候调度器就会通过调用 Scheduler 类的成员函数ProcessScheduledActions 调度执行网页的渲染操作，也就是图2所示的第6个操作 ACTION_DRAW_AND_SWAP_FORCED。

​		从 Scheduler 类的成员函数 ProcessScheduledActions 返回后，BeginImplFrameState 状态就从BEGIN_IMPL_FRAME_STATE_INSIDE_DEADLINE 重新变为 BEGIN_IMPL_FRAME_STATE_IDLE。这时候调度器等待再下一个 VSync 信号的到来。



​		状态机除了有 BeginImplFrameState 状态，还有其它三个状态，分别是 OutputSurfaceState、CommitState 和 ForcedRedrawOnTimeoutState。

​		OutputSurfaceState 描述的是网页绘图表面的状态，如下所示：

```c++
class CC_EXPORT SchedulerStateMachine {
 public:
  ......
 
  enum OutputSurfaceState {
    OUTPUT_SURFACE_ACTIVE,
    OUTPUT_SURFACE_LOST,
    OUTPUT_SURFACE_CREATING,
    OUTPUT_SURFACE_WAITING_FOR_FIRST_COMMIT,
    OUTPUT_SURFACE_WAITING_FOR_FIRST_ACTIVATION,
  };
 
  ......
 
 protected:
  ......
 
  OutputSurfaceState output_surface_state_;
  
  ......
};
```

​		网页绘图表面的状态记录在 SchedulerStateMachine 类的成员变量 output_surface_state_ 中。网页绘图表面的状态有五个状态，分别是：

1. OUTPUT_SURFACE_LOST

2. OUTPUT_SURFACE_CREATING

3. OUTPUT_SURFACE_WAITING_FOR_FIRST_COMMIT

4. OUTPUT_SURFACE_WAITING_FOR_FIRST_ACTIVATION

5. OUTPUT_SURFACE_ACTIVE

它们的变迁关系如图 4 所示：

![img](markdownimage/20160405030148856)

​		网页绘图表面最初处于 OUTPUT_SURFACE_LOST 状态，等到 CC Layer Tree 创建之后，调度器会调度图2所示的第 2 个操作 ACTION_BEGIN_OUTPUT_SURFACE_CREATION，也就是请求 Compositor 线程为网页创建绘图表面，这时候网页绘图表面的状态变为 OUTPUT_SURFACE_CREATING。

​		Compositor 线程为网页创建好了绘图表面之后，就会调用 SchedulerStateMachine 类的成员函数DidCreateAndInitializeOutputSurface 将绘图表面的状态设置为OUTPUT_SURFACE_WAITING_FOR_FIRST_COMMIT。这将会触发调度器尽快调度执行图2所示的第2个操作ACTION_SEND_BEGIN_MAIN_FRAME 和第 3 个操作 ACTION_COMMIT，也就是请求 Main 线程对 CC Layer Tree 进行绘制，并且将其同步到 CC Pending Layer Tree 中去。这时候绘图表面的状态变为OUTPUT_SURFACE_WAITING_FOR_FIRST_ACTIVATION，表示要尽快将 CC Pending Layer Tree 激活为 CC Active Layer Tree。

​		CC Pending Layer Tree 被激活之后，也就是图2所示的第5个操作 ACTION_ACTIVATE_PENDING_TREE 执行之后，绘图表面以后就会一直处于 OUTPUT_SURFACE_ACTIVE 状态。不过，在绘图表面处于OUTPUT_SURFACE_ACTIVE 状态期间，如果 Render 进程与 GPU 进程之间的 GPU 通道断开了连接，或者 GPU 进程在解析 Render 进程发送来的 GPU 命令时发生了错误，那么 SchedulerStateMachine 类的成员函数DidLoseOutputSurface 会被调用。这时候绘图表面的状态就会被设置为 OUTPUT_SURFACE_LOST 状态。这将会触发调度器调度执行 ACTION_BEGIN_OUTPUT_SURFACE_CREATION 操作，以便为网页重新创建绘图表面。





​		CommitState 描述的是 CC Layer Tree 的提交状态，包括同步到 CC Pending Layer Tree，以及 CC Pending Layer Tree 激活为 CC Active Layer Tree 的过程，如下所示：

```c++
class CC_EXPORT SchedulerStateMachine {
 public:
  ......
 
  enum CommitState {
    COMMIT_STATE_IDLE,
    COMMIT_STATE_BEGIN_MAIN_FRAME_SENT,
    COMMIT_STATE_BEGIN_MAIN_FRAME_STARTED,
    COMMIT_STATE_READY_TO_COMMIT,
    COMMIT_STATE_WAITING_FOR_ACTIVATION,
    COMMIT_STATE_WAITING_FOR_FIRST_DRAW,
  };
 
  ......
 
 private:
  ......
 
  CommitState commit_state_;
 
  ......
};
```

​		CC Layer Tree 的提交状态记录在 SchedulerStateMachine 类的成员变量 commit_state_ 中。CC Layer Tree有六个提交状态，分别是：

1. COMMIT_STATE_IDLE

2. COMMIT_STATE_BEGIN_MAIN_FRAME_SENT

3. COMMIT_STATE_BEGIN_MAIN_FRAME_STARTED

4. COMMIT_STATE_READY_TO_COMMIT

5. COMMIT_STATE_WAITING_FOR_ACTIVATION

6. COMMIT_STATE_WAITING_FOR_FIRST_DRAW

它们的变迁关系如图5所示：

![img](markdownimage/20160406024210799)

​		CC Layer Tree 的提交状态最开始时被设置为 COMMIT_STATE_IDLE。当调度器调度执行图2所示的第2个操作ACTION_BEGIN_MAIN_FRAME 时，CC Layer Tree 的提交状态被设置为COMMIT_STATE_BEGIN_MAIN_FRAME_SENT，表示调度器已经请求 Main 线程对 CC Layer Tree 进行绘制。

​		在 Main 线程执行 ACTION_BEGIN_MAIN_FRAME 操作期间，CC Layer Tree 有可能变为不可见，这时候调度器就会调用 SchedulerStateMachine 类的成员函数 BeginMainFrameAborted 重新设置为 COMMIT_STATE_IDLE。

​		Main 线程执行完成 ACTION_BEGIN_MAIN_FRAME 操作之后，调度器就会调用 SchedulerStateMachine 类的成员函数 NotifyReadyToCommit 将 CC Layer Tree 的提交状态设置为COMMIT_STATE_BEGIN_MAIN_FRAME_STARTED。这时候 Main 线程会通知 Compositor 线程对网页中的图片资源以纹理方式上传到 GPU 去，以便后面进行渲染显示。

​		图片资源上传完毕，调度器就会调用 SchedulerStateMachine 类的成员函数 NotifyReadyToCommit 将 CC Layer Tree 的提交状态设置为 COMMIT_STATE_READY_TO_COMMIT，表示 CC Layer Tree 需要同步到 CC Pending Layer Tree 中去。这将会触发调度器调度执行图2所示的第三个操作 ACTION_COMMIT，也就是将 CC Layer Tree 同步到 CC Pending Layer Tree 中去。

​		ACTION_COMMIT 操作执行完成之后，CC Layer Tree 的提交状态会从COMMIT_STATE_READY_TO_COMMIT 变为以下三个状态之一：

1. 在满足以下两个条件之一时，变为COMMIT_STATE_IDLE：
   1. main_frame_before_activation_enabled 被设置为 true。这表示在上一个 CC Pending Layer Tree 被激活为 CC Active Layer Tree 之前，允许 Main 线程绘制网页的下一帧。
   2. main_frame_before_draw_enabled 被设置为 true，但是 impl_side_painting 被设置为 false。main_frame_before_draw_enabled 设置为 true，表示在上一个 CC Active Layer Tree 被渲染之前，允许 Main 线程绘制网页的下一帧。impl_side_painting 设置为 true 表示 Main 线程在绘制网页时，实际上只是记录了网页的绘制命令。只有在 impl_side_painting 设置为 true 的时候，才会有 CC Pending Layer Tree 被激活为 CC Active Layer Tree 的环节。因此，在 impl_side_painting 等于 false 的情况下，main_frame_before_draw_enabled 被设置为 true 等同于 main_frame_before_activation_enabled 被设置为 true 的情况。
2. FORCED_REDRAW_STATE_WAITING_FOR_ACTIVATION。需要满足的条件是main_frame_before_draw_enabled 和 impl_side_painting 均被设置为 true，并且main_frame_before_activation_enabled 被设置为 false。这表示在上一个 CC Pending Layer Tree 被激活为CC Active Layer Tree 之后，才允许 Main 线程绘制网页的下一帧。
3. COMMIT_STATE_WAITING_FOR_FIRST_DRAW。需要满足的条件是 main_frame_before_activation_enabled 和 main_frame_before_draw_enabled 均被设置为 true。这表示在上一个 CC Active Layer Tree 第一次渲染之后，才允许 Main 线程绘制网页的下一帧。这实际上是给予 CC Active Layer Tree 更高的优先级，使得它一激活就马上进行渲染。

如果 CC Layer Tree 的提交状态处于 FORCED_REDRAW_STATE_WAITING_FOR_ACTIVATION，那么当 CC Pending Layer Tree 被激活为 CC Active Layer Tree 之后，也就是图2所示的第5个操作ACTION_ACTIVATE_PENDING_TREE 执行之后，CC Layer Tree 的提交状态就变为 COMMIT_STATE_IDLE。

如果 CC Layer Tree 的提交状态处于 COMMIT_STATE_WAITING_FOR_FIRST_DRAW，那么当 CC Active Layer Tree 被渲染之后，也就是图2所示的第6个操作 ACTION_DRAW_AND_SWAP_FORCED，或者另外一个操作ACTION_DRAW_AND_SWAP_IF_POSSIBLE 执行之后，CC Layer Tree 的提交状态就变为 COMMIT_STATE_IDLE。

​		注意，只有 CC Layer Tree 的提交状态处于 COMMIT_STATE_IDLE 时，Main 线程才可以绘制网页的下一帧。





ForcedRedrawOnTimeoutState 描述的是网页的渲染状态，如下所示：

```c++
class CC_EXPORT SchedulerStateMachine {
 public:
  ......
 
  enum ForcedRedrawOnTimeoutState {
    FORCED_REDRAW_STATE_IDLE,
    FORCED_REDRAW_STATE_WAITING_FOR_COMMIT,
    FORCED_REDRAW_STATE_WAITING_FOR_ACTIVATION,
    FORCED_REDRAW_STATE_WAITING_FOR_DRAW,
  };
 
  ......
 
 private:
  ......
 
  ForcedRedrawOnTimeoutState forced_redraw_state_;
 
  ......
};
```

​		网页的渲染状态记录在 SchedulerStateMachine 类的成员变量 forced_redraw_state_ 中。网页的渲染状态有四个，分别是：

1. FORCED_REDRAW_STATE_IDLE,

2. FORCED_REDRAW_STATE_WAITING_FOR_COMMIT
3. FORCED_REDRAW_STATE_WAITING_FOR_ACTIVATION
4. FORCED_REDRAW_STATE_WAITING_FOR_DRAW

它们的变迁关系如图6所示：

![img](markdownimage/20160406025850243)

​		网页渲染状态最开始处于 FORCED_REDRAW_STATE_IDLE 状态。在渲染动画的过程中，如果某些分块（Tile）还没有光栅化好，那么 CC 模块就会用棋盘（Checkboard）来代替这些缺失的分块。这时候的网页渲染结果被视为 DRAW_ABORTED_CHECKERBOARD_ANIMATIONS。如果网页连续渲染结果都是DRAW_ABORTED_CHECKERBOARD_ANIMATIONS 的次数超出预设值，那么网页渲染状态就会被设置为FORCED_REDRAW_STATE_WAITING_FOR_COMMIT ，表示要尽快执行一次图2所示的第3个操作ACTION_COMMIT，以便补充缺失的分块。

​		ACTION_COMMIT 操作执行完成之后，如果 Main 线程在绘制网页时，仅仅记录了 CC Layer Tree 的绘制命令，也就是前面提到的 impl_side_painting 等于 true，那么就意味着存在一个 CC Pending Layer Tree，这时候网页渲染状态会被设置为 FORCED_REDRAW_STATE_WAITING_FOR_ACTIVATION，表示等待 CC Pending Layer Tree 被激活为 CC Active Layer Tree。

​		另一方面，ACTION_COMMIT 操作执行完成之后，如果 Main 线程在绘制网页时，直接进行光栅化，也就是前面提到的 impl_side_painting 等于 false，那么就意味着不会存在一个 CC Pending Layer Tree，这时候网页渲染状态会被设置为 FORCED_REDRAW_STATE_WAITING_FOR_DRAW，表示等待 CC Active Layer Tree 被渲染。

​		如果网页渲染状态被设置为 FORCED_REDRAW_STATE_WAITING_FOR_ACTIVATION，那么当图2所示的第5个操作 ACTION_ACTIVATE_PENDING_TREE 执行完成之后。也就是 CC Pending Layer Tree 被激活为 CC Active Layer Tree 之后，网页渲染状态会被设置为 FORCED_REDRAW_STATE_WAITING_FOR_DRAW，表示等待 CC Active Layer Tree 被渲染。

​		当 CC Active Layer Tree 被渲染之后，网页渲染状态就会从FORCED_REDRAW_STATE_WAITING_FOR_DRAW 变为 FORCED_REDRAW_STATE_IDLE 状态。









​		理解了网页的BeginImplFrameState、OutputSurfaceState、CommitState和ForcedRedrawOnTimeoutState状态之后，接下来我们就可以分析调度器的实现了，也就是调度器的执行过程。

​		调度器通过 Scheduler 类实现，调度器的执行过程就表现为 Scheduler 类的成员函数ProcessScheduledActions 不断地被调用，如下所示：

```c++
void Scheduler::ProcessScheduledActions() {
  ......
 
  SchedulerStateMachine::Action action;
  do {
    action = state_machine_.NextAction();
    ......
    state_machine_.UpdateState(action);
    base::AutoReset<SchedulerStateMachine::Action>
        mark_inside_action(&inside_action_, action);
    switch (action) {
      case SchedulerStateMachine::ACTION_NONE:
        break;
      case SchedulerStateMachine::ACTION_ANIMATE:
        client_->ScheduledActionAnimate();
        break;
      case SchedulerStateMachine::ACTION_SEND_BEGIN_MAIN_FRAME:
        client_->ScheduledActionSendBeginMainFrame();
        break;
      case SchedulerStateMachine::ACTION_COMMIT:
        client_->ScheduledActionCommit();
        break;
      case SchedulerStateMachine::ACTION_UPDATE_VISIBLE_TILES:
        client_->ScheduledActionUpdateVisibleTiles();
        break;
      case SchedulerStateMachine::ACTION_ACTIVATE_PENDING_TREE:
        client_->ScheduledActionActivatePendingTree();
        break;
      case SchedulerStateMachine::ACTION_DRAW_AND_SWAP_IF_POSSIBLE:
        DrawAndSwapIfPossible();
        break;
      case SchedulerStateMachine::ACTION_DRAW_AND_SWAP_FORCED:
        client_->ScheduledActionDrawAndSwapForced();
        break;
      case SchedulerStateMachine::ACTION_DRAW_AND_SWAP_ABORT:
        // No action is actually performed, but this allows the state machine to
        // advance out of its waiting to draw state without actually drawing.
        break;
      case SchedulerStateMachine::ACTION_BEGIN_OUTPUT_SURFACE_CREATION:
        client_->ScheduledActionBeginOutputSurfaceCreation();
        break;
      case SchedulerStateMachine::ACTION_MANAGE_TILES:
        client_->ScheduledActionManageTiles();
        break;
    }
  } while (action != SchedulerStateMachine::ACTION_NONE);
 
  SetupNextBeginFrameIfNeeded();
  ......
 
  if (state_machine_.ShouldTriggerBeginImplFrameDeadlineEarly()) {
    ......
    ScheduleBeginImplFrameDeadline(base::TimeTicks());
  }
}
```

​		Scheduler 类的成员函数 ProcessScheduledActions 在一个 while 循环中不断地调用成员变量state_machine_ 指向的 SchedulerStateMachine 对象的成员函数 NextAction 询问状态机下一个要执行的操作，直到状态机告知当前没有操作要执行为止。这些操作大概就对应于图2所示的操作。

​		每一个操作都是通过调用成员变量 client_ 指向的 ThreadProxy 对象对应的成员函数执行的。例如，ACTION_SEND_BEGIN_MAIN_FRAME 操作是通过调用成员变量 client_ 指向的 ThreadProxy 对象的成员函数ScheduledActionSendBeginMainFrame 执行的。ThreadProxy 类的这些函数我们在后面的文章中再详细分析。每一个操作在执行之前，Scheduler 类的成员函数 ProcessScheduledActions 会先调用 state_machine_ 指向的SchedulerStateMachine 对象的成员函数 UpdateState 更新状态机的状态。

​		跳出 while 循环之后，Scheduler 类的成员函数 ProcessScheduledActions 调用另外一个成员函数SetupNextBeginFrameIfNeeded 根据状态机的当前状态决定是否要发起下一个 BEGIN_IMPL_FRAME 操作。如果需要的话，就会在下一个 VSync 信号到来时，通过调用 Scheduler 类的成员函数 ProcessScheduledActions 渲染网页的下一帧。

​		Scheduler 类的成员函数 ProcessScheduledActions 最后还会调用成员变量 state_machine_ 指向的SchedulerStateMachine 对象的成员函数 ShouldTriggerBeginImplFrameDeadlineEarly 检查是否提前执行渲染网页的操作。如果需要提前的话，那么就不会等待上一个 VSync 信号到来时设置的 Deadline 到期，而是马上调用成员函数 ScheduleBeginImplFrameDeadline 假设该 Deadline 已经到期，于是就可以马上渲染网页。

​		为了更好地理解调度器的执行过程，接下来我们继续前面提到的 SchedulerStateMachine 类的成员函数NextAction、UpdateState 和 ShouldTriggerBeginImplFrameDeadlineEarly 以及 Scheduler 类的成员函数SetupNextBeginFrameIfNeeded 和 ScheduleBeginImplFrameDeadline 的实现。

​		SchedulerStateMachine 类的成员函数 NextAction 的实现如下所示：

```c++
SchedulerStateMachine::Action SchedulerStateMachine::NextAction() const {
  if (ShouldUpdateVisibleTiles())
    return ACTION_UPDATE_VISIBLE_TILES;
  if (ShouldActivatePendingTree())
    return ACTION_ACTIVATE_PENDING_TREE;
  if (ShouldCommit())
    return ACTION_COMMIT;
  if (ShouldAnimate())
    return ACTION_ANIMATE;
  if (ShouldDraw()) {
    if (PendingDrawsShouldBeAborted())
      return ACTION_DRAW_AND_SWAP_ABORT;
    else if (forced_redraw_state_ == FORCED_REDRAW_STATE_WAITING_FOR_DRAW)
      return ACTION_DRAW_AND_SWAP_FORCED;
    else
      return ACTION_DRAW_AND_SWAP_IF_POSSIBLE;
  }
  if (ShouldManageTiles())
    return ACTION_MANAGE_TILES;
  if (ShouldSendBeginMainFrame())
    return ACTION_SEND_BEGIN_MAIN_FRAME;
  if (ShouldBeginOutputSurfaceCreation())
    return ACTION_BEGIN_OUTPUT_SURFACE_CREATION;
  return ACTION_NONE;
}
```

​		SchedulerStateMachine 类的成员函数 NextAction 通过调用一系列的成员函数 ShouldXXX 决定当前需要执行的操作。例如，如果调用成员函数 ShouldCommit 得到的返回值为 true，那么就 SchedulerStateMachine 类的成员函数 NextAction 就会返回一个 ACTION_COMMIT 值表示要执行一个 ACTION_COMMIT 操作。这些ShouldXXX 成员函数我们在后面的文章中再详细分析。

​		SchedulerStateMachine 类的成员函数 UpdateState 的实现如下所示：

```c++
void SchedulerStateMachine::UpdateState(Action action) {
  switch (action) {
    case ACTION_NONE:
      return;
 
    case ACTION_UPDATE_VISIBLE_TILES:
      last_frame_number_update_visible_tiles_was_called_ =
          current_frame_number_;
      return;
 
    case ACTION_ACTIVATE_PENDING_TREE:
      UpdateStateOnActivation();
      return;
 
    case ACTION_ANIMATE:
      last_frame_number_animate_performed_ = current_frame_number_;
      needs_animate_ = false;
      // TODO(skyostil): Instead of assuming this, require the client to tell
      // us.
      SetNeedsRedraw();
      return;
 
    case ACTION_SEND_BEGIN_MAIN_FRAME:
      DCHECK(!has_pending_tree_ ||
             settings_.main_frame_before_activation_enabled);
      DCHECK(!active_tree_needs_first_draw_ ||
             settings_.main_frame_before_draw_enabled);
      DCHECK(visible_);
      commit_state_ = COMMIT_STATE_BEGIN_MAIN_FRAME_SENT;
      needs_commit_ = false;
      last_frame_number_begin_main_frame_sent_ =
          current_frame_number_;
      return;
 
    case ACTION_COMMIT: {
      bool commit_was_aborted = false;
      UpdateStateOnCommit(commit_was_aborted);
      return;
    }
 
    case ACTION_DRAW_AND_SWAP_FORCED:
    case ACTION_DRAW_AND_SWAP_IF_POSSIBLE: {
      bool did_request_swap = true;
      UpdateStateOnDraw(did_request_swap);
      return;
    }
 
    case ACTION_DRAW_AND_SWAP_ABORT: {
      bool did_request_swap = false;
      UpdateStateOnDraw(did_request_swap);
      return;
    }
 
    case ACTION_BEGIN_OUTPUT_SURFACE_CREATION:
      DCHECK_EQ(output_surface_state_, OUTPUT_SURFACE_LOST);
      output_surface_state_ = OUTPUT_SURFACE_CREATING;
 
      // The following DCHECKs make sure we are in the proper quiescent state.
      // The pipeline should be flushed entirely before we start output
      // surface creation to avoid complicated corner cases.
      DCHECK_EQ(commit_state_, COMMIT_STATE_IDLE);
      DCHECK(!has_pending_tree_);
      DCHECK(!active_tree_needs_first_draw_);
      return;
 
    case ACTION_MANAGE_TILES:
      UpdateStateOnManageTiles();
      return;
  }
}
```

​		参数 action 表示调度器即将要调度执行的操作，SchedulerStateMachine 类的成员函数 UpdateState 根据这个操作相应地更新状态机的状态。例如，如果调度器即将要调度执行的操作为 ACTION_COMMIT，那么SchedulerStateMachine 类的成员函数 UpdateState 就会调用另外一个成员函数 UpdateStateOnCommit 更新状态机的 CommitState 状态。这些状态的更新过程我们同样是在后面的文章再详细分析。

​		SchedulerStateMachine 类的成员函数 ShouldTriggerBeginImplFrameDeadlineEarly 的实现如下所示：

```c++
bool SchedulerStateMachine::ShouldTriggerBeginImplFrameDeadlineEarly() const {
  // TODO(brianderson): This should take into account multiple commit sources.
 
  if (begin_impl_frame_state_ != BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME)
    return false;
 
  // If we've lost the output surface, end the current BeginImplFrame ASAP
  // so we can start creating the next output surface.
  if (output_surface_state_ == OUTPUT_SURFACE_LOST)
    return true;
 
  // SwapAck throttle the deadline since we wont draw and swap anyway.
  if (pending_swaps_ >= max_pending_swaps_)
    return false;
 
  if (active_tree_needs_first_draw_)
    return true;
 
  if (!needs_redraw_)
    return false;
 
  // This is used to prioritize impl-thread draws when the main thread isn't
  // producing anything, e.g., after an aborted commit. We also check that we
  // don't have a pending tree -- otherwise we should give it a chance to
  // activate.
  // TODO(skyostil): Revisit this when we have more accurate deadline estimates.
  if (commit_state_ == COMMIT_STATE_IDLE && !has_pending_tree_)
    return true;
 
  // Prioritize impl-thread draws in smoothness mode.
  if (smoothness_takes_priority_)
    return true;
 
  return false;
}
```

​		如前所述，当 SchedulerStateMachine 类的成员函数 ShouldTriggerBeginImplFrameDeadlineEarly 返回true 的时候，就表示要马上渲染网页的下一帧，而不要等待上一个 VSync 到来时所设置的 Deadline。这隐含着SchedulerStateMachine 类的成员函数 ShouldTriggerBeginImplFrameDeadlineEarly 返回 true 的一个必要条件，那就是调度器当前必须是在等待 Deadline 的到来。

​		从前面的分析可以知道，当 SchedulerStateMachine 类的成员变量 begin_impl_frame_state_ 的值等于BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME 的时候，就表示调度器正在等待 Deadline 的到来。因此，当SchedulerStateMachine 类的成员变量 begin_impl_frame_state_ 的值不等于BEGIN_IMPL_FRAME_STATE_INSIDE_BEGIN_FRAME 的时候，可以马上断定 SchedulerStateMachine 类的成员函数ShouldTriggerBeginImplFrameDeadlineEarly 返回 true 的必要条件不满足。

​		接下来有四种情况会导致 SchedulerStateMachine 类的成员函数ShouldTriggerBeginImplFrameDeadlineEarly 返回 true。

​		第一种情况是网页的绘图表面表还没有创建，或者之前已经创建过，但是现在丢失了。在这种情况下，SchedulerStateMachine 类的成员变量 output_surface_state_ 的值等于 OUTPUT_SURFACE_LOST。

​		第二种情况是上一个 CC Pending Layer Tree 刚刚激活为 CC Active Layer Tree。在这种情况下，SchedulerStateMachine 类的成员变量 active_tree_needs_first_draw_ 的值等于 true。CC Pending Layer Tree被激活为 CC Active Layer Tree 时，说明网页很可能发生了较大的变化，也就是它有可能需要花费更多的渲染时间，因此需要提前进行渲染，而不是等到 Deadline 到来时再渲染。不过，这种情况还需要满足另外一个条件，就是 Render 进程当前请求 GPU 进程执行的 SwapBuffers 操作未完成的次数 pending_swaps_ 不能大于等于预先设置的阀值 max_pending_swaps_。

​		第三种情况是虽然 CC Layer Tree 没有发生新的变化需要同步给 CC Pending Layer Tree，但是网页当前被要求重新进行渲染，也就是对当前的 CC Active Layer Tree 进行渲染。这时候 SchedulerStateMachine 类的成员变量 commit_state_ 的值等于 COMMIT_STATE_IDLE，并且成员变量 needs_redraw_ 的值等于 true。这种情况出现在 CC Layer Tree 上一次将变化同步给 CC Pending Layer Tree 时还没有完成就被取消。不过，如果这时候存在一个 CC Pending Layer Tree，那么 SchedulerStateMachine 类的成员函数ShouldTriggerBeginImplFrameDeadlineEarly 就不会返回 true，目的是让已经存在的 CC Pending Layer Tree 优先渲染出来，而不是继续渲染当前的 CC Active Layer Tree。

​		第四种情况是对第三种情况的补充，也就是网页当前的状态为：

1. 被要求重新渲染，也就是成员变量 needs_redraw_ 的值等于 true。

2. CC Layer Tree 有发生了新的变化，并且正在同步到 CC Pending Layer Tree，或者上一次同步 CC Layer Tree得到的 CC Pending Layer Tree 还未完成光栅化。

这时候本应该让已经存在的 CC Pending Layer Tree 优先渲染，或者让 CC Layer Tree 同步后得到的新 CC Pending Layer Tree 优先渲染。但是如果 SchedulerStateMachine 类的成员变量 smoothness_takes_priority_ 的值等于 true，那么优先渲染的是当前的 CC Active Layer Tree。也就是说，当网页被要求重新渲染时，不要等待新的内容准备就绪，要马上对现有的就绪内容进行渲染。因此，SchedulerStateMachine 类的成员变量smoothness_takes_priority_ 的值等于 true 时，表示的意思是网页的现有内容显示优先于新内容显示，即使现有内容是较旧的。这种体验带来的好处是让用户觉得网页显示很流畅，不是每次都一卡一顿地显示最新的内容。

​		接下来我们重点分析调度器准备下一个 BEGIN_IMPL_FRAME 操作的过程，也就是 Scheduler 类的成员函数SetupNextBeginFrameIfNeeded 的实现，如下所示：

```c++
void Scheduler::SetupNextBeginFrameIfNeeded() {
  bool needs_begin_frame = state_machine_.BeginFrameNeeded();
 
  if (settings_.throttle_frame_production) {
    SetupNextBeginFrameWhenVSyncThrottlingEnabled(needs_begin_frame);
  } else {
    SetupNextBeginFrameWhenVSyncThrottlingDisabled(needs_begin_frame);
  }
  SetupPollingMechanisms(needs_begin_frame);
}
```

​		Scheduler 类的成员函数 SetupNextBeginFrameIfNeeded 首先调用成员变量 state_machine_ 指向的SchedulerStateMachine 对象的成员函数 BeginFrameNeeded 询问状态机是否需要重新绘制网页。在需要的情况下，才可能会请求在下一个 VSync 信号到来时对网页进行重绘。这一点容易理解，如果网页不需要重绘，那么下一帧就继续显示上一帧的内容即可。

​		并不是所有的平台都支持 VSync 信号。如果当前平台不支持 VSync 信号，那么 Scheduler 类的成员变量settings_ 描述的一个 SchedulerSettings 对象的成员变量 throttle_frame_production 的值会等于 false，这时候Scheduler 类的成员函数 SetupNextBeginFrameIfNeeded 调用另外一个成员函数SetupNextBeginFrameWhenVSyncThrottlingDisabled 简单地根据帧率来决定下一个 BEGIN_IMPL_FRAME 操作的时间点。例如，假设屏幕的刷新频率为 60fps，那么 Scheduler 类的成员函数SetupNextBeginFrameWhenVSyncThrottlingDisabled 就会在 1000 / 60 = 16.67 毫秒后发起下一个BEGIN_IMPL_FRAME 操作。

​		这里我们只考虑平台支持 VSync 信号，这时候 Scheduler 类的成员函数 SetupNextBeginFrameIfNeeded 就会调用另外一个成员函数 SetupNextBeginFrameWhenVSyncThrottlingEnabled 设置在下一个 VSync 信号到来时执行一个 BEGIN_IMPL_FRAME 操作。

​		BEGIN_IMPL_FRAME 操作是通过调用 Scheduler 类的成员函数 ProcessScheduledActions 执行的，它的职责对网页进行渲染，特指图2所示的 ACTION_DRAW_AND_SWAP_FORCED 操作。但是，Scheduler 类的成员函数ProcessScheduledActions 在执行的时候，根据当时状态机的状态，也可能会执行其它类型的操作。这样BEGIN_IMPL_FRAME 操作就会起到向前推进网页的渲染管线的作用，也就是使得网页不会停留在一个中间状态的作用。例如，状态机的 CommitState 状态不等于 COMMIT_STATE_IDLE 的时候，就是一个中间状态。假如这时候Scheduler 类的成员函数 SetupNextBeginFrameWhenVSyncThrottlingEnabled 通过计算发现不需要在下一个VSync 信号到来时发起一个 BEGIN_IMPL_FRAME 操作。这就会意味着 Scheduler 类的成员函数ProcessScheduledActions 很可能在接下来一段时间里不会被调用。这将会造成状态机停留在 CommitState 中间状态。

​		为了避免状态机长时间停留在中间状态，调度器提供了一种 Polling 机制，它会定时地调用 Scheduler 类的成员函数 ProcessScheduledActions，这样就可以不断地将网页的渲染管线向前推进，直到到达稳定状态。这种Polling 机制是通过调用 Scheduler 类的成员函数 SetupPollingMechanisms 运作的。Scheduler 类的成员函数SetupPollingMechanisms 的实现与前面提到的另外一个成员函数SetupNextBeginFrameWhenVSyncThrottlingDisabled 类似，都是根据屏幕的刷新频率来定时调用 Scheduler 类的成员函数 ProcessScheduledActions 的。

​		接下来我们主要分析 SchedulerStateMachine 类的成员函数 BeginFrameNeeded 和 Scheduler 类的成员函数SetupNextBeginFrameWhenVSyncThrottlingEnabled 的实现，Scheduler 类的另外两个成员函数SetupNextBeginFrameWhenVSyncThrottlingDisabled 和 SetupPollingMechanisms 读者可以根据前面的解释进行自行分析。

​		SchedulerStateMachine 类的成员函数 BeginFrameNeeded 的实现如下所示：

```c++
bool SchedulerStateMachine::BeginFrameNeeded() const {
  // Proactive BeginFrames are bad for the synchronous compositor because we
  // have to draw when we get the BeginFrame and could end up drawing many
  // duplicate frames if our new frame isn't ready in time.
  // To poll for state with the synchronous compositor without having to draw,
  // we rely on ShouldPollForAnticipatedDrawTriggers instead.
  if (!SupportsProactiveBeginFrame())
    return BeginFrameNeededToAnimateOrDraw();
 
  return BeginFrameNeededToAnimateOrDraw() || ProactiveBeginFrameWanted();
}
```

​		SchedulerStateMachine 类的成员函数 BeginFrameNeeded 分两种情况决定当前是否是发起一个BEGIN_IMPL_FRAME 操作。第一种情况是网页使用的合成器不支持主动发起 BEGIN_IMPL_FRAME 操作。第二种情况与第一种情况相反。

​		网页使用的合成器是否支持主动发起 BEGIN_IMPL_FRAME 操作，可以通过调用 SchedulerStateMachine 类的成员函数 SupportsProactiveBeginFrame 判断，它的实现如下所示：

```c++
// Note: If SupportsProactiveBeginFrame is false, the scheduler should poll
// for changes in it's draw state so it can request a BeginFrame when it's
// actually ready.
bool SchedulerStateMachine::SupportsProactiveBeginFrame() const {
  // It is undesirable to proactively request BeginFrames if we are
  // using a synchronous compositor because we *must* draw for every
  // BeginFrame, which could cause duplicate draws.
  return !settings_.using_synchronous_renderer_compositor;
}
```

​		当 SchedulerStateMachine 类的成员变量 settings_ 描述的 SchedulerSettings 对象的成员变量using_synchronous_renderer_compositor 等于 false 的时候，SchedulerStateMachine 类的成员函数SupportsProactiveBeginFrame 的返回值就等于 true，表示网页使用的合成器支持主动发起 BEGIN_IMPL_FRAME 操作。

​		当 SchedulerStateMachine 类的成员变量 settings_ 描述的 SchedulerSettings 对象的成员变量using_synchronous_renderer_compositor 等于 true 的时候，表示网页使用的合成器是**同步合成器（Synchronous Renderer Compositor）**。同步合成器适用在 WebView 的情景。WebView 是嵌入在应用程序窗口中显示网页的，它渲染网页的方式与独立的浏览器应用有所不同。简单来说，就是**<u>每当 WebView 需要重绘网页，它需要向应用程序窗口发送一个 Invalidate 消息。应用程序窗口接下来就会调用 WebView 的 onDraw 函数。一旦 WebView 的 onDraw 函数被调用，它就必须准备好要合成的内容。也就是它不能设置一个Deadline，等到 Deadline 到期时再去合成的内容。这就是所谓的“同步”，而设置 Deadline 的方式是“异步”的。</u>**

​		回到 SchedulerStateMachine 类的成员函数 BeginFrameNeeded 中，如果网页使用的是同步合成器，那么就只有在调用另外一个成员函数 BeginFrameNeededToAnimateOrDraw 得到的返回值等于 true 的时候，SchedulerStateMachine 类的成员函数 BeginFrameNeeded 的返回才会等于 true。

​		SchedulerStateMachine 类的成员函数 BeginFrameNeededToAnimateOrDraw 的实现如下所示：

```c++
// These are the cases where we definitely (or almost definitely) have a
// new frame to animate and/or draw and can draw.
bool SchedulerStateMachine::BeginFrameNeededToAnimateOrDraw() const {
  // The output surface is the provider of BeginImplFrames, so we are not going
  // to get them even if we ask for them.
  if (!HasInitializedOutputSurface())
    return false;
 
  // If we can't draw, don't tick until we are notified that we can draw again.
  if (!can_draw_)
    return false;
 
  // The forced draw respects our normal draw scheduling, so we need to
  // request a BeginImplFrame for it.
  if (forced_redraw_state_ == FORCED_REDRAW_STATE_WAITING_FOR_DRAW)
    return true;
 
  // There's no need to produce frames if we are not visible.
  if (!visible_)
    return false;
 
  // We need to draw a more complete frame than we did the last BeginImplFrame,
  // so request another BeginImplFrame in anticipation that we will have
  // additional visible tiles.
  if (swap_used_incomplete_tile_)
    return true;
 
  if (needs_animate_)
    return true;
 
  return needs_redraw_;
}
```

​		SchedulerStateMachine 类的成员函数 BeginFrameNeededToAnimateOrDraw 返回 true 需要满足两个必要条件：

1. 网页当前的绘图表面可用，也就是网页的绘图表面已经创建，并且没有失效。这可以通过调用SchedulerStateMachine 类的成员函数 HasInitializedOutputSurface 判断。

2. 网页的上一个 CC Pending Layer Tree 已经被激活为 CC Active Layer Tree。这时候 SchedulerStateMachine类的成员变量 can_draw_ 的值会等于 true。

​        满足了以上两个必要条件后，有四种情况会使得 SchedulerStateMachine 类的成员函数BeginFrameNeededToAnimateOrDraw 返回 true。

​		第一种情况是状态机的 ForcedRedrawOnTimeoutState 状态等于FORCED_REDRAW_STATE_WAITING_FOR_DRAW。从图6可以知道，这时候网页正在等待 CC Pending Layer Tree 激活为 CC Active Layer Tree，以便得到的 CC Active Layer Tree。现在既然 CC Pending Layer Tree 已经激活，因此就需要对网页执行一个渲染操作。

​		后面三种情况需要满足第三个必要条件，就是网页当前是可见的。这时候 SchedulerStateMachine 类的成员变量 visible_ 会等于 true。

​		第二种情况是网页上次渲染时，有些分块（Tile）还没有准备就绪，也就是还没有光栅化完成。这时候SchedulerStateMachine 类的成员变量 swap_used_incomplete_tile_ 会等于 true。这种情况也要求执行一个渲染操作。在执行这个渲染操作的时候，调度器会检查之前未准备就绪的分块是否已经就准备就绪。如果已经准备就绪，那么就可以对它们进行渲染。

​		第三种情况是网页现在正处于动画显示过程中。这时候 SchedulerStateMachine 类的成员变量needs_animate_ 的值会等于 true。这时候要求执行一个渲染操作，就可以使得动画持续执行下去。

​		第四种情况是网页被要求进行重新绘制，或者是因为 CC Pending Layer Tree 刚刚激活为 CC Active Layer Tree，或者网页的 CC Layer Tree 上一次同步到 CC Pending Layer Tree 的过程中还没有完成就被取消了。这时候要求执行一个渲染操作，就可以使得刚刚激活得到的 CC Active Layer Tree 可以马上进行渲染，或者恢复 CC Layer Tree 同步到 CC Pending Layer Tree 的操作。

​		回到 SchedulerStateMachine 类的成员函数 BeginFrameNeeded 中，如果网页使用的不是同步合成器，那么除了调用成员函数 BeginFrameNeededToAnimateOrDraw 得到的返回值等于 true 的情况，还有另外一种情况也会使得 SchedulerStateMachine 类的成员函数 BeginFrameNeeded 的返回值等于 true，就是调用另外一个成员函数 ProactiveBeginFrameWanted 得到的返回值也等于 true。

​		SchedulerStateMachine 类的成员函数 ProactiveBeginFrameWanted 的实现如下所示：

```c++
// Proactively requesting the BeginImplFrame helps hide the round trip latency
// of the SetNeedsBeginFrame request that has to go to the Browser.
bool SchedulerStateMachine::ProactiveBeginFrameWanted() const {
  // The output surface is the provider of BeginImplFrames,
  // so we are not going to get them even if we ask for them.
  if (!HasInitializedOutputSurface())
    return false;
 
  // Do not be proactive when invisible.
  if (!visible_)
    return false;
 
  // We should proactively request a BeginImplFrame if a commit is pending
  // because we will want to draw if the commit completes quickly.
  if (needs_commit_ || commit_state_ != COMMIT_STATE_IDLE)
    return true;
 
  // If the pending tree activates quickly, we'll want a BeginImplFrame soon
  // to draw the new active tree.
  if (has_pending_tree_)
    return true;
 
  // Changing priorities may allow us to activate (given the new priorities),
  // which may result in a new frame.
  if (needs_manage_tiles_)
    return true;
 
  // If we just sent a swap request, it's likely that we are going to produce
  // another frame soon. This helps avoid negative glitches in our
  // SetNeedsBeginFrame requests, which may propagate to the BeginImplFrame
  // provider and get sampled at an inopportune time, delaying the next
  // BeginImplFrame.
  if (HasRequestedSwapThisFrame())
    return true;
 
  return false;
}
```

​		SchedulerStateMachine 类的成员函数 ProactiveBeginFrameWanted 返回 true 需要满足两个必要条件： 

1. 网页的绘图表面已经创建好，并且没有失效。

2. 网页当前是可见的。

满足了以上两个必要条件后，有五种情况会使得 SchedulerStateMachine 类的成员函数ProactiveBeginFrameWanted 返回 true。

​		第一种情况是 Main 线程通知调度器网页的 CC Layer Tree 有新的变化，需要同步到 CC Pending Layer Tree去。这时候 SchedulerStateMachine 类的成员变量 needs_commit_ 会等于 true。

​		第二种情况是状态机的 CommitState 状态不等于 COMMIT_STATE_IDLE。这意味着 Compositor 线程正在执行同步 CC Layer Tree 和 CC Pending Layer Tree 的操作。为了正常推进这个操作，需要一个BEGIN_IMPL_FRAME 操作，以便触发 Scheduler 类的成员函数 ProcessScheduledActions 的调用。这样就可以保证正常推进 CC Layer Tree 和 CC Pending Layer Tree 的同步操作。

​		第三种情况是网页存在一个 CC Pending Layer Tree。这时候 SchedulerStateMachine 类的成员变量has_pending_tree_ 会等于 true。这时候同样需要通过一个 BEGIN_IMPL_FRAME 操作推进这个 CC Pending Layer Tree 激活为 CC Active Layer Tree。

​		第四种情况是 Compositor 线程需要对网页的分块进行光栅化操作。这时候 SchedulerStateMachine 类的成员变量 needs_manage_tiles_ 会等于 true。这种情况同样需要通过一个 BEGIN_IMPL_FRAME 操作推进光栅化操作的执行。

​		第五种情况是网页的当前帧已经渲染好，并且 Render 进程也已经向 GPU 发起了一个 SwapBuffers 操作，也就是请求 Browser 进程将网页的 UI 显示出来。这时候调用 SchedulerStateMachine 类的成员函数HasRequestedSwapThisFrame 得到的返回值为 true。这种情况请求执行下一个 BEGIN_IMPL_FRAME 操作，可以尽快地检查网页是否需要绘制下一帧，也就是让 Main 线程或者 Compositor 线程尽快准备好下一个帧。

​		对比 SchedulerStateMachine 类的成员函数 ProactiveBeginFrameWanted 和BeginFrameNeededToAnimateOrDraw 的实现可以发现，前者比后者更激进地请求执行下一个BEGIN_IMPL_FRAME 操作。后者在被动要求重绘网页下一帧的时候才会返回 true，而前者会主动去准备网页的下一帧的绘制操作。

​		回到 Scheduler 类的成员函数 SetupNextBeginFrameIfNeeded 中，它通过调用 SchedulerStateMachine 类的成员函数 SupportsProactiveBeginFrame 获知是否要发起下一个 BEGIN_IMPL_FRAME 操作的信息后，如果平台支持 VSync 信号，那么接下来它会调用另外一个成员函数SetupNextBeginFrameWhenVSyncThrottlingEnable 根据 VSync 信号来准备下一个 BEGIN_IMPL_FRAME 操 作。

​		Scheduler 类的成员函数 SetupNextBeginFrameWhenVSyncThrottlingEnable 的实现如下所示：

```c++
// When we are throttling frame production, we request BeginFrames
// from the OutputSurface.
void Scheduler::SetupNextBeginFrameWhenVSyncThrottlingEnabled(
    bool needs_begin_frame) {
  bool at_end_of_deadline =
      state_machine_.begin_impl_frame_state() ==
          SchedulerStateMachine::BEGIN_IMPL_FRAME_STATE_INSIDE_DEADLINE;
 
  bool should_call_set_needs_begin_frame =
      // Always request the BeginFrame immediately if it wasn't needed before.
      (needs_begin_frame && !last_set_needs_begin_frame_) ||
      // Only stop requesting BeginFrames after a deadline.
      (!needs_begin_frame && last_set_needs_begin_frame_ && at_end_of_deadline);
 
  if (should_call_set_needs_begin_frame) {
    if (settings_.begin_frame_scheduling_enabled) {
      client_->SetNeedsBeginFrame(needs_begin_frame);
    } else {
      synthetic_begin_frame_source_->SetNeedsBeginFrame(
          needs_begin_frame, &begin_retro_frame_args_);
    }
    last_set_needs_begin_frame_ = needs_begin_frame;
  }
 
  PostBeginRetroFrameIfNeeded();
}
```

​		在两种情况下，Scheduler类的成员函数 SetupNextBeginFrameWhenVSyncThrottlingEnable 会请求执行下一个 BEGIN_IMPL_FRAME 操作。

​		第一种情况是调度器之前没有请求过调度执行下一个BEGIN_IMPL_FRAME操作，这时候Scheduler类的成员变量last_set_needs_begin_frame_等于false。此时参数needs_begin_frame的值又等于true，也就是状态机要求执行一个BEGIN_IMPL_FRAME操作。这时候调度器就必须要调度执行一个BEGIN_IMPL_FRAME操作。否则的话，就永远不会执行下一个BEGIN_IMPL_FRAME操作。

​		第二种情况是调度器之前请求过调度执行下一个BEGIN_IMPL_FRAME操作，并且这个BEGIN_IMPL_FRAME操作的Deadline已经到来，以及参数needs_begin_frame的值等于false。试想这种情况，如果调度器不继续请求调度执行下一个BEGIN_IMPL_FRAME操作的话，网页的渲染管线在上一次请求的BEGIN_IMPL_FRAME操作执行完成后，就断开了。因此在这种情况下，也需要请求请示调度执行下一个BEGIN_IMPL_FRAME操作。

​		如果对上述两种情况做一个总结，就是只有在前两个连续的 BEGIN_IMPL_FRAME 操作期间，状态机都表示不需要绘制网页的下一帧的情况下，调度器才会停止请求调度执行下一个BEGIN_IMPL_FRAME操作。通过这种方式保证网页的渲染管线可以持续地推进到稳定状态，而不会停留在上面提到的中间状态。



​		一旦决定需要请求调度执行下一个 BEGIN_IMPL_FRAME 操作，本地变量should_call_set_needs_begin_frame 的值就会被设置为 true，这时候如果 Scheduler 类的成员变量 settings_ 描述的 SchedulerSettings 对象的成员变量 begin_frame_scheduling_enabled 的值等于 true，那么 Scheduler 类的成员函数 SetupNextBeginFrameWhenVSyncThrottlingEnabled 就会调用成员变量 client_ 指向的一个ThreadProxy 对象的成员函数 SetNeedsBeginFrame 请求调度执行下一个 BEGIN_IMPL_FRAME 操作，否则的话，调用另外一个成员变量 synthetic_begin_frame_source_ 指向的一个 SyntheticBeginFrameSource 对象的成员函数 SetNeedsBeginFrame 请求调度执行下一个 BEGIN_IMPL_FRAME 操作。

​		SyntheticBeginFrameSource 类是通过软件方式调度执行 BEGIN_IMPL_FRAME 操作的，实际上就是通过定时器，根据屏幕的刷新频率模拟生成 VSync 信号。这种方式产生的 VSync 信号会受到定时器精度的影响。例如，假设屏幕刷新频率为 60fps，那么就应该每 16.67ms 生成一个 VSync 信号。如果定时器只能精确到整数毫秒，那么就意味着定时器只能在 16ms 或者 17ms 后触发定时器。如果一直都是使用 16ms，那么就会导致 VSync 信号的产生频率大于 60fps。如果一直都使用 17ms，那么就会导致 VSync 信号的产生频率小于 60fps。为了弥补定时精度带来的缺陷，SyntheticBeginFrameSource 类会自动调整定时器的 Timeout 时间，使得 VSync 信号的平均周期等于 16.67ms，也就是第一个 VSync 信号 16ms 后产生，第二个 VSync 信号17ms 后产生，第三个 VSync 信号17ms 后产生，第四个 VSync 信号 16ms 后产生......，依次类推。这一点是 SyntheticBeginFrameSource 类通过使用另外一个类 DelayBasedTimeSource 实现的。有兴趣的读者可以自行分析 DelayBasedTimeSource 类的实现。

​		我们假设 Scheduler 类的成员变量 settings_ 描述的 SchedulerSettings 对象的成员变量begin_frame_scheduling_enabled 的值等于 true，这表示屏幕支持以硬件方式产生 VSync 信号。在这种情况下，就直接利用屏幕产生的 VSync 信号调度 BEGIN_IMPL_FRAME 操作即可。如前所述，这是通过调用ThreadProxy 类的成员函数 SetNeedsBeginFrame 实现的。

​		请求了调度下一个BEGIN_IMPL_FRAME操作之后，Scheduler类的成员函数SetupNextBeginFrameWhenVSyncThrottlingEnabled还会调用另外一个成员函数PostBeginRetroFrameIfNeeded检查之前调度的BEGIN_IMPL_FRAME操作是否都已经得到执行。有时候之前请求调度的BEGIN_IMPL_FRAME操作在下一个VSync信号到来时，会被延时执行。这些被延时的BEGIN_IMPL_FRAME操作会被保存在一个队列中，等待Scheduler类的成员函数SetupNextBeginFrameWhenVSyncThrottlingEnabled执行。注意，这个延时与BEGIN_IMPL_FRAME操作设置的Deadline是无关的。一个BEGIN_IMPL_FRAME操作只有被执行时，才可以设置Deadline。这一点我们后面再进行分析。

​		接下来我们继续分析ThreadProxy类的成员函数SetNeedsBeginFrame的实现，以便了解调度器通过VSync信号调度执行BEGIN_IMPL_FRAME操作的过程。

​		ThreadProxy类的成员函数SetNeedsBeginFrame的实现如下所示：

```c++
void ThreadProxy::SetNeedsBeginFrame(bool enable) {
  ......
  impl().layer_tree_host_impl->SetNeedsBeginFrame(enable);
  ......
}
```

​		ThreadProxy 类的成员函数 SetNeedsBeginFrame 实现是调用负责管理网页的 CC Pending Layer Tree 和 CC Active Layer Tree 的 LayerTreeHostImpl 对象的成员函数 SetNeedsBeginFrame 请求在下一个 VSync 信号到来时获得通知。

