/**
 * @file 睡眠、切换线程、超线程CPU切换线程、线程执行时间.h
 * @author dxp
 * @brief 
 * @version 0.1
 * @date 2020-03-12
 * 
 * @copyright Copyright (c) 2020
 * 
 */

//-------------------睡眠-----------------------------------------
/**
 * @brief 线程可以告诉系统，在一段时间内自己不需要调度了
 * 
 * @param dwMilliseconds 系统设置的时间只是"近似于"所设定的毫秒数。Windows不是实时操作系统，实际时间可能准时、数秒或者几分钟。
    此参数可传入INFINTE，告诉系统永远不要调用该线程
    此参数可传入0，这是告诉系统该线程主动放弃时间片剩余部分
 * @return VOID 
 */
VOID Sleep(DWORD dwMilliseconds);

//-------------------睡眠--------------------------end-----------


//------------------切换到另一个线程-----------------------------
/**
 * @brief 调用这个函数时，系统查看是否存在正急需CPU时间的饥饿线程。
 如果没有，SwitchToThread立即返回。如果存在，SwitchToThread将调度该线程(其优先级可能比SwitchToThread的主调线程低)。
 接下才能可以运行一个时间量，然后系统调度程序恢复正常运行。
    通过这个函数，需要某个资源的线程可以强制一个可能拥有该资源的低优先级的线程放弃资源。
 * 
 * @return BOOL 如果在调用SwitchToThread时没其他线程可以运行，则该函数将返回FALSE；否则，函数将返回一个非零值。
 */
BOOL SwitchToThread();
调用SwitchToThread与调用Sleep类似，传入0ms超时即可。区别在于：
SwitchToThread允许执行低优先级线程
Sleep会立即重新调度主调线程，即使低优先级线程还处于饥饿状态。

//------------------切换到另一个线程-------------end-------------


//------------------线程的执行时间-------------------------------
   许多人在计算代码执行时间的方法是编写如下代码：

ULONGLONG qwStartTime = GetTickCount64()；
ULONGLONG qwElapsedTime = GetTickCount64()-qwStartTime;

   这段代码有一个简单的前提：即代码的执行不会被中断。但是，在抢占式操作系统中，我们不可能知道线程什么时候会获得CPU时间。
当线程失去CPU时间时，为线程执行的各种任务进行计时就更困难了。我们需要的是一个能够返回线程已获得的CPU时间量的函数。值得庆祝的是，
在windows vista之前，就有一个函数能够返回这种信息了，即GetThreadTimes：

/**
 * @brief Get the Thread Times object
 * 
 * @param hThread 
 * @param pftCreationTime 创建时间 用100ns为单位
 * @param pftExitTime 退出时间 用100ns为单位。如果线程仍在运行，退出时间未定义
 * @param pftKernelTime 内核时间 一个用来表示线程执行内核模式下的操作系统代码时所用的时间的绝对值，用100ns为单位
 * @param pftUserTime 用户时间 一个用来表示线程执行应用代码所用时间的绝对值
 * @return BOOL 
 */
BOOL GetThreadTimes(
   HANDLE hThread,
   PFILETIME pftCreationTime,
   PFILETIME pftExitTime,
   PFILETIME pftKernelTime,
   PFILETIME pftUserTime
);

使用这个函数，便可以确定执行一个复杂算法所需的时间，具体代码如下：
__int64 FIleTimeToQuadWord(PFILETIME pft) 
{
	return(Int64ShllMod32(pft->dwHighDateTime, 32)|pft->dwLowDateTime);
}
void PerformLongOperation() 
{
	PFILETIME ftKernelTimeStart, ftKernelTimeEnd;
	PFILETIME ftUserTimeStart, ftUserTimeEnd;
	PFILETIME ftDummy;
	__int64 qwKernelTimeElapsed, qwUserTimeElapsed, qwTotalTimeElapsed;
	// Get starting times. 
   GetThreadTimes(GetCurrentThread(), &ftDummy, &ftDummy, 
   &ftKernelTimeStart, &;ftKernelTimeStart); 
   // Perform complex algorithm here. 
   
   // Get ending time. 
   GetThreadTimes(GetCurrentThread(), &ftDummy, &ftDummy, 
   &ftKernelTimeEnd, &;ftKernelTimeEnd); 
   // Get the elapsed kernel and user times by converting the start and end times from FILETIMEs to quad words, 
   // and then subtract the start times from the end times. 
   qwKernelTimeElapsed = FileTimeToQuadWord(&;ftKernelTimeEnd) - FileTimeToQuadWord(&;ftKernelTimeStart); 

   qwUTimeElapsed = FileTimeToQuadWord(&;ftUserTimeEnd) - FileTimeToQuadWord(&;ftUserTimeStart);

   // Get total time duration by adding the kernel and user times. 
   qwTotalTimeElapsed = qwKernelTimeElapsed + qwUserTimeElapsed; 

   // The total wlapsed time is in qwTotalTimeElapsed. 
}
注意GetProcessTimes，这个函数类似与GetThreadTimes,可用于进程中的所有线程:
/**
 * @brief 适用于进程中的所有线程
 * 
 * @param hProcess 
 * @param pftCreationTime 
 * @param pftExitTime 
 * @param pftKernelTime 
 * @param pftUsertime 
 * @return BOOL 
 */
BOOL GetProcessTimes(
   HANDLE hProcess,
   PFILETIME pftCreationTime,
   PFILETIME pftExitTime,
   PFILETIME pftKernelTime,
   PFILETIME pftUsertime
);