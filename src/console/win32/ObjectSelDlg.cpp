// ObjectSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectSelDlg dialog


CObjectSelDlg::CObjectSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CObjectSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CObjectSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_dwNumObjects = 0;
   m_pdwObjectList = NULL;
   m_dwAllowedClasses = 0xFFFF;  // Allow all classes by default
   m_bSingleSelection = FALSE;
}


CObjectSelDlg::~CObjectSelDlg()
{
   safe_free(m_pdwObjectList);
}


void CObjectSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectSelDlg)
	DDX_Control(pDX, IDC_LIST_OBJECTS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectSelDlg, CDialog)
	//{{AFX_MSG_MAP(CObjectSelDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_OBJECTS, OnDblclkListObjects)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectSelDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectSelDlg::OnInitDialog() 
{
   RECT rect;
   NXC_OBJECT_INDEX *pIndex;
   DWORD i, dwNumObjects;
   int iItem;
   CBitmap bmp;
   static DWORD dwClassMask[8] = { 0, SCL_SUBNET, SCL_NODE, SCL_INTERFACE,
                                   SCL_NETWORK, SCL_CONTAINER, SCL_ZONE,
                                   SCL_SERVICEROOT };

	CDialog::OnInitDialog();

   // Prepare image list
   m_imageList.Create(g_pObjectSmallImageList);
	
   // Setup list control
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, "Name", LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(1, "Class", LVCFMT_LEFT, 
                              rect.right - 150 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   if (m_bSingleSelection)
   {
      ::SetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE, 
         ::GetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE) | LVS_SINGLESEL);
   }

   // Fill in object list
   NXCLockObjectIndex();
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(&dwNumObjects);
   for(i = 0; i < dwNumObjects; i++)
      if (dwClassMask[pIndex[i].pObject->iClass] & m_dwAllowedClasses)
      {
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pIndex[i].pObject->szName,
                                          GetObjectImageIndex(pIndex[i].pObject));
         m_wndListCtrl.SetItemText(iItem, 1, g_szObjectClass[pIndex[i].pObject->iClass]);
         m_wndListCtrl.SetItemData(iItem, pIndex[i].pObject->dwId);
      }
   NXCUnlockObjectIndex();

	return TRUE;
}


//
// Handle OK button
//

void CObjectSelDlg::OnOK() 
{
   int iItem;
   DWORD i;

   m_dwNumObjects = m_wndListCtrl.GetSelectedCount();
   if (m_dwNumObjects > 0)
   {
      // Build list of selected objects
      m_pdwObjectList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumObjects);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; iItem != -1; i++)
      {
         m_pdwObjectList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }
	   CDialog::OnOK();
   }
   else
   {
      MessageBox("You should select at least one object", "Warning", MB_OK | MB_ICONEXCLAMATION);
   }
}


//
// Handler for doble click in list control
//

void CObjectSelDlg::OnDblclkListObjects(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
      PostMessage(WM_COMMAND, IDOK, 0);
	*pResult = 0;
}
