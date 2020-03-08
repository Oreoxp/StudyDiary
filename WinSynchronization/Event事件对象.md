## Event事件对象
概念：
事件内核对象：事件对象包含一个使用计数，一个用来表示事件是手动还是自动重置事件的布尔值和一个表示事件是否被触发的布尔值。

事件对象类型：手动重置和自动重置。
    当一个手动重置事件被触发的时候，正在等待该事件的所有线程都将变成可调度状态。
    而当一个自动重置事件被触发时，只有一个正在等待该事件的线程变成可调度状态。

事件的用途：通常我们使用事件，让一个线程执行初始化任务，然后触发另一个线程执行剩余任务。

API：
HANDLE CreateEvent(
        PSECURITY_ATTRIBUTES   psa,
        BOOL bManualReset,
        BOOL bInitialState,
        PCSTR pszName);
作用：
创建一个事件内核对象。
参数：
psa           安全描述符
bManualReset  用来告诉系统是设置一个自动重置事件(FALSE)还是手动重置事件(TRUE)。
bInitialState 用来表示刚创建的事件初始化为触发事件（TRUE）还是未触发事件（FALSE）
pszName       表示一个事件名，可通过事件名共享内核对象
返回值：
返回一个内核对象的句柄值


HANDLE CreateEventEx(
        PSECURITY_ATTRIBUTES   psa,
        PCSTR pszName,
        DWORD dwFlags,
        DWORD dwDesiredAccess
        );
psa     安全描述符
pszName 表示一个事件名，可通过事件名共享内核对象。
dwFlags 可以取下面两个值
1.CREATE_EVENT_INITIAL_SET   等价于CreateEvent中传入的bInitialState参数，如果设置了，那么函数会将事件初始化为触发状态，否则为未触发状态
2.CREATE_EVENT_MANUAL_RESET等价为CreateEvent中传入的bManualReset参数，如果设置了，那么创建一个手动重置事件，否则创建一个自动重置事件
dwDesiredAccess允许我们制定创建的事件返回的句柄对事件有何种访问权限。相对而言，CreateEvent总是拥有所有访问权限。

 

BOOL SetEvent(HANDLE   hEvent);
将事件设置为触发状态

 

BOOL ResetEvent(HANDLE   hEvent);
将事件设置为未触发状态

案例：
#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define THREADCOUNT 4 

HANDLE ghWriteEvent = NULL; 
HANDLE ghThreads[THREADCOUNT] = {};

DWORD WINAPI ThreadProc(LPVOID);

void CreateEventsAndThreads(void) 
{
    int i = 0; 
    DWORD dwThreadID = 0;
	//创建成手动重置事件对象状态可以使所有线程都退出
	//如果改为自动,那么就只有一个线程有机会等到该事件,程序出现问题
	//这就是成功等待的副作用(像药物副作用一样)
    ghWriteEvent = CreateEvent(NULL,TRUE,FALSE,_T("WriteEvent")); 
    for(i = 0; i < THREADCOUNT; i++) 
    {
        ghThreads[i] = CreateThread(NULL,0,ThreadProc,NULL,0,&dwThreadID);
    }
}

void WriteToBuffer(VOID) 
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("Main thread writing to the shared buffer...\n"));
	if (! SetEvent(ghWriteEvent) ) 
    {
        GRS_PRINTF(_T("SetEvent failed (%d)\n"), GetLastError());
        return;
    }
}

void CloseEvents()
{
    CloseHandle(ghWriteEvent);
}

void _tmain()
{
	GRS_USEPRINTF();
    DWORD dwWaitResult = 0;
    CreateEventsAndThreads();    
    WriteToBuffer();
    GRS_PRINTF(_T("Main thread waiting for threads to exit...\n"));
    dwWaitResult = WaitForMultipleObjects(THREADCOUNT,ghThreads,TRUE,INFINITE);
    switch (dwWaitResult) 
    {
        case WAIT_OBJECT_0: 
            GRS_PRINTF(_T("All threads ended, cleaning up for application exit...\n"));
            break;
        default: 
            GRS_PRINTF(_T("WaitForMultipleObjects failed (%d)\n"), GetLastError());
            return;
    }
    CloseEvents();

	_tsystem(_T("PAUSE"));
}

DWORD WINAPI ThreadProc(LPVOID lpParam) 
{
	GRS_USEPRINTF();
    DWORD dwWaitResult = 0;
    GRS_PRINTF(_T("Thread %d waiting for write event...\n"), GetCurrentThreadId());
    dwWaitResult = WaitForSingleObject(ghWriteEvent,INFINITE);
    switch (dwWaitResult) 
    {
        case WAIT_OBJECT_0: 
            GRS_PRINTF(_T("Thread %d reading from buffer\n"),GetCurrentThreadId());
            break; 
        default: 
            GRS_PRINTF(_T("Wait error (%d)\n"), GetLastError()); 
            return 0; 
    }
    GRS_PRINTF(_T("Thread %d exiting\n"), GetCurrentThreadId());
    return 1;
}


