#### Slim读/写锁

​		**SRWLock**的目的和关键段相同：对一个资源进行保护，不让其他线程访问它。但是，与关键段不同的是**SRWLock**允许我们区分哪些想要读取资源的值得线程（读取者线程）和想要更新资源的值的线程（写入者线程）。让所有读者线程同一时间访问共享资源是可能的，因为读取资源并不存在破坏数据的风险。只有当写入者线程想要对资源进行更新的时候才需要进行同步。

​		这种情况下，写入者线程应该独占对资源的访问权：任何其他线程，包括读者线程都不允许访问资源。这就是**SRWLock**提供的全部功能，我们可以在代码中以一种非常清晰的方式来使用它。

​		首先，我们需要分配一个**SRWLOCK**结构并用**InitalSRWLock**函数对它进行初始化：

```c++
VOID InitalSRWLock(PSRWLOCK SRWLock);
```

​		**SRWLOCK**只包含一个指向其他东西的指针。但是指向的东西未公开，<u>因此我们不能编写代码来访问它</u>。	

​		一旦**SRWLOCK**的初始化完成之后，写入者线程就可以调用**AcquireSRWLockExclusive**，将**SRWLOCK**对象的地址作为参数传入，以尝试获取对被保护的资源的独占访问权。

```c++
VOID AcquireSRWLockExclusive(PSRWLOCK SRWLock);
```

​		完成对资源的感更新后，应该调用**ReleaseSRWLockExclusive**，并将**SRWLOCK**对象的地址作为参数传入，这样就<u>解除了对资源的锁定</u>。

```c++
VOID ReleaseSRWLockExclusive(PSRWLOCK SRWLock);
```

​		对读取者线程来说，同样需要两个步骤，但调用的是下面函数：

```c++
VOID AcquireSRWLockShared(PSRWLOCK SRWLock);
VOID ReleaseSRWLockShared(PSRWLOCK SRWLock);
```

​		仅此而已，不需要删除或销毁**SRWLOCK**的汉化，系统会自动执行清理工作。

​		与关键段相比，SRWLock缺乏下面两个特性：


•不存在**TryEnter**(Shared/Exclusive)**SRWLock** 之类的函数：如果锁已经被占用，那么调用**AcquireSRWLock**(Shared/Exclusive) 会阻塞调用线程。

•不能递归地调用**SRWLOCK**。也就是说，一个线程不能为了多次写入资源而多次锁定资源，然后再多次调用**ReleaseSRWLock*** 来释放对资源的锁定。

​    总结，如果希望在应用程序中得到最佳性能，那么首先应该尝试不要共享数据，然后依次使用volatile读取，volatile写入，**Interlocked API**，**SRWLock**以及关键段。当且仅当所有这些都不能满足要求的时候，再使用内核对象。因为每次等待和释放内核对象都需要在用户模式和内核模式之间切换，这种切换的CPU开销非常大。

### 条件变量

​		我们已经看到，当想让写入者线程和读取者线程以独占的方式或共享的方式访问一个资源的时候，可以使用**SRWLock**。在这些情况下，如果读取者线程 没有数据可以读取，那么它应该将锁释放并等待，直到写入者线程产生了新的数据为止。如果用来接收写入者线程产生的数据结构已满，那么写入者同样应该释放 **SRWLock**并进入睡眠状态，直到读取这线程把数据结构清空为止。

​       我们希望线程以原子的方式把锁释放并将自己阻塞，直到某一个条件成立为止。要实现这样的线程同步是比较复杂的。Windows通过 **SleepConditionVariableCS(critical section)** 或者**SleepConditionVariableSRW** 函数，提供了一种 条件变量 帮助我们完成这项工作。

​       当线程检测到相应的条件满足的时候（比如，有数据供读取者使用），它会调用 **WakeConditionVariable** 或  **WakeAllConditionVariable**,这样阻塞在Sleep*函数中的线程就会被唤醒。

​      先来看一段代码：

