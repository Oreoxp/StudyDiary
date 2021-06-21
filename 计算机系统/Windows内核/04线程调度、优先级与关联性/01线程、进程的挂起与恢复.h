/**
 * @file 线程、进程的挂起与恢复.h
 * @author dxp
 * @brief 
 * @version 0.1
 * @date 2020-03-12
 * 
 * @copyright Copyright (c) 2020
 * 
 */


 //----------------------------挂起线程-----------------------------
在线程内核对象中有一个值表示线程的挂起计数。调用CreateProcess或者CreateThread时，
系统将创建线程的内核对象，并把挂起计数初始化为1.这样就不会给这个线程调度CPU了。这正是我们所以希望的，
因为线程初始化需要时间，我们当然不想在线程准备好之前就开始执行它。


/**
 * @brief 恢复线程
 * 
 * @param hThread 传入CreateThread返回的句柄，或者传CreateProcess的ppiProcInfo参数所指向的结构中的线程句柄
 * @return DWORD 如果成功，返回一个挂起计数；否则返回0xFFFFFFFF。
 */
DWORD ResumeThread(HADNLE hThread);

/**
 * @brief 挂起线程
 * 任何线程都可以调用这个函数挂起另一个线程（只要有线程的句柄）
 * @param hThread 传入CreateThread返回的句柄，或者传CreateProcess的ppiProcInfo参数所指向的结构中的线程句柄
 * @return DWORD 如果成功，返回一个挂起计数；否则返回0xFFFFFFFF。
 */注意：1.SuspendThread是异步的哦
        2.一个线程最多可以挂起MAXIMUM_SUSPEND_COUNT(WinNT.h中定义为127)
DWORD SuspendThread(HADNLE hThread);
实际开发中，应用程序在调用SuspendThread时必须消息，因为试图挂起一个线程时，我们不知道线程在做什么。
例如，如果线程正在分配堆中的内存，线程将锁定堆。其他所有线程都不能访问堆，直到该线程恢复。
只有在确切知道目标线程是哪个（或者他在做什么），而且采取完备措施避免出现因挂起线程二引起的问题或者死锁的时候，
调用SuspendThread才是安全的。
//----------------------------挂起线程--------------end-----------


//----------------------------挂起进程-----------------------------
其实，windows中不存在挂起和恢复进程的概念，因为系统从不会给进程调度CPU时间，但是，在一个特殊情况下，即调试器处理
WaitForDebugEvent返回的调试事件时。Windows将冻结被调试进程中的所有线程，直至调试器调用ContinueDebugEvent。

虽然我们没有十全十美的SuspendProcess函数，但是我们可以创建一个适用于许多情况的版本。
这是我实现的SuspendProcess函数:
//使用请谨慎
VOID SuspendProcess(DWORD dwProcessID, BOOL fSuspend) {
    // Get the list of threads in the system.
    HANDLE hSnapshot = CreateToolhelp32Snapshot(
    TH32CS_SNAPTHREAD, dwProcessID);

    if (hSnapshot != INVALID_HANDLE_VALUE) {

    // Walk the list of threads.
    THREADENTRY32 te = { sizeof(te) };
    BOOL fOk = Thread32First(hSnapshot, &te);
    for (; fOk; fOk = Thread32Next(hSnapshot, &te)) {

        // Is this thread in the desired process?
        if (te.th32OwnerProcessID == dwProcessID) {

            // Attempt to convert the thread ID into a handle.
            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME,
            FALSE, te.th32ThreadID);
            if (hThread != NULL) {

                // Suspend or resume the thread.
                if (fSuspend)
                SuspendThread(hThread);
                else
                ResumeThread(hThread);
            }
            CloseHandle(hThread);
        }
    }
    CloseHandle(hSnapshot);
    }
}

//----------------------------挂起进程--------------end-----------