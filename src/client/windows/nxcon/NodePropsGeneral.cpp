// NodePropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePropsGeneral.h"
#include "ObjectPropSheet.h"
#include "ObjectSelDlg.h"
#include <nxsnmp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CNodePropsGeneral property page

IMPLEMENT_DYNCREATE(CNodePropsGeneral, CPropertyPage)

CNodePropsGeneral::CNodePropsGeneral() : CPropertyPage(CNodePropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CNodePropsGeneral)
	m_dwObjectId = 0;
	m_strName = _T("");
	m_strOID = _T("");
	//}}AFX_DATA_INIT
}

CNodePropsGeneral::~CNodePropsGeneral()
{
}

void CNodePropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodePropsGeneral)
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	DDX_Text(pDX, IDC_EDIT_OID, m_strOID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNodePropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CNodePropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_BN_CLICKED(IDC_SELECT_IP, OnSelectIp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropsGeneral message handlers

BOOL CNodePropsGeneral::OnInitDialog() 
{
   TCHAR szBuffer[16];

	CPropertyPage::OnInitDialog();
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
   SetDlgItemText(IDC_EDIT_PRIMARY_IP, IpToStr(m_dwIpAddr, szBuffer));
   return TRUE;
}


//
// Handlers for changed controls
//

void CNodePropsGeneral::OnChangeEditName() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}


//
// Handler for PSN_OK message
//

void CNodePropsGeneral::OnOK() 
{
	CPropertyPage::OnOK();

   // Set fields in update structure
   m_pUpdate->pszName = (TCHAR *)((LPCTSTR)m_strName);
   m_pUpdate->dwIpAddr = m_dwIpAddr;
}


//
// Select primary IP address for node
//

void CNodePropsGeneral::OnSelectIp() 
{
   CObjectSelDlg dlg;
   NXC_OBJECT *pObject;

   dlg.m_bSelectAddress = TRUE;
   dlg.m_bSingleSelection = TRUE;
   dlg.m_bAllowEmptySelection = FALSE;
   dlg.m_dwParentObject = m_dwObjectId;
   if (dlg.DoModal() == IDOK)
   {
      pObject = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
      if (pObject != NULL)
      {
         TCHAR szBuffer[16];

         m_dwIpAddr = pObject->dwIpAddr;
         SetDlgItemText(IDC_EDIT_PRIMARY_IP, IpToStr(m_dwIpAddr, szBuffer));
      }
      m_pUpdate->qwFlags |= OBJ_UPDATE_IP_ADDR;
      SetModified();
   }
}
