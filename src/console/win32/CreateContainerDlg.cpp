// CreateContainerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateContainerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateContainerDlg dialog


CCreateContainerDlg::CCreateContainerDlg(CWnd* pParent)
	: CCreateObjectDlg(CCreateContainerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateContainerDlg)
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
}


void CCreateContainerDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateContainerDlg)
	DDX_Control(pDX, IDC_COMBO_CATEGORY, m_wndCategoryList);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateContainerDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateContainerDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateContainerDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CCreateContainerDlg::OnInitDialog() 
{
   DWORD i;

	CCreateObjectDlg::OnInitDialog();
	
   // Setup list of container categories
   for(i = 0; i < g_pCCList->dwNumElements; i++)
      m_wndCategoryList.AddString(g_pCCList->pElements[i].szName);
   m_wndCategoryList.SelectString(-1, g_pCCList->pElements[0].szName);

   return TRUE;
}