1. ```c++
   // 条件变量.cpp : 定义控制台应用程序的入口点。  
   
   1. //  
   2.   
   3. \#include "stdafx.h"  
   4. \#include <windows.h>  
   5. \#include <iostream>  
   6. \#include <vector>  
   7. \#include <process.h>  
   8. using namespace std;  
   9.   
   10. DWORD WINAPI ThreadProduce(PVOID pvParam);  
   11. DWORD WINAPI ThreadUser1(PVOID pvParam);  
   12. DWORD WINAPI ThreadUser2(PVOID pvParam);  
   13. vector<int> ivec;  
   14. SRWLOCK g_lock;  
   15. SRWLOCK g_lock2;  
   16. CONDITION_VARIABLE g_ConditionVar;  
   17.   
   18. int _tmain(int argc, _TCHAR* argv[])  
   19. {  
   20. ​    InitializeSRWLock(&g_lock);//初始化锁  
   21.   
   22. ​    //创建生产者  
   23. ​    HANDLE hThread1 = (HANDLE)_beginthreadex(NULL,0,(unsigned int(_stdcall *)(void*))ThreadProduce,NULL,0,0);  
   24.   
   25. ​    //创建读取者线程  
   26. ​    HANDLE hThread2 = (HANDLE)_beginthreadex(NULL,0,(unsigned int(_stdcall *)(void*))ThreadUser1,NULL,0,0);  
   27. ​    HANDLE hThread3 = (HANDLE)_beginthreadex(NULL,0,(unsigned int(_stdcall *)(void*))ThreadUser2,NULL,0,0);  
   28. ​    CloseHandle(hThread1);  
   29. ​    CloseHandle(hThread2);  
   30. ​    CloseHandle(hThread3);  
   31.   
   32. ​    system("pause");  
   33. ​    return 0;  
   34. }  
   35.   
   36. DWORD WINAPI ThreadProduce(PVOID pvParam)  {  
   37. ​    for(int i=0; i<10; ++i)  
   38. ​    {  
   39. ​        AcquireSRWLockExclusive(&g_lock);  //获得SRW锁  
   40.   
   41. ​        ivec.push_back(i);  
   42.   
   43. ​        ReleaseSRWLockExclusive(&g_lock);  //释放SRW锁  
   44.   
   45. ​        WakeConditionVariable(&g_ConditionVar);  //因为，每次执行push_back 后，容器里就会必定至少有一个元素（生产者  
   46. ​                                                 //生产出东西了）这时候阻塞在Sleep*里的线程被唤醒 （读取者sleep的线程）。  
   47. ​        Sleep(1000);//停一下，让读取者先读  如果不等待，可能会导致vector empty before pop错误
   48. ​    }  
   49. ​    return 0;  
   50. }  
   51. DWORD WINAPI ThreadUser1(PVOID pvParam)  
   52. {  
   53. ​    while(1)  
   54. ​    {  
   55. ​        AcquireSRWLockExclusive(&g_lock);  
   56. ​        while(ivec.empty())  
   57. ​        { 
   58. ​            cout<<"等待写入"<<endl;  
   59. ​    //如果容器是空的，也就是没有内容可以读，那么让线程进入睡眠状态，一直到调用WakeConditionAllVariable(&g_ConditionVar);  
   60. ​            SleepConditionVariableSRW(&g_ConditionVar,&g_lock,INFINITE,CONDITION_VARIABLE_LOCKMODE_SHARED);  
   61. ​        }  
   62. ​        cout<<"线程1："<<ivec.back()<<endl;  
   63. ​        ivec.pop_back();  
   64. ​        ReleaseSRWLockExclusive(&g_lock);  
   65. ​    }  
   66. ​    return 0;  
   67. }  
   68. DWORD WINAPI ThreadUser2(PVOID pvParam)  
   69. {  
   70. ​    while(1)  
   71. ​    {  
   72. ​        AcquireSRWLockExclusive(&g_lock);  
   73. ​        while(ivec.empty())  
   74. ​        {  
   75. ​            cout<<"等待写入"<<endl;  
   76. ​      SleepConditionVariableSRW(&g_ConditionVar,&g_lock,INFINITE,CONDITION_VARIABLE_LOCKMODE_SHARED);  
   77. ​        }  
   78. ​        cout<<"线程2："<<ivec.back()<<endl;  
   79. ​        ivec.pop_back();  
   80. ​        ReleaseSRWLockExclusive(&g_lock);  
   81. ​    }  
   82. ​    return 0;  
   83. }  
   ```

   


分析一下这段代码，其实很简单，具体可以从代码注释中看。之前，写这段代码的时候犯过几个错误：

（1）**AcquireSRWLockExclusive**()  **ReleaseSRWLockExclusive**()   和 **AccquireSRWLockShare**() **ReleaseSRWLockShare**() 两对函数之间的区别

​         前者获得的对保护资源的 独占 访问权 而后者获得是 保护资源的 共享访问权，因为虽然 代码里的读线程，在读取数据的同时，他也**pop_back**()了容器里的内容，也就是可以看作是“写”，因为，我们必须获得的是 独占 访问权。

（2）**WakeCoditionVariabel**() 和 **WakeAllconditionVariable**() 的区别：

​         当调用 前者的时候，会使一个在 **SleepConditionVariable***函数中等待同一个条件变量被触发的线程得到锁并返回。当这个线程释放同一个锁的时候，不会唤醒其他正在等待同一个条件变量的线程

​        当调用后者的时候，会使一个或几个在**SleepConditionVariable***函数中等待这个条件变量触发的线程达到对资源的访问权并返回。

（3）

```c++
BOOL WINAPI SleepConditionVariableSRW(
  __in_out      PCONDITION_VARIABLE ConditionVariable,  //线程休眠相关的条件变量
  __in_out      PSRWLOCK SRWLock,                       //指向一个SRWLock的指针
  __in          DWORD dwMilliseconds,　　　　　　　　　　　　//希望等待的时间，可以为INFINITE
  __in          ULONG Flags
);
```

 该函数一原子操作的方式执行了两个操作：

1.释放**SRWLock**指向的锁；

2.把线程休眠。

 如果参数Flags是 **CONDITION_VARIABLE_LOCKMODE_SHARED**,那么在同一时刻可以允许多个读取者线程得到锁