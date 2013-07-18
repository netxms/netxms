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
   m_dwParentObject = 0;   // All objects by default
   m_bSelectAddress = FALSE;  // Not in address selection mode by default
   m_bAllowEmptySelection = FALSE;
   m_bShowLoopback = FALSE;
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
	ON_BN_CLICKED(IDC_BUTTON_NONE, OnButtonNone)
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
   UINT32 i, dwNumObjects;
   int iItem;
   CBitmap bmp;
   static DWORD dwClassMask[15] = { 0, SCL_SUBNET, SCL_NODE, SCL_INTERFACE,
                                    SCL_NETWORK, SCL_CONTAINER, SCL_ZONE,
                                    SCL_SERVICEROOT, SCL_TEMPLATE, SCL_TEMPLATEGROUP,
                                    SCL_TEMPLATEROOT, SCL_NETWORKSERVICE,
                                    SCL_VPNCONNECTOR, SCL_CONDITION, SCL_CLUSTER };

	CDialog::OnInitDialog();

   // Prepare image list
   m_imageList.Create(g_pObjectSmallImageList);
	
   // Setup list control
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);
   m_wndListCtrl.GetClientRect(&rect);
   if (m_bSelectAddress)
   {
      m_wndListCtrl.InsertColumn(0, _T("Interface"), LVCFMT_LEFT, 100);
      m_wndListCtrl.InsertColumn(1, _T("IP Address"), LVCFMT_LEFT, 
                                 rect.right - 100 - GetSystemMetrics(SM_CXVSCROLL));
   }
   else
   {
      m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 150);
      m_wndListCtrl.InsertColumn(1, _T("Class"), LVCFMT_LEFT, 
                                 rect.right - 150 - GetSystemMetrics(SM_CXVSCROLL));
   }
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   if (m_bSingleSelection)
   {
      ::SetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE, 
         ::GetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE) | LVS_SINGLESEL);
   }

   // Fill in object list
   if (m_dwParentObject == 0)
   {
      NXCLockObjectIndex(g_hSession);
      pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(g_hSession, &dwNumObjects);
      for(i = 0; i < dwNumObjects; i++)
         if ((dwClassMask[pIndex[i].pObject->iClass] & m_dwAllowedClasses) &&
             (!pIndex[i].pObject->bIsDeleted))
         {
            iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pIndex[i].pObject->szName,
                                             pIndex[i].pObject->iClass);
            m_wndListCtrl.SetItemText(iItem, 1, g_szObjectClass[pIndex[i].pObject->iClass]);
            m_wndListCtrl.SetItemData(iItem, pIndex[i].pObject->dwId);
         }
      NXCUnlockObjectIndex(g_hSession);
   }
   else
   {
      NXC_OBJECT *pObject, *pChild;
      TCHAR szBuffer[16];

      pObject = NXCFindObjectById(g_hSession, m_dwParentObject);
      if (pObject != NULL)
      {
         for(i = 0; i < pObject->dwNumChilds; i++)
         {
            pChild = NXCFindObjectById(g_hSession, pObject->pdwChildList[i]);
            if (pChild != NULL)
            {
               if (m_bSelectAddress)
               {
                  if ((pChild->iClass == OBJECT_INTERFACE) &&
                      (pChild->dwIpAddr != 0) && (!pChild->bIsDeleted))
                  {
                     iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pChild->szName, pChild->iClass);
                     m_wndListCtrl.SetItemText(iItem, 1, IpToStr(pChild->dwIpAddr, szBuffer));
                     m_wndListCtrl.SetItemData(iItem, pObject->pdwChildList[i]);
                  }
               }
               else
               {
                  if ((dwClassMask[pChild->iClass] & m_dwAllowedClasses) &&
                      (!pChild->bIsDeleted))
                  {
                     iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pChild->szName, pChild->iClass);
                     m_wndListCtrl.SetItemText(iItem, 1, g_szObjectClass[pChild->iClass]);
                     m_wndListCtrl.SetItemData(iItem, pObject->pdwChildList[i]);
                  }
               }
            }
         }

         if (m_bSelectAddress && m_bShowLoopback)
         {
            iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, _T("<loopback>"), OBJECT_INTERFACE);
            m_wndListCtrl.SetItemText(iItem, 1, _T("127.0.0.1"));
            m_wndListCtrl.SetItemData(iItem, 0);
         }
      }
   }

   // Disable "None" button if empty selection is not allowed
   if (!m_bAllowEmptySelection)
   {
      EnableDlgItem(this, IDC_BUTTON_NONE, FALSE);
   }

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
         m_pdwObjectList[i] = (DWORD)m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }
	   CDialog::OnOK();
   }
   else
   {
      MessageBox(_T("You should select at least one object"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
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


//
// Empty selection
//

void CObjectSelDlg::OnButtonNone() 
{
   if (m_bAllowEmptySelection)
   {
      m_dwNumObjects = 0;
      CDialog::OnOK();
   }
}
