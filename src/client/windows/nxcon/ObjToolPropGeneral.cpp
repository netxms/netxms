// ObjToolPropGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjToolPropGeneral.h"
#include "UserSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropGeneral property page

IMPLEMENT_DYNCREATE(CObjToolPropGeneral, CPropertyPage)

CObjToolPropGeneral::CObjToolPropGeneral() : CPropertyPage(CObjToolPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CObjToolPropGeneral)
	m_strDescription = _T("");
	m_strName = _T("");
	m_strRegEx = _T("");
	m_strEnum = _T("");
	m_strData = _T("");
	//}}AFX_DATA_INIT
   m_pdwACL = NULL;
}

CObjToolPropGeneral::~CObjToolPropGeneral()
{
   safe_free(m_pdwACL);
}

void CObjToolPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjToolPropGeneral)
	DDX_Control(pDX, IDC_LIST_USERS, m_wndListCtrl);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	DDX_Text(pDX, IDC_EDIT_REGEX, m_strRegEx);
	DDV_MaxChars(pDX, m_strRegEx, 255);
	DDX_Text(pDX, IDC_EDIT_ENUM, m_strEnum);
	DDV_MaxChars(pDX, m_strEnum, 255);
	DDX_Text(pDX, IDC_EDIT_DATA, m_strData);
	DDV_MaxChars(pDX, m_strData, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjToolPropGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CObjToolPropGeneral)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropGeneral message handlers

BOOL CObjToolPropGeneral::OnInitDialog() 
{
   RECT rect;
   DWORD i;
   NXC_USER *pUser;
   int iItem;
   static TCHAR *m_szDataField[] = { _T("Operation"), _T("Action"), _T("Title"),
                                     _T("Title"), _T("URL"), _T("Command") };

	CPropertyPage::OnInitDialog();
	
   // Setup list view
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 
                              rect.right - GetSystemMetrics(SM_CXVSCROLL) - 4);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_USER));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_USER_GROUP));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EVERYONE));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Populate user list with current ACL data
   for(i = 0; i < m_dwACLSize; i++)
   {
      pUser = NXCFindUserById(g_hSession, m_pdwACL[i]);
      if (pUser != NULL)
      {
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pUser->szName,
                                             (pUser->dwId == GROUP_EVERYONE) ? 2 :
                                                ((pUser->dwId & GROUP_FLAG) ? 1 : 0));
         if (iItem != -1)
            m_wndListCtrl.SetItemData(iItem, m_pdwACL[i]);
      }
   }

   // Setup data fields
   if (m_iToolType != TOOL_TYPE_TABLE_AGENT)
   {
      EnableDlgItem(this, IDC_STATIC_ENUM, FALSE);
      EnableDlgItem(this, IDC_EDIT_ENUM, FALSE);
      EnableDlgItem(this, IDC_STATIC_REGEX, FALSE);
      EnableDlgItem(this, IDC_EDIT_REGEX, FALSE);
   }
   SetDlgItemText(IDC_STATIC_DATA, m_szDataField[m_iToolType]);

   return TRUE;
}


//
// Handler for "Add..." button
//

void CObjToolPropGeneral::OnButtonAdd() 
{
   CUserSelectDlg wndSelectDlg;
   DWORD i, j;
   int iItem = -1;
   NXC_USER *pUser;

   if (wndSelectDlg.DoModal() == IDOK)
   {
      for(j = 0; j < wndSelectDlg.m_dwNumUsers; j++)
      {
         // Check if we have this user in ACL already
         for(i = 0; i < m_dwACLSize; i++)
            if (m_pdwACL[i] == wndSelectDlg.m_pdwUserList[j])
            {
               LVFINDINFO lvfi;

               // Find appropriate item in list
               lvfi.flags = LVFI_PARAM;
               lvfi.lParam = wndSelectDlg.m_pdwUserList[j];
               iItem = m_wndListCtrl.FindItem(&lvfi);
               break;
            }

         if (i == m_dwACLSize)
         {
            // Create new entry in ACL
            m_dwACLSize++;
            m_pdwACL = (DWORD *)realloc(m_pdwACL, sizeof(DWORD) * m_dwACLSize);
            m_pdwACL[i] = wndSelectDlg.m_pdwUserList[j];

            // Add new line to user list
            pUser = NXCFindUserById(g_hSession, wndSelectDlg.m_pdwUserList[j]);
            if (pUser != NULL)
            {
               iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pUser->szName,
                                                (pUser->dwId == GROUP_EVERYONE) ? 2 :
                                                   ((pUser->dwId & GROUP_FLAG) ? 1 : 0));
               m_wndListCtrl.SetItemData(iItem, wndSelectDlg.m_pdwUserList[j]);
            }
         }
      }

      // Select new item
      if (iItem != -1)
         SelectListViewItem(&m_wndListCtrl, iItem);
   }
}


//
// Handler for "Delete" button
//

void CObjToolPropGeneral::OnButtonDelete() 
{
   int iItem;
   DWORD i, dwUserId;

   iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   while(iItem != -1)
   {
      dwUserId = m_wndListCtrl.GetItemData(iItem);
      m_wndListCtrl.DeleteItem(iItem);
      for(i = 0; i < m_dwACLSize; i++)
         if (m_pdwACL[i] == dwUserId)
         {
            m_dwACLSize--;
            memmove(&m_pdwACL[i], &m_pdwACL[i + 1], sizeof(DWORD) * (m_dwACLSize - i));
            break;
         }
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   }
}
