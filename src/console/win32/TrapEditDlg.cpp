// TrapEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapEditDlg.h"
#include <nxsnmp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapEditDlg dialog


CTrapEditDlg::CTrapEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrapEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrapEditDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   memset(&m_trap, 0, sizeof(NXC_TRAP_CFG_ENTRY));
}

CTrapEditDlg::~CTrapEditDlg()
{
   DWORD i;

   safe_free(m_trap.pdwObjectId);
   for(i = 0; i < m_trap.dwNumMaps; i++)
      safe_free(m_trap.pMaps[i].pdwObjectId);
   safe_free(m_trap.pMaps);
}

void CTrapEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrapEditDlg)
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_wndEditDescr);
	DDX_Control(pDX, IDC_EDIT_TRAP, m_wndEditOID);
	DDX_Control(pDX, IDC_EDIT_EVENT, m_wndEditEvent);
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_wndEditMsg);
	DDX_Control(pDX, IDC_LIST_ARGS, m_wndArgList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrapEditDlg, CDialog)
	//{{AFX_MSG_MAP(CTrapEditDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapEditDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CTrapEditDlg::OnInitDialog() 
{
   TCHAR szBuffer[1024];

	CDialog::OnInitDialog();

   m_wndEditDescr.SetWindowText(m_trap.szDescription);
   SNMPConvertOIDToText(m_trap.dwOidLen, m_trap.pdwObjectId, szBuffer, 1024);
   m_wndEditOID.SetWindowText(szBuffer);

	return TRUE;
}
