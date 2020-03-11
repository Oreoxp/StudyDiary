/**
 * @file Thread.h
 * @author dxp
 * @brief WindowsThread
 * @version 0.1
 * @date 2020-03-10
 * 
 * @copyright Copyright (c) 2020
 * 
 */
 /**********************************
 一些概念：
 1.进程从来不执行任何东西，他只是一个线程得容器
 2.同一个进程中，所有线程共享同一地址空间
 3.这些线程共享内核对象句柄，因为句柄表是针对每个进程的而不是每个线程。
 **********************************/

/**
 * @brief Create a Thread object
 * 新线程在与负责创建的哪个线程在相同的进程上下文中运行。
 因此，新线程可以访问进程内核对象的所有句柄、进程中的所有内存以及同一个进程中的其他所有线程的栈，
 因此，同一进程下的多个线程很容易互相通信。
 * @param psa 安全描述符，可设置是否继承该线程句柄
 * @param cbStackSize 指定线程可以为其线程栈使用多少地址空间
 * @param pFNStartAddr 指定希望新线程执行的线程函数的地址
 * @param pvParam 执行线程函数的参数。注意传入参数的生命周期哦
 * @param dwCreationFlags 指定额外的编制来控制线程的创建。
       0：线程创建后立即调度、 
       CREATE_SUPENDED:创建并初始化线程后将暂停线程的运行。
 * @param pdwThreadId 存储系统分配给新线程的id.可传入NULL，表示我们对ID没兴趣
 * @return HANDLE 
 */注意：请使用C/C++库函数_beginthreadex创建线程

 HANDLE CreateThread(
  PSECURITY_ATTRIBUTES   psa,
  DWORD                  cbStackSize,
  PTHREAD_START_ROUTINE  pFNStartAddr,
  LPVOID                 pvParam,
  DWORD                  dwCreationFlags,
  PDWORD                 pdwThreadId
);


//-------------------终止运行线程的四种方法-------------
//⭐线程函数返回（强烈推荐）
让线程函数返回，可以确保以下正确的应用程序清理工作都得以执行。
1.线程函数中创建的所有C++对象都通过其析构函数被正确销毁。
2.操作系统正确释放线程栈使用的内存。
3.操作系统把线程的退出代码（在线程的内核对象中维护）设为线程函数的返回值。
4.系统递减烧线程的内核对象的使用计数

//⭐线程通过调用ExitThread函数"杀死"自己（要避免使用这种方法）
为了强迫线程终止运行，可以让他调用ExitThread：
/**
 * @brief 该函数将终止线程的运行，并导致操作系统清理该线程使用的所有操作系统资源。
 * 但是，你的C/C++资源（如c++类对象不会被销毁）
 * @param dwExitCode 
 * @return VOID 
 */
VOID ExitThread(DWORD dwExitCode);
//⭐同一个进程或另一个进程中的线程调用TerminateThread函数（要避免使用这种方法）
调用TerminateThread函数也可以杀死一个线程
/**
 * @brief 可以杀死任何线程
 * 
 * @param hThread 要终止的哪个线程的句柄
 * @param dwExitCode 指定退出码
 * @return BOOL 返回是否成功。注意：该函数是异步的哦
 */
BOOL TerminateThread(
    HADNLE hThread,
    DWORD dwExitCode
);
//⭐包含线程的进程终止运行（这种方法避免使用）
//-------------------终止运行线程的四种方法------end-------


//-------------------线程终止运行时-------------begin-----
线程终止运行时，会发送下面这些事情
♥  线程拥有的所有用户对象句柄会被释放。在windows中，大多数对象都是由包含了“创建这些对象的线程”的进程拥有的。
但是一个线程有两个用户对象：窗口（window）和钩子（hook）。一个线程终止运行时，系统会自动销毁由线程创建或安装的任何窗口，
并写在由线程创建或安装的任何挂钩。其他对象只有在拥有线程的进程终止时才被销毁。
♥  线程的退出代码从STILL_ACTIVE变成传给ExitThread或TerminateThread的代码。
♥  线程内核对象的状态变成触发状态。
♥  如果线程是进程中的最后一个活动线程，系统认为进程也终止了。
♥  线程内核对象的使用计数递减1.