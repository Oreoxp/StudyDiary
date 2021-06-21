/**
 * @file 04进、线程优先级.h
 * @author DXP
 * @brief 
 * @version 0.1
 * @date 2020-03-29
 * 
 * @copyright Copyright (c) 2020
 * 
 */


一、如何给进程指派优先级：调用CreateProcess时，fdwCreate参数传入需要的优先级

进程优先级：
优先级类	            标识符
实时	                REALTIME_PRIORITY_CLASS
高	                    HIGH_PRIORITY_CLASS
高于默认	            ABOVE_NORMAL_PRIORITY_CLASS
默认	                NROMAL_PRIORITY_CLASS
低于默认	            BELOW_NORMAL_PRIORITY_CLASS
空闲	                IDLE_PRIORITY_CLASS


进程可以通过SetPriorityClass来改变自己的优先级:

/**
 * @brief 进程设置自己的优先级
 * 
 * @param hProcess 进程句柄,需要有句柄权限
 * @param fdwPrioority 为上述表中标志符
 * @return BOOL 
 */
BOOL SetPriorityClass(
    HANDLE hProcess,
    DWORD fdwPrioority
);

/**
 * @brief 获取进程的优先级
 * 
 * @param hProcess 进程句柄 
 * @return DWORD 上述表中之一
 */
DWORD GetPriorityClass(
    HANDLE hProcess,
);


二、当线程创建时，优先级总是设置为normal，CreateThread函数没有设置新线程优先级的办法


线程的相对优先级有7级，分别是：
相对优先级      	    常量标识符
时间关键	            THREAD_PRIORITY_TIME_CRITICAL
最高	                THREAD_PRIORITY_HIGHEST
高于默认	            THREAD_PRIORITY_ABOVE_NORMAL
默认	                THREAD_PRIORITY_NORMAL
低于默认	            THREAD_PRIORITY_BELOW_NORMAL
最低	                THREAD_PRIORITY_LOWEST
空闲	                THREAD_PRIORITY_IDLE

线程真正优先级与上面进程优先级、线程相对优先级的对应关系： 进程优先级和线程优先级之间的映射表:

线程相对优先级	空闲	低于默认	  默认	    高于默认	    高	        实时
Time-critical	15	    15	        15	        15	        15      	31
Highest	        6	    8	        10	        12	        15      	26
Above normal	5	    7	        9	        11	        14      	25
Normal	        4	    6	        8	        10      	13      	24
Below normal	3	    5	        7	        9	        12      	23
Lowest	        2	    4	        6	        8	        11      	22
Idle	        1	    1	        1	        1	        1       	16

tips:线程优先级值没有为0的。这是因为0优先级保留给页面清零线程了。对于17,18,19,20,21,27,28,29,30.如果编写一个以内核方式运行的设备驱动程序,可以获得这些优先级的等级,而用户方式的应用程序则不能


进程可以通过SetPriorityClass来改变自己的优先级:

/**
 * @brief 线程设置自己的优先级
 * 
 * @param hThread 线程句柄,需要有句柄权限
 * @param fdwPrioority 为上述表中标志符
 * @return BOOL 
 */注意：在新建线程时，可创建挂起线程，设置优先级后在启动线程
BOOL SetThreadPriority(
    HANDLE hThread,
    DWORD fdwPrioority
);

/**
 * @brief 获取线程的优先级
 * 
 * @param hProcess 线程句柄 
 * @return DWORD 上述表中之一
 */
DWORD GetThreadPriority(
    HANDLE hProcess,
);


三、动态提升线程优先级

线程基本优先级(base priority level)：线程优先级+所属进程优先级。偶尔，系统也会提升一个线程的优先级——通常是为了响应某种I/O事件比如窗口消息或者磁盘读取。

【例】：high优先级进程中的一个线程优先级为normal的线程，其基本优先级为13.如果用户敲了一个键，系统会在线程的队列中放入一个 WM_KEYDOWN 消息。因为有消息出现在线程的队列中，
线程就成为可调度的了。而且键盘设备驱动程序使系统临时提升线程的优先级。因此线程的优先级可能会提升2，从而使当前的优先级达到15.

线程在优先级15时分了一个时间片。在该时间片结束后，系统将线程的优先级值减一，所以下一个时间片线程的优先级为14、线程的第三个时间片以优先级13只想。以后的时间片将保持在13，及
线程的基本优先级。

注：线程的优先级不会低于线程的基本优先级；不同的设备驱动程序决定优先级提升多少；系统只提升优先级值在1~15的线程，此范围称为动态优先级范围(dynamic priority range)；
线程不会把优先级提升到高于15（防止影响操作系统）。


API:
可以选择是否对开启优先级系统动态提升：
/**
 * @brief Set the Process Priority Boost object
 * 
 * @param hProcess 进程句柄
 * @param bDisablePriorityBoost 是否允许系统提升该进程优先级
 * @return BOOL 
 */
BOOL SetProcessPriorityBoost(
    HANDLE hProcess,
    BOOL bDisablePriorityBoost
);
/**
 * @brief Set the Thread Priority Boost object
 * 
 * @param hThread 线程句柄
 * @param bDisablePriorityBoost 是否允许系统提升该线程优先级
 * @return BOOL 
 */
BOOL SetThreadPriorityBoost(
    HANDLE hThread,
    BOOL bDisablePriorityBoost
);

BOOL GetProcessPriorityBoost(
    HANDLE hProcess,
    BOOL bDisablePriorityBoost//输入bool值带回
);

BOOL GetThreadPriorityBoost(
    HANDLE hThread,
    BOOL bDisablePriorityBoost//输入bool值带回
);

四、为前台进程微调调度程序
如果用户需要使用某个进程的窗口，这个进程就称为前台进程（foreground process），而所有其他的线程称为后台进程（background process）。
Windows会为前台进程中的线程微调调度算法。系统给前台进程的线程分配比一般情况下更多的时间片。
这种微调只在前台进程是normal优先级时才进行，如果处于其他优先级，则不会进行微调。

五、调度I/O请求优先级

低优先级的线程长时间I/O请求可能会挂起高优先级的线程。故从Windows Vista开始，线程可以在进行I/O请求时设置优先级了。

我们可以通过调研SetThreadPriority并传入THREAD_MODE_BACKGROUND_BEGIN来告诉Windows，线程应该发送低优先级的I/O请求；
我们可以通过调研SetThreadPriority并传入THREAD_MODE_BACKGROUND_END  来告诉Windows，线程应该发送normal优先级的I/O请求。

我们可以通过调研SetPriorityClass并传入PROCESS_MODE_BACKGROUND_BEGIN来告诉Windows，该进程下的所有线程应该发送低优先级的I/O请求；
我们可以通过调研SetPriorityClass并传入PROCESS_MODE_BACKGROUND_END  来告诉Windows，该进程下的所有线程应该发送normal优先级的I/O请求。

