// LastValuesPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "LastValuesPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLastValuesPropDlg dialog


CLastValuesPropDlg::CLastValuesPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLastValuesPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLastValuesPropDlg)
	m_bShowGrid = FALSE;
	m_bRefresh = FALSE;
	m_dwSeconds = 0;
	m_bHideEmpty = FALSE;
	m_bUseMultipliers = FALSE;
	//}}AFX_DATA_INIT
}


void CLastValuesPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLastValuesPropDlg)
	DDX_Check(pDX, IDC_CHECK_GRID, m_bShowGrid);
	DDX_Check(pDX, IDC_CHECK_REFRESH, m_bRefresh);
	DDX_Text(pDX, IDC_EDIT_TIME, m_dwSeconds);
	DDV_MinMaxDWord(pDX, m_dwSeconds, 5, 3600);
	DDX_Check(pDX, IDC_CHECK_HIDE_EMPTY, m_bHideEmpty);
	DDX_Check(pDX, IDC_CHECK_USE_MULTIPLIERS, m_bUseMultipliers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLastValuesPropDlg, CDialog)
	//{{AFX_MSG_MAP(CLastValuesPropDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLastValuesPropDlg message handlers
