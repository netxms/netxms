// ObjectPropsSecurity.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsSecurity.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsSecurity property page

IMPLEMENT_DYNCREATE(CObjectPropsSecurity, CPropertyPage)

CObjectPropsSecurity::CObjectPropsSecurity() : CPropertyPage(CObjectPropsSecurity::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsSecurity)
	m_bInheritRights = FALSE;
	//}}AFX_DATA_INIT
   m_pAccessList = NULL;
   m_dwAclSize = 0;
}

CObjectPropsSecurity::~CObjectPropsSecurity()
{
   MemFree(m_pAccessList);
}

void CObjectPropsSecurity::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsSecurity)
	DDX_Control(pDX, IDC_LIST_USERS, m_wndUserList);
	DDX_Check(pDX, IDC_CHECK_INHERIT_RIGHTS, m_bInheritRights);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsSecurity, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsSecurity)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsSecurity message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsSecurity::OnInitDialog() 
{
   DWORD i;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
   m_bIsModified = FALSE;

   // Copy object's ACL
   m_dwAclSize = m_pObject->dwAclSize;
   m_pAccessList = (NXC_ACL_ENTRY *)nx_memdup(m_pObject->pAccessList, 
                                              sizeof(NXC_ACL_ENTRY) * m_dwAclSize);

   // Populate user list with current ACL data
   for(i = 0; i < m_dwAclSize; i++)
   {
      char szBuffer[32];
      sprintf(szBuffer, "%d", m_pAccessList[i].dwUserId);
      m_wndUserList.InsertItem(0x7FFFFFFF, szBuffer);
   }
	
	return TRUE;
}


//
// PSN_OK handler
//

void CObjectPropsSecurity::OnOK() 
{
	CPropertyPage::OnOK();
   
   if (m_bIsModified)
   {
      m_pUpdate->dwFlags |= OBJ_UPDATE_ACL;
      m_pUpdate->bInheritRights = m_bInheritRights;
      m_pUpdate->dwAclSize = m_dwAclSize;
      m_pUpdate->pAccessList = (NXC_ACL_ENTRY *)nx_memdup(m_pAccessList,
                                                          sizeof(NXC_ACL_ENTRY) * m_dwAclSize);
   }
}
