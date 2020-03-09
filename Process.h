//---------------------------创建进程-------------------------------------------------------------------------------

//api
/// \param lpApplicationName 子进程要使用得可执行文件名称 可传入完整路径
/// \param lpCommandLine 传给子进程得命令行字符串  类型为LPSTR 传入非const字符串 函数中会修改该字符串 函数返回前会还原 此参数将会传给子进程的pszCmdLine参数
/// \param lpProcessAttributes 子进程安全描述符，
/// \param lpThreadAttributes 子进程的主线程安全描述符
/// \param bInheritHandles 是否继承父进程句柄
/// \param dwCreationFlags See note below
/// \param lpEnvironment 子进程要使用的环境字符串，可传入NULL
/// \param lpCurrentDirectory 设置子进程的当前驱动器和目录，若不为NULL必须带上驱动器
/// \param lpStartupInfo 指向一个STARTUPINFO结构体，设置子进程的属性信息，设置错误会导致子进程崩溃
/// \param lpProcessInformation 指向一个PROCESS_INFORMATION结构 
BOOL CreateProcessA(
  LPCSTR                lpApplicationName,
  LPSTR                 lpCommandLine,
  LPSECURITY_ATTRIBUTES lpProcessAttributes,
  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  BOOL                  bInheritHandles,
  DWORD                 dwCreationFlags,
  LPVOID                lpEnvironment,
  LPCSTR                lpCurrentDirectory,
  LPSTARTUPINFOA        lpStartupInfo,
  LPPROCESS_INFORMATION lpProcessInformation
);
/// \brief 得到当前进程ID
DWORD GetCurrentProcessId();
/// \brief 得到当前线程ID
DWORD GetCurrentProcessId();
/// \brief 根据线程ID获取它的进程ID
DWORD GetProcessIdThread();
//对于ID ，进程与线程公用一个号码池，所以不存在相同的ID
//注意：请勿试图利用ID来操作进程、线程，因为进程、线程一旦释放ID重回号码池
//，再建线程、进程时会被重用，ID就不准确了

//---------------------------终止进程-------------------------------------------------------------------------------
//进程有一下四种方式终止：

//⭐主线程的主函数结束返回（强烈推荐）
//主程序的主函数返回，可以保证一下操作会被正确执行：
//1.该线程创建的任何C++对象都由这些对象的析构函数正确销毁
//2.操作系统将正确释放线程栈使用的内存
//3.系统将进程的退出代码(在进程内核对象中维护)设为朱函数的返回值
//4.系统递减进程内核对象的使用计数

//⭐进程中的一个线程调用ExitProcess函数（要避免这种方式）
//api：
VOID ExitProcess(UINT fuExitcode);
//主函数调用方式：
/*  c++运行库（）{//伪代码
    CreateProcess（...）；
    Process主函数..；
    释放资源；
    ExitProcess（...）；
}
*/
//由上述代码可知当在主函数里调用ExitProcess后C++运行库并不会释放资源！！

//⭐另一个进程中的线程调用TerminateProcess函数（要避免这种方式）
//api：
/// \brief 此函数和Exitprocess有哥明显的区别：任何线程都可以调用TerminateProcess来终止另一个进程或者它自己的进程
/// \param hProcess 指定了要终止的进程句柄
/// \param fuExitcode 进程终止时，其推出的代码的值就是传给fuExitCode参数的值
BOOL TerminateProcess(HANDLE hProcess,UINT fuExitcode);
//只有在无法通过其他方法来强制进程退出时，才应该使用TerminateProcess
//注意：该函数是异步的！

//⭐进程中的所有线程自然死亡（这种情况几乎从来不会发生）





