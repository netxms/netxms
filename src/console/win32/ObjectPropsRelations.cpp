// ObjectPropsRelations.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsRelations.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsRelations property page

IMPLEMENT_DYNCREATE(CObjectPropsRelations, CPropertyPage)

CObjectPropsRelations::CObjectPropsRelations() : CPropertyPage(CObjectPropsRelations::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsRelations)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_pImageList = NULL;
}

CObjectPropsRelations::~CObjectPropsRelations()
{
   delete m_pImageList;
}

void CObjectPropsRelations::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsRelations)
	DDX_Control(pDX, IDC_LIST_PARENTS, m_wndListParents);
	DDX_Control(pDX, IDC_LIST_CHILDS, m_wndListChilds);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsRelations, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsRelations)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsRelations message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsRelations::OnInitDialog() 
{
   DWORD i;
   RECT rect;
   NXC_OBJECT *pObject;
   int iItem;

	CPropertyPage::OnInitDialog();

   // Create image list for list controls
   m_pImageList = new CImageList;
   m_pImageList->Create(g_pObjectSmallImageList);

   // Setup list controls
   m_wndListChilds.GetClientRect(&rect);
   m_wndListChilds.InsertColumn(0, "Object", LVCFMT_LEFT, rect.right);
   m_wndListChilds.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndListChilds.SetImageList(m_pImageList, LVSIL_SMALL);

   m_wndListParents.GetClientRect(&rect);
   m_wndListParents.InsertColumn(0, "Object", LVCFMT_LEFT, rect.right);
   m_wndListParents.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndListParents.SetImageList(m_pImageList, LVSIL_SMALL);

   // Fill childs list
   for(i = 0; i < m_pObject->dwNumChilds; i++)
   {
      pObject = NXCFindObjectById(g_hSession, m_pObject->pdwChildList[i]);
      if (pObject != NULL)
      {
         iItem = m_wndListChilds.InsertItem(0x7FFFFFFF, pObject->szName,
                                            GetObjectImageIndex(pObject));
         if (iItem != -1)
         {
            m_wndListChilds.SetItemData(iItem, (LPARAM)pObject);
         }
      }
   }
	
   // Fill parents list
   for(i = 0; i < m_pObject->dwNumParents; i++)
   {
      pObject = NXCFindObjectById(g_hSession, m_pObject->pdwParentList[i]);
      if (pObject != NULL)
      {
         iItem = m_wndListParents.InsertItem(0x7FFFFFFF, pObject->szName,
                                             GetObjectImageIndex(pObject));
         if (iItem != -1)
         {
            m_wndListChilds.SetItemData(iItem, (LPARAM)pObject);
         }
      }
   }

   // Disable unbind buttons depending on object's class
   switch(m_pObject->iClass)
   {
      case OBJECT_NETWORK:
      case OBJECT_SUBNET:
      case OBJECT_TEMPLATEROOT:
         EnableDlgItem(this, IDC_BUTTON_UNBIND_PARENT, FALSE);
         EnableDlgItem(this, IDC_BUTTON_UNBIND_CHILD, FALSE);
         break;
      case OBJECT_SERVICEROOT:
         EnableDlgItem(this, IDC_BUTTON_UNBIND_PARENT, FALSE);
         break;
      case OBJECT_NODE:
         EnableDlgItem(this, IDC_BUTTON_UNBIND_CHILD, FALSE);
         break;
      default:
         break;
   }
	
	return TRUE;
}
