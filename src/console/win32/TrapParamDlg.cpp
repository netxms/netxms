// TrapParamDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapParamDlg.h"
#include "MIBBrowserDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapParamDlg dialog


CTrapParamDlg::CTrapParamDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrapParamDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrapParamDlg)
	m_strDescription = _T("");
	m_strOID = _T("");
	//}}AFX_DATA_INIT
}


void CTrapParamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrapParamDlg)
	DDX_Control(pDX, IDC_EDIT_OID, m_wndEditOID);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	DDX_Text(pDX, IDC_EDIT_OID, m_strOID);
	DDV_MaxChars(pDX, m_strOID, 1023);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrapParamDlg, CDialog)
	//{{AFX_MSG_MAP(CTrapParamDlg)
	ON_BN_CLICKED(IDC_SELECT_OID, OnSelectOid)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapParamDlg message handlers


//
// Handler for "Select" button
//

void CTrapParamDlg::OnSelectOid() 
{
   CMIBBrowserDlg dlg;
   TCHAR szBuffer[1024];

   dlg.m_pNode = NULL;
   m_wndEditOID.GetWindowText(szBuffer, 1024);
   dlg.m_strOID = szBuffer;
   if (dlg.DoModal() == IDOK)
   {
      _stprintf(szBuffer, _T(".%lu"), dlg.m_dwInstance);
      dlg.m_strOID += szBuffer;
      m_wndEditOID.SetWindowText(dlg.m_strOID);
   }
}
