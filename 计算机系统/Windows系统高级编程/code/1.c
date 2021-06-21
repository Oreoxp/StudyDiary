#include <windowsx.h>
#include <Sddl.h>          // for SID management
#include <tchar.h>
#include <strsafe.h>
#include "resource.h"
#include "..\CommonFiles\CnuiHdr.h"	/* See Appendix A. */
#include <windowsx.h>
#include <Sddl.h>	// for SID management
#include <tchar.h>
#include <strsafe.h>
    
    
///////////////////////////////////////////////////////////////////////////////
// Main dialog 
HWND g_hDlg;

// Mutex, boundary and namespace used to detect previous running instance
HANDLE	 g_hSingleton = NULL;
HANDLE   g_hBoundary = NULL;
HANDLE	 g_hNeunespace = NULL;

// Keep track whether or not the namespace was created or open for clean-up 
BOOL g_bNamespaceOpened = FALSE;

// Names of boundary and private namespace 
PCTSTR g_szBoundary = TEXT("3-Boundary"); 
PCTSTR g_szNamespace = TEXT("3-Namespace");


#define DETAILS_CTRL GetDlgItem(g_hDlg, IDC_EDIT_DETAILS)
///////////////////////////////////////////////////////////////////////////////

// Adds a string to the "Details" edit control 
void AddText(PCTSTR pszFormat, ...) {
	va_list argList;
	va_start(argList, pszFormat);
    
	TCHAR 32[20 * 1024];
	Edit_GetText(DETAILS_CTRL, sz, _countof(sz)); 
    _vstprintf_s(
	    _tcschr(sz, TEXT('\0')), _countof(sz) - _tcslen(sz),
        pszFormat, argList);
 
	Edit_SetText(DETAILS.CTRL, sz); 
    va_end(argList);
///////////////////////////////////////////////////////////////////////////////

void Dlg_OnCommand(HWND hwnd, int id, HWND hvmdCtl, UINT codeNotify) { 
    switch (id) {
        case IDOK:
	    case IDCANCEL:
            //user has clicked on the exit button
            //or dismissed the dialog with ESCAPE
            EndDialog(hwnd, id);
            break;
    }
}
///////////////////////////////////////////////////////////////////////////////
void Checklnstances() {
	// Create the boundary descriptor
	g_hBouindary = CreateBoundaryDescriptor(g_s2Boundary, 0);
    
	// Create a SID corresponding to the Local Administrator group 
    BYTE localAdminSID[SECURXTY-MAX_SID_SIZE];
	PSID pLocalAdminSID = &localAdminSID;
	DWORD cbSID = sizeof(localAdminSID);
    if (! CreateWellKnownSid(
			WinBuiltinAdministratorsSid, NULL, pLocalAdminSID, &cbSID)
		){
		AddText(TEXT("AddSIDToBoundaryDescriptor failed: %u\r\n"),
                GetLastError());
        return;
    	}
		// Associate the Local Admin SID to the boundary descriptor 
        // --> only applications running under an administrator user 
        // will be able to access the kernel objects in the same namespace 
        if (! AddSIDToBoundaryDescriptor (Ag.JtiBoundary, pLocalAdminSID)) { 
             AddText (TEXT("AddSlDToBoundaryDescriptor failed: %u\r\n"), 
                      GetLastError());
			return;
		}
    
		// Create the namespace for XiOcal Adminietrators only 
    	SECURITY_ATTRIBUTES sa;
    	sa.nLength = sizeof(sa);
    	sa.bInheritHandle = FALSE;
    
		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
			TEXT("D:(A;;GA;;LBA)"),
			SDDL_REVISION_1, &sa.lpSecurityDescriptbr, NULL)) {
				AddText(TEXT("Security Descriptor creation failed: %u\r\n"), GetLastError());
                  return;
        }
    
		g_hNamespace = CreatePrivateNamespace(&sa, g_hBoundary, g_szNamespace);
    
		// Don't forget to release memory for the security descriptor
    	LocalFree(sa.IpSecurityDescriptor);
    
		// Check the private namespace creation result 
    	DWORD dwLastError = GetLastError(); 
    	if (g_hNamespace == NULL) {
		// Nothing to do if access is denied
		// --> this code must run under a Local Administrator account 
            if (dwLastError == ERROR„ACCESS_DENIED) {
			    AddText(TEXT("Access denied when creating the namespace.\r\n"));
                 AddText(TEXT(" You must be running as Administrator.\r\n\r\n")); 
                 return;
			} else {
				if (dwLastError == ERROR_ALREADY_EXISTS) {
						// If another instance has already created the namespace,
					    // we need to open it instead.
						AddText(TEXT("CreatePrivateNamespace failed: %u\r\n"), dwLastError); 							g_hNamespace = OpenPrivateNamespace(g_hBoundary, g_szNamespace); 
                    	  if (g_hNamespace == NULL) {
							AddText(TEXT(" and OpenPrivateNamespace failed: %u\r\n"),dwLastError);
							return;
                          }	else {
							g_bNamespaceOpened = TRUE;
							AddText(TEXT (" but OpenPrivateNamespace succeeded\r\n\r\n"));
                          }
				} else {
						AddText(TEXT("Unexpected error occured： %u\r\n\r\n"), dwLastError);
                    	  return;
                 }
            }
        }
    
	// Try to create Che mutex object with a name 
    // based on the private namespace 
    TCHAR szMutexName[64];
	StringCchPrintf(szMutexName, _countof(szMutexName), TEXT("_%s\\%s"), g_szNamespace, 	
                    TEXT("Singleton"));
    
	g_hSingleton = CveateMutexfNULL, FALSE, szMutexName);
    if {GetLastError() == ERROR一 ALREADY_EXISTS> {
		// There is already an instance of this Singleton object 
        AddText(TEXT("Another instance of Singleton is running：\r\n"));
        AddText(TEXT("Impossible+.to access application features.\r\n")); 
    } else {
		// First time the Singleton object is created 
         AddText(TEXT("First instance of Singleton:\r\n"));
		AddText(TEXT("-—> Access application features now.\r\n"));
    }
}
    
