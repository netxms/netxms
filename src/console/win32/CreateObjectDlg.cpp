// CreateObjectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateObjectDlg.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateObjectDlg dialog


CCreateObjectDlg::CCreateObjectDlg(int iId, CWnd* pParent /*=NULL*/)
	: CDialog(iId, pParent)
{
	//{{AFX_DATA_INIT(CCreateObjectDlg)
	m_strObjectName = _T("");
	//}}AFX_DATA_INIT
}


void CCreateObjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateObjectDlg)
	DDX_Control(pDX, IDC_STATIC_NAME, m_wndStaticName);
	DDX_Control(pDX, IDC_STATIC_ID, m_wndStaticId);
	DDX_Control(pDX, IDC_ICON_PARENT, m_wndParentIcon);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strObjectName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateObjectDlg, CDialog)
	//{{AFX_MSG_MAP(CCreateObjectDlg)
	ON_BN_CLICKED(IDC_SELECT_PARENT, OnSelectParent)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateObjectDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CCreateObjectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
   UpdateParentInfo();
	return TRUE;
}


//
// Handler for "Select..." button
//

void CCreateObjectDlg::OnSelectParent() 
{
   CObjectSelDlg dlg;

   switch(m_iObjectClass)
   {
      case OBJECT_TEMPLATE:
      case OBJECT_TEMPLATEGROUP:
         dlg.m_dwAllowedClasses = SCL_TEMPLATEROOT | SCL_TEMPLATEGROUP;
         break;
      case OBJECT_VPNCONNECTOR:
         dlg.m_dwAllowedClasses = SCL_NODE;
         break;
      default:
         dlg.m_dwAllowedClasses = SCL_CONTAINER | SCL_SERVICEROOT;
         break;
   }
   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_pParentObject = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
      UpdateParentInfo();
   }
}


//
// Update parent object's information
//

void CCreateObjectDlg::UpdateParentInfo()
{
   if (m_pParentObject != NULL)
   {
      TCHAR szBuffer[16];

      m_wndStaticName.SetWindowText(m_pParentObject->szName);
      _stprintf(szBuffer, _T("ID: %d"), m_pParentObject->dwId);
      m_wndStaticId.SetWindowText(szBuffer);
      m_wndParentIcon.SetIcon(
         g_pObjectNormalImageList->ExtractIcon(
            GetObjectImageIndex(m_pParentObject)));
   }
   else
   {
      m_wndStaticName.SetWindowText(_T(""));
      m_wndStaticId.SetWindowText(_T("<no parent>"));
   }
}
