// SaveDesktopDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SaveDesktopDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaveDesktopDlg dialog


CSaveDesktopDlg::CSaveDesktopDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSaveDesktopDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSaveDesktopDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
   m_bRestore = FALSE;
}


void CSaveDesktopDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveDesktopDlg)
	DDX_Control(pDX, IDC_LIST_DESKTOPS, m_wndListCtrl);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveDesktopDlg, CDialog)
	//{{AFX_MSG_MAP(CSaveDesktopDlg)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DESKTOPS, OnItemchangedListDesktops)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_DESKTOPS, OnDblclkListDesktops)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveDesktopDlg message handlers

//
// Sort items in alphabetical order.
//

static int CALLBACK CompareListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   // lParamSort contains a pointer to the list view control.
   // The lParam of an item is just its index.
   CListCtrl* pListCtrl = (CListCtrl*)lParamSort;
   CString    strItem1 = pListCtrl->GetItemText(lParam1, 0);
   CString    strItem2 = pListCtrl->GetItemText(lParam2, 0);

   return _tcsicmp(strItem1, strItem2);
}


//
// WM_INITDIALOG message handler
//

BOOL CSaveDesktopDlg::OnInitDialog() 
{
   RECT rect;
   TCHAR **ppVarList, *ptr;
   DWORD i, dwNumVars, dwResult;
   int iItem;

	CDialog::OnInitDialog();

   if (m_bRestore)
   {
      EnableDlgItem(this, IDC_EDIT_NAME, FALSE);
      SetWindowText(_T("Restore desktop configuration"));
   }

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 1, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_DESKTOP));
	
   // Setup list control
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Load list of existing desktop configurations
   dwResult = DoRequestArg5(NXCEnumUserVariables, g_hSession, (void *)CURRENT_USER,
                            _T("/Win32Console/Desktop/*/WindowCount"),
                            &dwNumVars, &ppVarList, _T("Loading desktop configuration list..."));
	if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumVars; i++)
      {
         ptr = _tcschr(&ppVarList[i][22], _T('/'));
         if (ptr != NULL)
            *ptr = 0;
         iItem = m_wndListCtrl.InsertItem(i, &ppVarList[i][22], 0);
         m_wndListCtrl.SetItemData(iItem, iItem);
         free(ppVarList[i]);
      }
      safe_free(ppVarList);

      m_wndListCtrl.SortItems(CompareListItems, (LPARAM)&m_wndListCtrl);
   }

	return TRUE;
}


//
// "OK" button handler
//

void CSaveDesktopDlg::OnOK() 
{
   TCHAR szBuffer[MAX_OBJECT_NAME];

   GetDlgItemText(IDC_EDIT_NAME, szBuffer, MAX_OBJECT_NAME);
   StrStrip(szBuffer);
   if (szBuffer[0] == 0)
   {
      MessageBox(_T("Desktop name cannot be empty!"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_NAME));
   }
   else
   {
	   CDialog::OnOK();
   }
}


//
// Handler for selection change in list control
//

void CSaveDesktopDlg::OnItemchangedListDesktops(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
   TCHAR szBuffer[MAX_OBJECT_NAME];

   if (pNMListView->uNewState & LVIS_SELECTED)
   {
      m_wndListCtrl.GetItemText(pNMListView->iItem, 0, szBuffer, MAX_OBJECT_NAME);
      SetDlgItemText(IDC_EDIT_NAME, szBuffer);
   }
	*pResult = 0;
}


//
// Handler for double click in list view
//

void CSaveDesktopDlg::OnDblclkListDesktops(NMHDR* pNMHDR, LRESULT* pResult) 
{
   PostMessage(WM_COMMAND, IDOK, 0);
	*pResult = 0;
}
