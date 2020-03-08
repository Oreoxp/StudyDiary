## 关键代码段

概念：控制多线程访问同一全局资源，关键代码段本质是一种互相排斥的同步方法


API：

CRITICAL_SECTION结构体：这是一个未公开内部信息的结构体，通过这个结构，我们就可以使用API对想要独占的代码进行操作。
使用该结构的条件：
    1、所有想要访问资源的线程，必须知道用来保护资源的CRITICAL_SECTION结构的地址。我们通过传入该结构体的地址，进行操作。
    2、任何线程在访问被保护的资源之前，都必须对这个结构体进行初始化。


功能：初始化临界区
参数：psc指向CRITICAL_SECTION结构体的地址
VOID InitializeCriticalSection(PCRITICAL_SECTION pcs);

功能：重置结构体中的成员变量（而不是删除它）。注意：如果删除的话，别的线程还在使用一个关键段，就会产生不可预料的结果。
参数：psc指向CRITICAL_SECTION结构体的地址
VOID DeleteCriticalSection(PCRITICAL_SECTION pcs);


功能：EnterCriticalSection会检查结构中的成员变量，这些变量表示是否有线程正在进行访问。以及哪个线程正在访问。如果没有线程访问，则获取访问权限。如果有线程正在访问，则等待。
注意：这个API内部的实现，其实只是对这个结构体做了一些列的检测，它的价值就在于它可以原子性的执行所有的检测。
参数：psc指向CRITICAL_SECTION结构体的地址
VOID EnterCriticalSection(PCRITICAL_SECTION pcs)


功能：和EnterCriticalSection作用相同，只是这个API不会因为另一个线程正在使用pcs这个结构而进入等待状态，它会根据返回值判断是否可以访问。然后继续执行下面的代码
参数：psc指向CRITICAL_SECTION结构体的地址
BOOL TryEnterCriticalSection(PCRITICAL_SECTION pcs)


功能：更新成员函数，以表示没有任何线程访问被保护资源，同时检查其他线程有没有因为调用EnterCriticalSection而处于等待状态。如果有，则更新成员函数，将一个等待的线程切回调度状态
参数：psc指向CRITICAL_SECTION结构体的地址
VOID LeaveCriticalSection(PCRITICAL_SECTION pcs)



案例分析：

#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <time.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD* g_pdwAnyData = NULL;
CRITICAL_SECTION g_csAnyData = {};
//模拟对这个全局的数据指针进行分配写入释放等操作
void AllocData();
void WriteData();
void ReadData();
void FreeData();

DWORD WINAPI ThreadProc( LPVOID lpParameter );

#define THREADCOUNT 20
#define THREADACTCNT 20		//每个线程执行20次动作

void _tmain()
{
	srand((unsigned int)time(NULL));
	/// \brief 初始化关键代码段，必须！
	/// \param g_csAnyData 
	if (!InitializeCriticalSectionAndSpinCount(&g_csAnyData,0x80000400) ) 
	{
		return;
	}

	HANDLE aThread[THREADCOUNT];
	DWORD ThreadID = 0;
	SYSTEM_INFO si = {};
	GetSystemInfo(&si);

	for( int i=0; i < THREADCOUNT; i++ )
	{
		aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadProc
			,NULL,CREATE_SUSPENDED,&ThreadID);
		SetThreadAffinityMask(aThread[i],1 << (i % si.dwNumberOfProcessors));//设置CPU亲缘性
		ResumeThread(aThread[i]);//执行
	}

	WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);//等待线程退出
	GRS_USEPRINTF();
	GRS_PRINTF(_T("关闭句柄"));
	for( int i=0; i < THREADCOUNT; i++ )
	{
		CloseHandle(aThread[i]);//关闭句柄
	}
	
	DeleteCriticalSection(&g_csAnyData);//释放关键代码段

	GRS_SAFEFREE(g_pdwAnyData);//释放内存
	_tsystem(_T("PAUSE"));
}

DWORD WINAPI ThreadProc( LPVOID lpParameter )
{
	GRS_USEPRINTF();
	int iAct = rand()%4;
	int iActCnt = THREADACTCNT;
	while(iActCnt --)
	{
		switch(iAct)
		{
		case 0:
			{
				AllocData();
			}
			break;
		case 1:
			{
				FreeData();
			}
			break;
		case 2:
			{
				WriteData();
			}
			break;
		case 3:
			{
				ReadData();
			}
			break;
		}
		iAct = rand()%4;
	}

	return iActCnt;
}

//四个处理函数
void AllocData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
		GRS_PRINTF(_T("Thread(ID:0x%x) Alloc Data\n\t"),GetCurrentThreadId());

		if(NULL == g_pdwAnyData)
		{
			g_pdwAnyData = (DWORD*)GRS_CALLOC(sizeof(DWORD));
			GRS_PRINTF(_T("g_pdwAnyData is NULL Alloc it Address(0x%08x)\n"),g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Can't Alloc it Address(0x%08x)\n"),g_pdwAnyData);
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
		//更新成员函数，以表示没有任何线程访问被保护资源，同时检查其他线程有没有因为调用EnterCriticalSection而处于等待状态。
		//如果有，则更新成员函数，将一个等待的线程切回调度状态
	}
}

void WriteData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
		GRS_PRINTF(_T("Thread(ID:0x%x) Write Data\n\t"),GetCurrentThreadId());

		if(NULL != g_pdwAnyData)
		{
			*g_pdwAnyData = rand();
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Write Val(%u)\n"),*g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData is NULL Can't Write\n"));
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
		//更新成员函数，以表示没有任何线程访问被保护资源，同时检查其他线程有没有因为调用EnterCriticalSection而处于等待状态。
//如果有，则更新成员函数，将一个等待的线程切回调度状态
	}
}

void ReadData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
		GRS_PRINTF(_T("Thread(ID:0x%x) Read Data\n\t"),GetCurrentThreadId());

		if(NULL != g_pdwAnyData)
		{
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Read Val(%u)\n"),*g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData is NULL Can't Read\n"));
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
		//更新成员函数，以表示没有任何线程访问被保护资源，同时检查其他线程有没有因为调用EnterCriticalSection而处于等待状态。
//如果有，则更新成员函数，将一个等待的线程切回调度状态
	}
}

void FreeData()
{
	EnterCriticalSection(&g_csAnyData);
	GRS_USEPRINTF();
	__try
	{
//		GRS_PRINTF(_T("Thread(ID:0x%x) Free Data\n\t"),GetCurrentThreadId());

		if(NULL != g_pdwAnyData)
		{
			GRS_PRINTF(_T("g_pdwAnyData isn't NULL Free it\n"));
			GRS_SAFEFREE(g_pdwAnyData);
		}
		else
		{
			GRS_PRINTF(_T("g_pdwAnyData is NULL Can't Free\n"));
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_csAnyData);
		//更新成员函数，以表示没有任何线程访问被保护资源，同时检查其他线程有没有因为调用EnterCriticalSection而处于等待状态。
//如果有，则更新成员函数，将一个等待的线程切回调度状态
	}
}