//dwCreationFlags::
/*CREATE_BREAKAWAY_FROM_JOB 0x01000000
The child processes of a process associated with a job are not associated with the job.
If the calling process is not associated with a job, this constant has no effect. If the calling process is associated with a job, the job must set the JOB_OBJECT_LIMIT_BREAKAWAY_OK limit.
CREATE_DEFAULT_ERROR_MODE 0x04000000
The new process does not inherit the error mode of the calling process. Instead, the new process gets the default error mode.
This feature is particularly useful for multithreaded shell applications that run with hard errors disabled.
The default behavior is for the new process to inherit the error mode of the caller. Setting this flag changes that default behavior.
CREATE_NEW_CONSOLE 0x00000010
The new process has a new console, instead of inheriting its parent's console (the default). For more information, see Creation of a Console.
This flag cannot be used with DETACHED_PROCESS.
CREATE_NEW_PROCESS_GROUP 0x00000200
The new process is the root process of a new process group. The process group includes all processes that are descendants of this root process. The process identifier of the new process group is the same as the process identifier, which is returned in the lpProcessInformation parameter. Process groups are used by the GenerateConsoleCtrlEvent function to enable sending a CTRL+BREAK signal to a group of console processes.
If this flag is specified, CTRL+C signals will be disabled for all processes within the new process group.
This flag is ignored if specified with CREATE_NEW_CONSOLE.
CREATE_NO_WINDOW 0x08000000
The process is a console application that is being run without a console window. Therefore, the console handle for the application is not set.
This flag is ignored if the application is not a console application, or if it is used with either CREATE_NEW_CONSOLE or DETACHED_PROCESS.
CREATE_PROTECTED_PROCESS 0040000
The process is to be run as a protected process. The system restricts access to protected processes and the threads of protected processes. For more information on how processes can interact with protected processes, see Process Security and Access Rights.
To activate a protected process, the binary must have a special signature. This signature is provided by Microsoft but not currently available for non-Microsoft binaries. There are currently four protected processes: media foundation, audio engine, Windows error reporting, and system. Components that load into these binaries must also be signed. Multimedia companies can leverage the first two protected processes. For more information, see Overview of the Protected Media Path.
Windows Server 2003 and Windows XP: This value is not supported.
CREATE_PRESERVE_CODE_AUTHZ_LEVEL 2000000
Allows the caller to execute a child process that bypasses the process restrictions that would normally be applied automatically to the process.
CREATE_SECURE_PROCESS 0400000
This flag allows secure processes, that run in the Virtualization-Based Security environment, to launch.
CREATE_SEPARATE_WOW_VDM 0000800
This flag is valid only when starting a 16-bit Windows-based application. If set, the new process runs in a private Virtual DOS Machine (VDM). By default, all 16-bit Windows-based applications run as threads in a single, shared VDM. The advantage of running separately is that a crash only terminates the single VDM; any other programs running in distinct VDMs continue to function normally. Also, 16-bit Windows-based applications that are run in separate VDMs have separate input queues. That means that if one application stops responding momentarily, applications in separate VDMs continue to receive input. The disadvantage of running separately is that it takes significantly more memory to do so. You should use this flag only if the user requests that 16-bit applications should run in their own VDM.
CREATE_SHARED_WOW_VDM 0001000
The flag is valid only when starting a 16-bit Windows-based application. If the DefaultSeparateVDM switch in the Windows section of WIN.INI is TRUE, this flag overrides the switch. The new process is run in the shared Virtual DOS Machine.
CREATE_SUSPENDED 0000004
The primary thread of the new process is created in a suspended state, and does not run until the ResumeThread function is called.
CREATE_UNICODE_ENVIRONMENT 0000400
If this flag is set, the environment block pointed to by lpEnvironment uses Unicode characters. Otherwise, the environment block uses ANSI characters.
DEBUG_ONLY_THIS_PROCESS 0000002
The calling thread starts and debugs the new process. It can receive all related debug events using the WaitForDebugEvent function.
DEBUG_PROCESS 0000001
The calling thread starts and debugs the new process and all child processes created by the new process. It can receive all related debug events using the WaitForDebugEvent function.
A process that uses DEBUG_PROCESS becomes the root of a debugging chain. This continues until another process in the chain is created with DEBUG_PROCESS.
If this flag is combined with DEBUG_ONLY_THIS_PROCESS, the caller debugs only the new process, not any child processes.
DETACHED_PROCESS 0000008
For console processes, the new process does not inherit its parent's console (the default). The new process can call the AllocConsole function at a later time to create a console. For more information, see Creation of a Console.
This value cannot be used with CREATE_NEW_CONSOLE.
EXTENDED_STARTUPINFO_PRESENT 0080000
The process is created with extended startup information; the lpStartupInfo parameter specifies a STARTUPINFOEX structure.
Windows Server 2003 and Windows XP: This value is not supported.
INHERIT_PARENT_AFFINITY 0010000
The process inherits its parent's affinity. If the parent process has threads in more than one processor group, the new process inherits the group-relative affinity of an arbitrary group in use by the parent.
Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP: This value is not supported.
*/