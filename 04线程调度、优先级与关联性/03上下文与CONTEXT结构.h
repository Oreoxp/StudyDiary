/**
 * @file 03上下文与CONTEXT结构.h
 * @author dxp
 * @brief 
 * @version 0.1
 * @date 2020-03-17
 * 
 * @copyright Copyright (c) 2020
 * 
 */

typedef struct _CONTEXT {
    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.

    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.

    // The context record is never used as an OUT only parameter.
    //

    DWORD ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;

    DWORD   Dr6;
    DWORD   Dr7;
 
    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //
    FLOATING_SAVE_AREA FloatSave;
    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //
    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;
    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //
    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;
    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //
    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;              // MUST BE SANITIZED
    DWORD   EFlags;             // MUST BE SANITIZED
    DWORD   Esp;
    DWORD   SegSs;
    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //
    BYTE    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} CONTEXT;
/*
CONTEXT结构可以分成若干个部分。CONTEXT_CONTROL包含CPU的控制寄存器，
比如指令指针、堆栈指针、标志和函数返回地址（与x86处理器不同，
Alpya CPU在调用函数时，将该函数的返回地址放入一个寄存器中）。
CONTEXT_INTEGER用于标识CPU的整数寄存器。
CONTEXT_FLOATING_POINT用于标识CPU的浮点寄存器。
CONTEXT_SEGMENTS用于标识CPU的段寄存器（仅为x 8 6处理器）。
CONTEXT_DEBUG_REGISTER用于标识CPU的调试寄存器（仅为x86处理器）。
CONTEXT_EXTENDED_REGISTERS用于标识CPU的扩展寄存器（仅为x 8 6处理器）。
*/

/**
 * @brief 若要调用该函数，只需指定一个CONTEXT结构，对某些标志（该结构的ContextFlags成员）进行初始化，指明想要收回哪些寄存器，并将该结构的地址传递给 GetThreadConText。然后该函数将数据填入你要求的成员。
   在调用GetThreadConText函数之前，应该调用SuspendThread，否则，线程可能被调度，而且线程的环境可能与你收回的不同。一个线程实际上有两个环境。一个是用户方式，一个是内核方式。
GetThreadConText只能返回线程的用户方式环境。如果调用SuspendThread来停止线程的运行，但是该线程目前正在用内核方式运行，那么，即使 SuspendThread实际上尚未暂停该线程的运行，它的用户方式仍然处于稳定状态。
线程在恢复用户方式之前，它无法执行更多的用户方式代码，因此可以放心地将线程视为处于暂停状态， GetThreadConText函数将能正常运行。
 * 
 * @param hThread 
 * @param pContext 
 * @return BOOL 
 */
BOOL GetThreadConText(
    HANDLE hThread,
    PCONTEXT pContext
);

