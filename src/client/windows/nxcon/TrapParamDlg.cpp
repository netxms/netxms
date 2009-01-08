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
	m_dwPos = 0;
	m_nType = -1;
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
	DDX_Text(pDX, IDC_EDIT_POS, m_dwPos);
	DDV_MinMaxDWord(pDX, m_dwPos, 1, 255);
	DDX_Radio(pDX, IDC_RADIO_OID, m_nType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrapParamDlg, CDialog)
	//{{AFX_MSG_MAP(CTrapParamDlg)
	ON_BN_CLICKED(IDC_SELECT_OID, OnSelectOid)
	ON_BN_CLICKED(IDC_RADIO_OID, OnRadioOid)
	ON_BN_CLICKED(IDC_RADIO_POS, OnRadioPos)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapParamDlg message handlers

BOOL CTrapParamDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   OnTypeChange();
	
	return TRUE;
}


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
      _sntprintf_s(szBuffer, 1024, _TRUNCATE, _T(".%lu"), dlg.m_dwInstance);
      dlg.m_strOID += szBuffer;
      m_wndEditOID.SetWindowText(dlg.m_strOID);
   }
}


//
// Handle varbind matching type change
//

void CTrapParamDlg::OnTypeChange()
{
   EnableDlgItem(this, IDC_EDIT_OID, m_nType == 0);
   EnableDlgItem(this, IDC_SELECT_OID, m_nType == 0);
   EnableDlgItem(this, IDC_EDIT_POS, m_nType == 1);
}


//
// type selection radio buttons handlers
//

void CTrapParamDlg::OnRadioOid() 
{
   m_nType = 0;
   OnTypeChange();
}

void CTrapParamDlg::OnRadioPos() 
{
   m_nType = 1;
   OnTypeChange();
}


//
// "OK" button handler
//

void CTrapParamDlg::OnOK() 
{
   if (m_nType == 0)
      SetDlgItemText(IDC_EDIT_POS, _T("1"));
	
	CDialog::OnOK();
}
