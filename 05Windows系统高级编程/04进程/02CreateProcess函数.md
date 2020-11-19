## CreateProcess

​		我们用 CreateProcess 函数来创建一个进程，如下所示:

```C
BOOL CreateProcess(
    PCTSTR pszApplicationName,
    PTSTR pszCommandLine,
    PSECURITY_ATTRIBUTES psaProcess,
    PSEOJRITY_ATTRIBUTES psaThread,
    BOOL blnheritHandles,
    DWORD fdwCreate,
    PVOID pvEnvironment,
    PCTSTR pszCurDir,
    PSTARTUPINFO psiStartlnfo,
    PPROCESS_INFORMATION ppiProcInfo
);
```

​		一个线程调用**CreateProcess**时，系统将创建一个进程内核对象，其初始使用计数为1。进程内核对象不是进程本身，而是操作系统叫来管理这个进程的一个小型数据结构——可以把进程内核对象想象成由进程统计信息构成的一个小型数据结构。然后，系统为新进程创建一个虚拟地址空间，并将可执行文件 ( 和所有必要的 DLL ) 的代码及数据加载到进程的地址空间。

​		然后，系统为新进程的主线程创建一个线程内核对象 ( 其使用计数为 1 ) 。和进程内核对象一 样，线程内核对象也是一个小型数据结构，操作系统用它来管理这个线程。这个主线程一开始就会执行C/C++运行时的启动例程，它是由链接器设为应用程序入口的，最终会调用应用程序 WinMain ，wWinMain , main 或 wmain 函数。如果系统成功创建了新进程和主线程，**CreateProcess** 将返回 TRUE 。

——————————————————————————————————————————————————说明	**CreateProcess** 在进程完全初始化好之前就返回**TRUE**。这意味着操作系统加载程序 ( loader ) 尚未尝试定位所有必要的 DLL 。如果一个 DLL 找不到或者不能正确初始化，进程就会终止，因为**CreateProcess**返回TRUE，所以父进程不会注意到任何初始化问题。——————————————————————————————————————————————————

OK,前面只是泛泛而谈，下面将分小节逐一讨论**CreateProcess**的参数。





### 1. pszApplicationName 和 pszCommandLine 参数

​		**pszApplicationName**和**pszCommandLine**参数分别指定新进程要使用的可执行文件的名称，以及要传给新进程的命令行字符串。先来谈谈**pszCommandLine**参数。

​		注意，在函数原型中，**pszCommandLine**参数的类型为**PTSTR**。这意味着**CreateProces**期望我们传入的是一个非 “ 常量字符串 ” 的地址。在内部，**CreateProcess**实际上会修改我们传给它的命令行字符串。但在**CreateProcess**返回之前，它会将这个字符串还原为原来的形式。

​		这是很重要的，因为如果命令行字符串包含在文件映像的只读部分，就会引起访问违规 ( 违例 ) 。例如，以下代码就会导致访问违规，因为 Microsoft 的 C/C++ 编译器把 ” NOTEPAD " 字符串放在只读内存中：

```c
STARTUPINFO si = { sizeof(si) };
PROCESS_INFORMATION pi;
CreateProcess(NULL, TEXT("NOTEPAD"), NULL, NULL,
	FALSE, 0, NULL, NULL, &si, &pi);
```

​		**CreateProcess**试图修改字符串时，会引起一个访问违规 ( Microsoft C/C++ 编译器的早期版本把字符串放在可读/写内存中。所以对 **CreateProcess** 函数的调用不会引起访问违规 ) 。

​		解决这个问题的最佳方式是在调用**CreateProcess**之前,把常量字符串复制到一个临时缓冲区，如下所示：

```c
STARTUPINFO si = { sizeof(si) };
PROCESS_INFORMATION pi;
TCHAR szCommandLine = TEXT("NOTEPAD");

CreateProcess(NULL, szCommandLine, NULL, NULL,
	FALSE, 0, NULL, NULL, &si, &pi);
```

​		我们可能还要注意对 Microsoft C++ 的 **/Gf** 和 **/GF** 编译器开关的使用，它们可以消除重复的字符串，并判断是否将那些字符串放在一个只读的区域。（还要注意 /ZI 开关，它允许使用 Visual Studio的 “ 编辑并继续 ”（Edit & Continue)调试功能，它包含 /GF 开关的功能。）最佳 做法是使用 /GF 编译器开关和一个临时缓冲区。目前，Microsoft最应该做的一件事情就是修正**CreateProcess** , 使它自己能创建字符串的一个临时副本，从而使我们得到解放。 Windows未来的版本或许会对此进行修复。

​		顺便提一下，如果在 Windows Vista 中调用**CreateProcess**函数的ANSI版本，是不会发生访问违规的，因为它会为命令行字符串创建一个临时副本(详情参见第2章)。

​		可以使用**pszCommandLine**参数来指定一个完整的命令行，供**CreateProcess**用于创建新进程。当**CreateProcess**解析**pszCommandLine**字符串时，它会检查字符串中的第一个标记(token),并假定此标记是我们想运行的可执行文件的名称。如果可执行文件的名称没有扩展名，就会默认是.exe扩展名。**CreateProcess**还会按照以下顺序搜索可执行文件。

1. 主调进程.EXE文件所在的目录。

2. 主调进程的当前目录。

3. Windows 系统目隶，即 **GetSystemDirectory** 返回的 System32 子文件夹。

4. Windows 目录。

5. PATH环境变量中列出的目录。

​        当然，假如文件名包含一个完整路径，系统就会利用这个完整路径来查找可执行文件，而不会搜索目录。如果系统找到了可执行文件，就创建一个新进程，并将可执行文件的代码和数据映射到新进程的地址空间。然后，系统调用由链接器设为应用程序入口点的 C/C++ 运行时启动例程。如前所述，C/C++ 运行时启动例程会检査进程的命令行，将可执行文件名之后的第一个实参的地址传给**（w)WinMain** 的**pszCmdLine**参数。

​		只要**pszAppHcationName**参数为**NULL** ( 99%以上的情况都是如此 ) ，就会发生上述情况。 但是，也可以不在**pszApplicationName**中传递**NULL**，而是传递一个字符串地址，并在字符串中包含想要运行的可执行文件的名称。但在这种情况下，必须指定文件扩展名，系统不会自动假定文件名有一个**.exe**扩展名。**CreateProcess**假定文件位于当前目录，除非文件名前有一个路径。如果没有在当前目录中找到文件，**CreateProcess**不会在其他任何目录査找文件——调用会以失败而告终。

​		然而，即使在**pszApplicationName**参数中指定了文件名，**CreateProcess**也会将 **pszCommandLine**参数中的内容作为新进程的命令行传给它。例如，假设像下面这样调用 **CreateProcess**：

```c
// Make sure that the path is in a read/write section of memory.
TCHAR szPathf[] = TEXT("WORDPAD README.TXT");

// Spawn the new process.
CreateProcess (TEXT("C: \\WINDOWS\\SYSTEM32\\NOTEPAD.EXE") ,szPath, .. .);
```

​		系统会调用记事本应用程序，但记事本应用程序的命令行是WORDPAD README.TXT。 虽然这看起来有点儿怪，但**CreateProcess**的工作机制就是这样的。之所以让我们能为 **CreateProcess** 添加 **pszApplicationName** 参数，实际是为了支持 Windows 的 POSIX 子系统。





