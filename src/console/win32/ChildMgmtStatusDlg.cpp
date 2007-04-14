// ChildMgmtStatusDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ChildMgmtStatusDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildMgmtStatusDlg dialog


CChildMgmtStatusDlg::CChildMgmtStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChildMgmtStatusDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChildMgmtStatusDlg)
	//}}AFX_DATA_INIT
}


void CChildMgmtStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChildMgmtStatusDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LIST_OBJECTS, m_wndListCtrl);
}


BEGIN_MESSAGE_MAP(CChildMgmtStatusDlg, CDialog)
	//{{AFX_MSG_MAP(CChildMgmtStatusDlg)
	//}}AFX_MSG_MAP
	ON_MESSAGE(CLN_SETITEMS, OnComboListSetItems)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildMgmtStatusDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CChildMgmtStatusDlg::OnInitDialog() 
{
	RECT rect;
	DWORD i;
	NXC_OBJECT *pObject;
	int nItem;

	CDialog::OnInitDialog();
	
   // Prepare image list
   m_imageList.Create(g_pObjectSmallImageList);

	// Init list control
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Object"), LVCFMT_LEFT, rect.right - 80);
	m_wndListCtrl.InsertColumn(1, _T("Managed"), LVCFMT_LEFT, 80);
	m_wndListCtrl.SetReadOnlyColumn(0);
	m_wndListCtrl.SetComboColumn(1);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

	// Fill list control
	for(i = 0; i < m_pObject->dwNumChilds; i++)
	{
		pObject = NXCFindObjectById(g_hSession, m_pObject->pdwChildList[i]);
		if (pObject != NULL)
		{
			nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pObject->szName, GetObjectImageIndex(pObject));
			if (nItem != -1)
			{
				m_wndListCtrl.SetItemText(nItem, 1, (pObject->iStatus == STATUS_UNMANAGED) ? _T("No") : _T("Yes"));
				m_wndListCtrl.SetItemData(nItem, pObject->dwId);
			}
		}
	}
	
	return TRUE;
}


//
// Handler for CLN_SETITEMS message
//

LRESULT CChildMgmtStatusDlg::OnComboListSetItems(WPARAM wParam, LPARAM lParam)
{
	CStringList *pComboList = reinterpret_cast<CStringList *>(lParam);

	pComboList->RemoveAll();
	pComboList->AddTail("Yes");
	pComboList->AddTail("No");
	return TRUE;
}


//
// Structure for changing management status
//

struct MGMT_STATUS
{
	DWORD dwObjectId;
	BOOL isManaged;
};


//
// Status changing thread
//

static DWORD ChangeMgmtStatus(DWORD dwCount, MGMT_STATUS *pList)
{
	DWORD i, dwResult;

	for(i = 0; i < dwCount; i++)
	{
		dwResult = NXCSetObjectMgmtStatus(g_hSession, pList[i].dwObjectId, pList[i].isManaged);
		if (dwResult != RCC_SUCCESS)
			break;
	}
	return dwResult;
}


//
// "OK" button handler
//

void CChildMgmtStatusDlg::OnOK() 
{
	int i, nItems;
	DWORD dwResult;
	MGMT_STATUS *pList;
	TCHAR szBuffer[32];

	nItems = m_wndListCtrl.GetItemCount();
	if (nItems > 0)
	{
		pList = (MGMT_STATUS *)malloc(sizeof(MGMT_STATUS) * nItems);
		for(i = 0; i < nItems; i++)
		{
			pList[i].dwObjectId = m_wndListCtrl.GetItemData(i);
			m_wndListCtrl.GetItemText(i, 1, szBuffer, 32);
			pList[i].isManaged = !_tcsicmp(szBuffer, _T("Yes"));
		}

		dwResult = DoRequestArg2(ChangeMgmtStatus, (void *)nItems, pList,
		                         _T("Changing management status of child objects..."));
		if (dwResult != RCC_SUCCESS)
			theApp.ErrorBox(dwResult, _T("Cannot change object's management status: %s"));

		free(pList);
	}

	CDialog::OnOK();
}
