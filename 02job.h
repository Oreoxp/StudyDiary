/**
 * @file job.h
 * @author dxp
 * @brief windows job
 * @version 0.1
 * @date 2020-03-09
 * 
 * @copyright Copyright (c) 2020
 * 
CloseHandle、ExitProcess总是不能很好的结束子进程，作业系统可以很好的解决
 */

//--------------------------------生成作业
/**
 * @brief 
 * 
 * @param hProcess 
 * @param hJob 
 * @param pbInJob 
 * @return BOOL 
 */
BOOL IsProcessInJob(
    HANDLE hProcess,
    HANDLE hJob,
    PBOOL pbInJob
);

/**
 * @brief Create a Job Object object
 * 
 * @param psa 安全描述符 
 * @param pszName 命名
 * @return HANDLE Job句柄
 */
HANDLE CreateJobObject(
    PSECURITY_ATTRIBUTES psa,
    PCSTR pszName
);


HANDLE OpenJobObject(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    PCTSTR pszName
);

//---------------------------------对作业进行限制
/**
 * @brief 向作业施加限制
 * 
 * @param hJob 指定要限制的作业
 * @param JobObjectInformationClass 指定了要施加的限制类型
 * @param pJobObjectInofrmation 指向一个数据结构 包含了具体的限制设置
 * @param cbJobObjectInformationSize 此数据结构的大小
 * @return BOOL 
 */
BOOL SetInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    PVOID pJobObjectInofrmation,
    DWORD cbJobObjectInformationSize
);
/**
 * @brief 具体的限制
 <限制类型>        <第二个参数的值>                      <第三个参数所对应的数据结构>

 基本限额           JobObjectBasicLimiitInfomation       JOBOBJECT_BASIC_LIMIT_INFORMATION    
 扩展后的基本限额    JobObjectExtendedLimitInformation    JOBOBJECT_EXTENDED_LIMIT_INFORMATION
 基本的UI限制       JobObjectNasicUIRestrictions         JOBOBJECT_BASIC_UI_RESTRICTIONS
 安全限额           JobObjectSecurityLimitInfomation     JOBOBJECT_SECURITY_LIMIT_INFORMATION
 * 
 */



 //-----------------------------------将进程放进作业中
/**
 * @brief 将进程显示的放进作业中     向系统表明 此进程是作业的一部分
 * 注意：此进程必须是没有作业的，可使用IsPricessInJob检查
 * @param hJob job句柄
 * @param hProcess 线程句柄
 * @return BOOL 是否成功
 */
 BOOL AssignProcessToJobObject(
     HANDLE hJob,
     HANDLE hProcess
 );

 //--------------------------------终止作业中的所有线程！！
 /**
  * @brief 杀死作业内部所有进程
  * 注意：这类似于为作业内的每一个进程调用TerminateProcess，并将所有推出代码设为uExitCode
  * @param hJob 作业句柄
  * @param uExitCode 退出码
  * @return BOOL 
  */
 BOOL TerminateJobObject(
     HANDLE hJob,
     UINT uExitCode
 );

 //-----------------------------查询作业统计信息