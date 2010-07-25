// SrvDepsPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "SrvDepsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSrvDepsPage property page

IMPLEMENT_DYNCREATE(CSrvDepsPage, CPropertyPage)

CSrvDepsPage::CSrvDepsPage() : CPropertyPage(CSrvDepsPage::IDD)
{
	//{{AFX_DATA_INIT(CSrvDepsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   
   m_ppszServiceNames = NULL;
   m_dwNumServices = 0;
}

CSrvDepsPage::~CSrvDepsPage()
{
   DWORD i;

   for(i = 0; i < m_dwNumServices; i++)
      free(m_ppszServiceNames[i]);
   safe_free(m_ppszServiceNames);
}

void CSrvDepsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSrvDepsPage)
	DDX_Control(pDX, IDC_LIST_SERVICES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSrvDepsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSrvDepsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSrvDepsPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CSrvDepsPage::OnInitDialog() 
{
   SC_HANDLE hMgr;
   ENUM_SERVICE_STATUS *pList;
   BOOL bRet, bMore;
   DWORD i, j, dwBytes, dwNumServices, dwResume;
   RECT rect;
   int iItem;

	CPropertyPage::OnInitDialog();
	
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

   // Enumerate services
   hMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if (hMgr != NULL)
   {
      pList = (ENUM_SERVICE_STATUS *)malloc(8192);
      dwResume = 0;
      do
      {
         bRet = EnumServicesStatus(hMgr, SERVICE_WIN32, SERVICE_STATE_ALL,
                                   pList, 8192, &dwBytes, &dwNumServices, &dwResume);
         bMore = bRet ? FALSE : (GetLastError() == ERROR_MORE_DATA);
         if (bRet || bMore)
         {
            m_ppszServiceNames = (TCHAR **)realloc(m_ppszServiceNames, sizeof(TCHAR *) * (m_dwNumServices + dwNumServices));
            for(i = 0, j = m_dwNumServices; i < dwNumServices; i++, j++)
            {
               m_ppszServiceNames[j] = _tcsdup(pList[i].lpServiceName);
               iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pList[i].lpDisplayName);
               m_wndListCtrl.SetItemData(iItem, j);
            }
            m_dwNumServices += dwNumServices;
         }
      } while(bMore);
      CloseServiceHandle(hMgr);
      free(pList);
   }

	return TRUE;
}


//
// "Next" button handler
//

LRESULT CSrvDepsPage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;
   int i, nNumItems, nLen;
   DWORD dwIndex;

   safe_free(pc->m_pszDependencyList);
   pc->m_pszDependencyList = NULL;
   pc->m_dwDependencyListSize = 0;

   nNumItems = m_wndListCtrl.GetItemCount();
   for(i = 0; i < nNumItems; i++)
   {
      if (ListView_GetCheckState(m_wndListCtrl.m_hWnd, i))
      {
         dwIndex = (DWORD)m_wndListCtrl.GetItemData(i);
         nLen = (int)_tcslen(m_ppszServiceNames[dwIndex]);
         pc->m_pszDependencyList = (TCHAR *)realloc(pc->m_pszDependencyList,
                                                    (pc->m_dwDependencyListSize + nLen + 1) * sizeof(TCHAR));
         _tcscpy(&pc->m_pszDependencyList[pc->m_dwDependencyListSize], m_ppszServiceNames[dwIndex]);
         pc->m_dwDependencyListSize += nLen + 1;
      }
   }
   if (pc->m_dwDependencyListSize > 0)
   {
      pc->m_dwDependencyListSize++;
      pc->m_pszDependencyList = (TCHAR *)realloc(pc->m_pszDependencyList, pc->m_dwDependencyListSize * sizeof(TCHAR));
      pc->m_pszDependencyList[pc->m_dwDependencyListSize - 1] = 0;
   }
	
	return CPropertyPage::OnWizardNext();
}
