// CreateNetMapDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateNetMapDlg.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateNetMapDlg dialog


CCreateNetMapDlg::CCreateNetMapDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateNetMapDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateNetMapDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
	m_dwRootObj = 0;
}


void CCreateNetMapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateNetMapDlg)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateNetMapDlg, CDialog)
	//{{AFX_MSG_MAP(CCreateNetMapDlg)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateNetMapDlg message handlers

void CCreateNetMapDlg::OnButtonSelect() 
{
	CObjectSelDlg dlg;
	NXC_OBJECT *object;

	dlg.m_bSingleSelection = TRUE;
	dlg.m_dwAllowedClasses = SCL_NETWORK | SCL_CONTAINER;
	if (dlg.DoModal() == IDOK)
	{
		m_dwRootObj = dlg.m_pdwObjectList[0];
		object = NXCFindObjectById(g_hSession, m_dwRootObj);
		SetDlgItemText(IDC_EDIT_OBJECT, (object != NULL) ? object->szName : _T("<unknown>"));
	}
}

void CCreateNetMapDlg::OnOK() 
{
	if (m_dwRootObj == 0)
	{
		MessageBox(_T("You must select root object!"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	CDialog::OnOK();
}
