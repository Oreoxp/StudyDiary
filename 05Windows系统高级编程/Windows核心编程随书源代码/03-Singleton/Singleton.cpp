/******************************************************************************
Module:  Singleton.cpp
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/


#include "resource.h"

#include "..\CommonFiles\CmnHdr.h"     /* See Appendix A. */
#include <windowsx.h>
#include <Sddl.h>          // for SID management
#include <tchar.h>
#include <strsafe.h>



///////////////////////////////////////////////////////////////////////////////


// Main dialog
HWND     g_hDlg;

// 用于检测先前正在运行的实例的互斥量，边界和namespace
HANDLE   g_hSingleton = NULL;
HANDLE   g_hBoundary = NULL;
HANDLE   g_hNamespace = NULL;

// 跟踪名称空间是否已创建或已打开以进行清理
BOOL     g_bNamespaceOpened = FALSE;

// 边界和命名空间的名字
PCTSTR   g_szBoundary = TEXT("3-Boundary");
PCTSTR   g_szNamespace = TEXT("3-Namespace");


#define DETAILS_CTRL GetDlgItem(g_hDlg, IDC_EDIT_DETAILS)


///////////////////////////////////////////////////////////////////////////////


// 将字符串添加到“详细信息”编辑控件
void AddText(PCTSTR pszFormat, ...) {

   va_list argList;
   va_start(argList, pszFormat);

   TCHAR sz[20 * 1024];

   Edit_GetText(DETAILS_CTRL, sz, _countof(sz));
   _vstprintf_s(
      _tcschr(sz, TEXT('\0')), _countof(sz) - _tcslen(sz), 
      pszFormat, argList);
   Edit_SetText(DETAILS_CTRL, sz);
   va_end(argList);
}


///////////////////////////////////////////////////////////////////////////////


void Dlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {

   switch (id) {
      case IDOK:
      case IDCANCEL:
         //用户单击了“退出”按钮
         //或使用ESCAPE关闭该对话框
         EndDialog(hwnd, id);
         break;
   }
}


///////////////////////////////////////////////////////////////////////////////


void CheckInstances() {

   //创建边界描述符
   g_hBoundary = CreateBoundaryDescriptor(g_szBoundary, 0);

   //创建与本地管理员组相对应的SID
   BYTE localAdminSID[SECURITY_MAX_SID_SIZE];
   PSID pLocalAdminSID = &localAdminSID;
   DWORD cbSID = sizeof(localAdminSID);
   if (!CreateWellKnownSid(
      WinBuiltinAdministratorsSid, NULL, pLocalAdminSID, &cbSID)
      ) {
      AddText(TEXT("AddSIDToBoundaryDescriptor failed: %u\r\n"), 
         GetLastError());
      return;
   }
   
   // 将Local Admin SID与边界描述符关联--->只有在管理员用户下运行的应用程序才能访问同一名称空间中的内核对象
   if (!AddSIDToBoundaryDescriptor(&g_hBoundary, pLocalAdminSID)) {
      AddText(TEXT("AddSIDToBoundaryDescriptor failed: %u\r\n"), 
         GetLastError());
      return;
   }

   //仅为本地管理员创建名称空间
   SECURITY_ATTRIBUTES sa;
   sa.nLength = sizeof(sa);
   sa.bInheritHandle = FALSE;
   if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
      TEXT("D:(A;;GA;;;BA)"), 
      SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL)) {
      AddText(TEXT("Security Descriptor creation failed: %u\r\n"), GetLastError());
      return;
   }

   g_hNamespace = 
      CreatePrivateNamespace(&sa, g_hBoundary, g_szNamespace);

   //不要忘记为安全描述符释放内存
   LocalFree(sa.lpSecurityDescriptor);


   //检查私有名称空间创建结果
   DWORD dwLastError = GetLastError();
   if (g_hNamespace == NULL) {
      //如果访问被拒绝，则无需执行任何操作->此代码必须在本地管理员帐户下运行
      if (dwLastError == ERROR_ACCESS_DENIED) {
         AddText(TEXT("Access denied when creating the namespace.\r\n"));
         AddText(TEXT("   You must be running as Administrator.\r\n\r\n"));
         return;
      } else { 
         if (dwLastError == ERROR_ALREADY_EXISTS) {
         //如果另一个实例已经创建了命名空间，则需要打开它。
            AddText(TEXT("CreatePrivateNamespace failed: %u\r\n"), dwLastError);
            g_hNamespace = OpenPrivateNamespace(g_hBoundary, g_szNamespace);
            if (g_hNamespace == NULL) {
               AddText(TEXT("   and OpenPrivateNamespace failed: %u\r\n"), 
               dwLastError);
               return;
            } else {
               g_bNamespaceOpened = TRUE;
               AddText(TEXT("   but OpenPrivateNamespace succeeded\r\n\r\n"));
            }
         } else {
            AddText(TEXT("Unexpected error occured: %u\r\n\r\n"),
               dwLastError);
            return;
         }
      }
   }
   
   //尝试使用基于私有名称空间的名称创建互斥对象
   TCHAR szMutexName[64];
   StringCchPrintf(szMutexName, _countof(szMutexName), TEXT("%s\\%s"), 
      g_szNamespace, TEXT("Singleton"));

   g_hSingleton = CreateMutex(NULL, FALSE, szMutexName);
   if (GetLastError() == ERROR_ALREADY_EXISTS) {
      //已经有该Singleton对象的实例
      AddText(TEXT("Another instance of Singleton is running:\r\n"));
      AddText(TEXT("--> Impossible to access application features.\r\n"));
   } else  {
      // 第一次创建Singleton对象
      AddText(TEXT("First instance of Singleton:\r\n"));
      AddText(TEXT("--> Access application features now.\r\n"));
   }
}


///////////////////////////////////////////////////////////////////////////////


BOOL Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {

   chSETDLGICONS(hwnd, IDI_SINGLETON);

   // 跟踪主对话框窗口句柄
   g_hDlg = hwnd;

   // 检查另一个实例是否已经在运行
   CheckInstances();
      
   return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////


INT_PTR WINAPI Dlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   switch (uMsg) {
      chHANDLE_DLGMSG(hwnd, WM_COMMAND,    Dlg_OnCommand);
      chHANDLE_DLGMSG(hwnd, WM_INITDIALOG, Dlg_OnInitDialog);
   }

   return(FALSE);
}


///////////////////////////////////////////////////////////////////////////////


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   // Show main window 
   DialogBox(hInstance, MAKEINTRESOURCE(IDD_SINGLETON), NULL, Dlg_Proc);

   // 不要忘记清理和释放内核资源
   if (g_hSingleton != NULL) {
      CloseHandle(g_hSingleton);
   }

   if (g_hNamespace != NULL) {
      if (g_bNamespaceOpened) {  // Open namespace
         ClosePrivateNamespace(g_hNamespace, 0);
      } else { // Created namespace
         ClosePrivateNamespace(g_hNamespace, PRIVATE_NAMESPACE_FLAG_DESTROY);
      }
   }

   if (g_hBoundary != NULL) {
      DeleteBoundaryDescriptor(g_hBoundary);
   }

   return(0);
}


//////////////////////////////// End of File //////////////////////////////////