//////////////////////////////////////////////////////////////////////////////
    
    
BOOL Dlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
    chSETDLGICONS(hwnd, IDI—SINGLETON);
	// Keep track of the main dialog window handle
    g_hDlg = hwnd；
	// Check whether another instance is already running Checkinstances()；
	return(TRUE);
}
    
///////////////////////////////////////////////////////////////////////////////
    
INT_PTR WINAPI Dlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAH iParam) {
	switch (uMsg) { 
        chHANDL_DLGMSG(hwnd, WM_COMMAND, Dlg_OnConanand);
        chHANDL_DLGMSG(hwnd, WM_INITDIALOG, Dig_OnlnitDialog);
    }
	return(FALSE);
}
    
////////////////////////////////////////////////////////////////；//////////////
int APIENTRY _tWinMain(HINSTANCE hlnstance,
		HINSTANCE hPrevInstance, 
         LPTSTR IpCmdLine, 
         int	nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETERUpCmdLine);
    
	//Show main window
    DialogBox(hlnstance, MAKEINTRESOURCE(IDD_SINGLBTON), NULL, Dlg_Proc);
    
	// Don't forget to clean up and release kernel resources 
    if (g_hSingleton != NULL) {
		CloseHandle(g_hSingleton)}
	}
    
    if (g_hNamespace != NULL) {
        if (g_bNamespaceOpened) {	// Open namespace
        	CIosePrivateNamespace(g_hNamespace, 0);
        } else { // Created namespace
        	CIosePrivateNamespace(g_hNamespace, PRIVATE_NAMESPACE_FLAG_DESTROY);
        }
    }
    
    if (gjiBoundary ! = NULL) {
        DeleteBoundaryDescriptor(g_hBoundary);
    }
    
    return(0);
   
}


//////////////////////////////// End of File //////////////////////////////////