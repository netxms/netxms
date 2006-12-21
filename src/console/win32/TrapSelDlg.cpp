// TrapSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapSelDlg dialog


CTrapSelDlg::CTrapSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrapSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrapSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pdwTrapList = NULL;
	m_dwNumTraps = 0;
	m_dwTrapCfgSize = 0;
}

CTrapSelDlg::~CTrapSelDlg()
{
	safe_free(m_pdwTrapList);
}

void CTrapSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrapSelDlg)
	DDX_Control(pDX, IDC_LIST_TRAPS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrapSelDlg, CDialog)
	//{{AFX_MSG_MAP(CTrapSelDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapSelDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CTrapSelDlg::OnInitDialog() 
{
	DWORD i;
	int nItem;
	TCHAR szBuffer[1024];

	CDialog::OnInitDialog();
	
	m_wndListCtrl.InsertColumn(0, _T("Trap OID"), LVCFMT_LEFT, 150);
	m_wndListCtrl.InsertColumn(1, _T("Event"), LVCFMT_LEFT, 100);
	m_wndListCtrl.InsertColumn(2, _T("Description"), LVCFMT_LEFT, 200);

	for(i = 0; i < m_dwTrapCfgSize; i++)
	{
		SNMPConvertOIDToText(m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId, szBuffer, 1024);
		nItem = m_wndListCtrl.InsertItem(i, szBuffer, -1);
		if (nItem != -1)
		{
			m_wndListCtrl.SetItemText(nItem, 1, NXCGetEventName(g_hSession, m_pTrapCfg[i].dwEventCode));
			m_wndListCtrl.SetItemText(nItem, 2, m_pTrapCfg[i].szDescription);
			m_wndListCtrl.SetItemData(nItem, i);
		}
	}
	
	return TRUE;
}
