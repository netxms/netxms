// ConsolePropGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ConsolePropsGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConsolePropsGeneral property page

IMPLEMENT_DYNCREATE(CConsolePropsGeneral, CPropertyPage)

CConsolePropsGeneral::CConsolePropsGeneral() : CPropertyPage(CConsolePropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CConsolePropsGeneral)
	m_bExpandCtrlPanel = FALSE;
	m_bShowGrid = FALSE;
	//}}AFX_DATA_INIT
}

CConsolePropsGeneral::~CConsolePropsGeneral()
{
}

void CConsolePropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConsolePropsGeneral)
	DDX_Check(pDX, IDC_CHECK_EXPAND, m_bExpandCtrlPanel);
	DDX_Check(pDX, IDC_CHECK_GRID, m_bShowGrid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConsolePropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CConsolePropsGeneral)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConsolePropsGeneral message handlers
