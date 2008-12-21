// MapLinkPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapLinkPropDlg.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapLinkPropDlg dialog


CMapLinkPropDlg::CMapLinkPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMapLinkPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapLinkPropDlg)
	m_strPort1 = _T("");
	m_strPort2 = _T("");
	//}}AFX_DATA_INIT

	m_dwObject1 = 0;
	m_dwObject2 = 0;
}


void CMapLinkPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapLinkPropDlg)
	DDX_Text(pDX, IDC_EDIT_PORT1, m_strPort1);
	DDV_MaxChars(pDX, m_strPort1, 63);
	DDX_Text(pDX, IDC_EDIT_PORT2, m_strPort2);
	DDV_MaxChars(pDX, m_strPort2, 63);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapLinkPropDlg, CDialog)
	//{{AFX_MSG_MAP(CMapLinkPropDlg)
	ON_BN_CLICKED(IDC_SELECT_OBJECT1, OnSelectObject1)
	ON_BN_CLICKED(IDC_SELECT_OBJECT2, OnSelectObject2)
	ON_BN_CLICKED(IDC_SELECT_PORT1, OnSelectPort1)
	ON_BN_CLICKED(IDC_SELECT_PORT2, OnSelectPort2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapLinkPropDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CMapLinkPropDlg::OnInitDialog() 
{
	NXC_OBJECT *object;

	CDialog::OnInitDialog();
	
	if (m_dwObject1 != 0)
	{
		object = NXCFindObjectById(g_hSession, m_dwObject1);
		if (object != NULL)
		{
			SetDlgItemText(IDC_EDIT_OBJECT1, object->szName);
		}
		else
		{
			m_dwObject1 = 0;
		}
	}

	if (m_dwObject2 != 0)
	{
		object = NXCFindObjectById(g_hSession, m_dwObject2);
		if (object != NULL)
		{
			SetDlgItemText(IDC_EDIT_OBJECT2, object->szName);
		}
		else
		{
			m_dwObject2 = 0;
		}
	}
	
	return TRUE;
}


//
// OK button handler
//

void CMapLinkPropDlg::OnOK() 
{
	if ((m_dwObject1 == 0) || (m_dwObject2 == 0))
		MessageBox(_T("You should select two objects to create a link"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
	else
		CDialog::OnOK();
}


//
// Object selection handlers
//

void CMapLinkPropDlg::OnSelectObject1() 
{
	CObjectSelDlg dlg;
	NXC_OBJECT *object;

	dlg.m_bAllowEmptySelection = FALSE;
	dlg.m_bSingleSelection = TRUE;
	dlg.m_dwParentObject = m_dwParentObject;
	dlg.m_dwAllowedClasses = SCL_NODE | SCL_SUBNET | SCL_CONTAINER;
	if (dlg.DoModal() == IDOK)
	{
		object = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
		if (object != NULL)
		{
			m_dwObject1 = dlg.m_pdwObjectList[0];
			SetDlgItemText(IDC_EDIT_OBJECT1, object->szName);
		}
	}
}

void CMapLinkPropDlg::OnSelectObject2() 
{
	CObjectSelDlg dlg;
	NXC_OBJECT *object;

	dlg.m_bAllowEmptySelection = FALSE;
	dlg.m_bSingleSelection = TRUE;
	dlg.m_dwParentObject = m_dwParentObject;
	dlg.m_dwAllowedClasses = SCL_NODE | SCL_SUBNET | SCL_CONTAINER;
	if (dlg.DoModal() == IDOK)
	{
		object = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
		if (object != NULL)
		{
			m_dwObject2 = dlg.m_pdwObjectList[0];
			SetDlgItemText(IDC_EDIT_OBJECT2, object->szName);
		}
	}
}


//
// Interface selection handlers
//

void CMapLinkPropDlg::OnSelectPort1() 
{
	CObjectSelDlg dlg;
	NXC_OBJECT *object;

	if (m_dwObject1 != 0)
	{
		dlg.m_bAllowEmptySelection = FALSE;
		dlg.m_bSingleSelection = TRUE;
		dlg.m_bSelectAddress = TRUE;
		dlg.m_dwParentObject = m_dwObject1;
		if (dlg.DoModal() == IDOK)
		{
			object = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
			if (object != NULL)
			{
				SetDlgItemText(IDC_EDIT_PORT1, object->szName);
			}
		}
	}
	else
	{
		MessageBox(_T("You must select object fist"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
	}
}

void CMapLinkPropDlg::OnSelectPort2() 
{
	CObjectSelDlg dlg;
	NXC_OBJECT *object;

	if (m_dwObject2 != 0)
	{
		dlg.m_bAllowEmptySelection = FALSE;
		dlg.m_bSingleSelection = TRUE;
		dlg.m_bSelectAddress = TRUE;
		dlg.m_dwParentObject = m_dwObject2;
		if (dlg.DoModal() == IDOK)
		{
			object = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
			if (object != NULL)
			{
				SetDlgItemText(IDC_EDIT_PORT2, object->szName);
			}
		}
	}
	else
	{
		MessageBox(_T("You must select object fist"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
	}
}
