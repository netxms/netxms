// NodePropsPolling.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePropsPolling.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNodePropsPolling property page

IMPLEMENT_DYNCREATE(CNodePropsPolling, CPropertyPage)

CNodePropsPolling::CNodePropsPolling() : CPropertyPage(CNodePropsPolling::IDD)
{
	//{{AFX_DATA_INIT(CNodePropsPolling)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CNodePropsPolling::~CNodePropsPolling()
{
}

void CNodePropsPolling::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodePropsPolling)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNodePropsPolling, CPropertyPage)
	//{{AFX_MSG_MAP(CNodePropsPolling)
	ON_BN_CLICKED(IDC_SELECT_POLLER, OnSelectPoller)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropsPolling message handlers

//
// WM_INITDIALOG message handler
//

BOOL CNodePropsPolling::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   // Poller node
   if (m_dwPollerNode != 0)
   {
      NXC_OBJECT *pNode;

      pNode = NXCFindObjectById(g_hSession, m_dwPollerNode);
      if (pNode != NULL)
      {
         SetDlgItemText(IDC_EDIT_POLLER, pNode->szName);
      }
      else
      {
         SetDlgItemText(IDC_EDIT_POLLER, _T("<invalid>"));
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_POLLER, _T("<server>"));
   }

   return TRUE;
}


//
// Handler for "select poller node" button
//

void CNodePropsPolling::OnSelectPoller() 
{
   CObjectSelDlg dlg;

   dlg.m_dwAllowedClasses = SCL_NODE;
   dlg.m_bSingleSelection = TRUE;
   dlg.m_bAllowEmptySelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      if (dlg.m_dwNumObjects != 0)
      {
         NXC_OBJECT *pNode;

         m_dwPollerNode = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_dwPollerNode);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_POLLER, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_POLLER, _T("<invalid>"));
         }
      }
      else
      {
         m_dwPollerNode = 0;
         SetDlgItemText(IDC_EDIT_POLLER, _T("<server>"));
      }
      m_pUpdate->dwFlags |= OBJ_UPDATE_POLLER_NODE;
      SetModified();
   }
}


//
// Handler for "OK" button
//

void CNodePropsPolling::OnOK() 
{
	CPropertyPage::OnOK();

   m_pUpdate->dwPollerNode = m_dwPollerNode;
}
