// AlarmBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxav.h"
#include "AlarmBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser

IMPLEMENT_DYNCREATE(CAlarmBrowser, CHtmlView)

CAlarmBrowser::CAlarmBrowser()
{
	//{{AFX_DATA_INIT(CAlarmBrowser)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CAlarmBrowser::~CAlarmBrowser()
{
}

void CAlarmBrowser::DoDataExchange(CDataExchange* pDX)
{
	CHtmlView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAlarmBrowser)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAlarmBrowser, CHtmlView)
	//{{AFX_MSG_MAP(CAlarmBrowser)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser diagnostics

#ifdef _DEBUG
void CAlarmBrowser::AssertValid() const
{
	CHtmlView::AssertValid();
}

void CAlarmBrowser::Dump(CDumpContext& dc) const
{
	CHtmlView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser message handlers


//
// Handle navigation
//

void CAlarmBrowser::OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags, LPCTSTR lpszTargetFrameName, CByteArray& baPostedData, LPCTSTR lpszHeaders, BOOL* pbCancel) 
{
   DWORD dwId;

	CHtmlView::OnBeforeNavigate2(lpszURL, nFlags,	lpszTargetFrameName, baPostedData, lpszHeaders, pbCancel);
   if (!_tcsncmp(lpszURL, _T("nxav:"), 5))
   {
      *pbCancel = TRUE;
      switch(lpszURL[5])
      {
         case 'A':   // Acknowlege
            dwId = _tcstoul(&lpszURL[7], NULL, 10);
            AcknowlegeAlarm(dwId);
            break;
         default:
            break;
      }
   }
}


//
// Acknowlege alarm by ID
//

BOOL CAlarmBrowser::AcknowlegeAlarm(DWORD dwAlarmId)
{
   DWORD dwResult;

   dwResult = DoRequestArg1(NXCAcknowlegeAlarm, (void *)dwAlarmId, _T("Acknowleging alarm..."));
   return (dwResult == RCC_SUCCESS);
}
