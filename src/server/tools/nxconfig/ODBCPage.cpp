// ODBCPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "ODBCPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CODBCPage property page

IMPLEMENT_DYNCREATE(CODBCPage, CPropertyPage)

CODBCPage::CODBCPage() : CPropertyPage(CODBCPage::IDD)
{
	//{{AFX_DATA_INIT(CODBCPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CODBCPage::~CODBCPage()
{
}

void CODBCPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CODBCPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CODBCPage, CPropertyPage)
	//{{AFX_MSG_MAP(CODBCPage)
	ON_BN_CLICKED(IDC_BUTTON_ODBC, OnButtonOdbc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODBCPage message handlers

void CODBCPage::OnButtonOdbc() 
{
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   TCHAR szBuffer[1024];

   ExpandEnvironmentStrings(_T("%SystemRoot%\\system32\\odbcad32.exe"), szBuffer, 1024);
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   if (CreateProcess(NULL, szBuffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
   {
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
   }
   else
   {
      MessageBox(_T("Failed to create process"), _T("Error"), MB_OK | MB_ICONSTOP);
   }
}
