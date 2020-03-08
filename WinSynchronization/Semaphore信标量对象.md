## SEMAPHORE信标量对象

概念：Semaphore是旗语的意思，在Windows中，Semaphore对象用来控制对资源的并发访问数。Semaphore对象具有一个计数值，当值大于0时，Semaphore被置信号，当计数值等于0时，Semaphore被清除信号。每次针对Semaphore的wait functions返回时，计数值被减1，调用ReleaseSemaphore可以将计数值增加 lReleaseCount 参数值指定的值。


API：
CreateSemaphore函数用于创建一个Semaphore
HANDLE CreateSemaphore(
LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
LONG lInitialCount,
LONG lMaximumCount,
LPCTSTR lpName
);
lpSemaphoreAttributes   为安全属性，
lInitialCount           为Semaphore的初始值，
lMaximumCount           为最大值，
lpName                  为Semaphore对象的名字，NULL表示创建匿名Semaphore

此外还可以调用OpenSemaphore来打开已经创建的非匿名Semaphore
HANDLE OpenSemaphore(
DWORD dwDesiredAccess,
BOOL bInheritHandle,
LPCTSTR lpName
);

调用ReleaseSemaphore增加Semaphore计算值
BOOL ReleaseSemaphore(
HANDLE hSemaphore,
LONG lReleaseCount,
LPLONG lpPreviousCount
);
lpReleaseCount参数表示要增加的数值，
lpPreviousCount参数用于返回之前的计算值，如果不需要可以设置为NULL

比如我们要控制到服务器的连接数不超过10个，可以创建一个Semaphore，初值为10，每当要连接到服务器时，使用WaitForSingleObject请求Semaphore，当成功返回后再尝试连接到服务器，当连接失败或连接使用完后释放时，调用ReleaseSemaphore增加Semaphore计数值。


案例：
#include <windows.h>
#include <stdio.h>

//一个只能容纳10个客人的餐馆来了12位客人.......
#define MAX_SEM_COUNT 10
#define THREADCOUNT 12

HANDLE ghSemaphore = NULL;
DWORD WINAPI ThreadProc( LPVOID );
void main()
{
	HANDLE aThread[THREADCOUNT] = {};
	DWORD ThreadID = 0;
	int i = 0;

	ghSemaphore = CreateSemaphore(NULL,MAX_SEM_COUNT,MAX_SEM_COUNT,NULL);
	if (ghSemaphore == NULL) 
	{
		printf("CreateSemaphore error: %d\n", GetLastError());
		return;
	}

	for( i=0; i < THREADCOUNT; i++ )
	{
		aThread[i] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) ThreadProc, 
			NULL,0,&ThreadID);
		if( aThread[i] == NULL )
		{
			printf("CreateThread error: %d\n", GetLastError());
			return;
		}
	}
	WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);

	for( i=0; i < THREADCOUNT; i++ )
		CloseHandle(aThread[i]);

	CloseHandle(ghSemaphore);
	
	system("PAUSE");
}

DWORD WINAPI ThreadProc( LPVOID lpParam )
{
	DWORD dwWaitResult; 
	BOOL bContinue=TRUE;

	while(bContinue)
	{
		dwWaitResult = WaitForSingleObject(ghSemaphore,0L);
		switch (dwWaitResult) 
		{ 
		case WAIT_OBJECT_0: //等到了
			printf("Thread %d: wait succeeded\n", GetCurrentThreadId());
			bContinue=FALSE;            
			Sleep(5);
			if (!ReleaseSemaphore(ghSemaphore,1,NULL) )
			{
				printf("ReleaseSemaphore error: %d\n", GetLastError());
			}
			break; 
		case WAIT_TIMEOUT: 
			printf("Thread %d: wait timed out\n", GetCurrentThreadId());
			break; 
		}
	}
	return TRUE;
}

