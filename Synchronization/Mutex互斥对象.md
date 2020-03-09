## Mutex互斥对象

概念： 创建互斥量来确保一个线程独占对一个资源的访问。
    互斥量对象包含一个使用计数、线程ID以及一个递归计数。
        线程ID：用来标识当前占用这个互斥量的是系统中的那个线程，
        递归计数：表示这个线程占用该互斥量的次数。
    互斥量可以确保正在访问内存块中的任何线程会独占对内存块的访问权，这样就维护了数据的完整性

API：
HANDLE CreateMutex(
LPSECURITY_ATTRIBUTESlpMutexAttributes,// 安全描述符
BOOL bInitialOwner,// 初始化互斥对象的所有者
LPCTSTR lpName // 指向互斥对象名的指针
);
参数：
bInitialOwner：用来控制互斥量的初始状态，如果传的是FALSE（通常情况）,那么互斥量对象的线程ID和递归计数都将被设置为0，这意味着互斥量为任何线程占用，因此处于触发状态。如果传入的是TRUE，那么对象的线程ID将被设为调用线程的线程ID，递归计数将被设为1，由于线程ID为非0值，因此互斥量处于未触发状态。

假设线程试图等待一个未触发的互斥量对象。在这种情况下，线程通常会进入等待状态。但是，线程会检查想要的互斥量的线程ID与互斥量内部记录的线程ID是否相同。如果一致，那么线程将保持可调度状态--即使该互斥量没被触发。每次线程成功等待了一个互斥量，互斥量对象的递归计数会递增。使递归计数大于1的唯一途径就是利用这个例外


打开互斥对象（OpenMutex）
     OpenMutex函数
功能：打开一个互斥量对象
函数原型:
HANDLE OpenMutex(
DWORD dwDesiredAccess,// 打开方式
BOOL bInheritHandle,// 如希望子进程能够继承句柄，则为TRUE
LPCTSTRlpName// 指向互斥对象名的指针
);
参数：
dwDesiredAccess取值如下：
MUTEX_ALL_ACCESS请求对互斥体的完全访问
MUTEX_MODIFY_STATE允许使用 ReleaseMutex 函数
SYNCHRONIZE 允许互斥体对象同步使用


释放互斥对象（ReleaseMutex）
ReleaseMutex函数
功能：释放互斥量
原型：
HANDLE ReleaseMutex(
HANDLE hMutex
)
备注：这个函数会将对象的递归计数减1,。如果线程成功的等待了互斥量对象不止一次，那么线程必须调用该函数相同次数，才能将递归计数变成0


案例分析：
#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define THREADCOUNT 2
HANDLE g_hMutex = NULL; 
//模拟写入数据库这个公共资源的方法
DWORD WINAPI WriteToDatabase( LPVOID );

void _tmain()
{
	GRS_USEPRINTF();
	HANDLE aThread[THREADCOUNT];
	DWORD ThreadID;
	int i;

	g_hMutex = CreateMutex(	NULL/*安全描述符*/,FALSE/*是否拥有*/,NULL/*对象名字*/);
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	for( i=0; i < THREADCOUNT; i++ )
	{
		aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)WriteToDatabase
			,NULL,CREATE_SUSPENDED,&ThreadID);
		SetThreadAffinityMask(aThread[i],1 << (i % si.dwNumberOfProcessors));
		ResumeThread(aThread[i]);
	}

	WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);//等待所有线程对象退出
	
	for( i=0; i < THREADCOUNT; i++ )
	{
		CloseHandle(aThread[i]);
	}

	CloseHandle(g_hMutex);

	_tsystem(_T("PAUSE"));
}

DWORD WINAPI WriteToDatabase( LPVOID lpParam )//写入数据库
{ 
	GRS_USEPRINTF();
	DWORD dwCount=0, dwWaitResult; 
	while( dwCount < 20 )
	{ 
		dwWaitResult = WaitForSingleObject(g_hMutex,INFINITE);
		switch (dwWaitResult) 
		{
		case WAIT_OBJECT_0: 
			__try 
			{ 
				GRS_PRINTF(_T("Timestamp(%u) Thread 0x%x writing to database...\n")
					,GetTickCount(),GetCurrentThreadId());
				Sleep(1);
				dwCount++;
			} 
			__finally
			{ 
				if (! ReleaseMutex(g_hMutex)) //释放掉 Mutex
				{ 
					// Deal with error.
				} 
			} 
			break; 
		case WAIT_ABANDONED: 
			return FALSE; 
		}
	}
	return TRUE; 
}



 
