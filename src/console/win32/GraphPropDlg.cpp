// GraphPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraphPropDlg dialog


CGraphPropDlg::CGraphPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGraphPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGraphPropDlg)
	m_dwRefreshInterval = 0;
	//}}AFX_DATA_INIT
}


void CGraphPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGraphPropDlg)
	DDX_Text(pDX, IDC_EDIT_REFRESH, m_dwRefreshInterval);
	DDV_MinMaxDWord(pDX, m_dwRefreshInterval, 5, 3600);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGraphPropDlg, CDialog)
	//{{AFX_MSG_MAP(CGraphPropDlg)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CB_BACKGROUND, OnCbBackground)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphPropDlg message handlers


//
// WM_CTLCOLOR message handler
//

HBRUSH CGraphPropDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   HBRUSH hbr;

   switch(pWnd->GetDlgCtrlID())
   {
      case IDC_CB_BACKGROUND:
         pDC->SetBkColor(RGB(200,0,0));
         hbr = CreateSolidBrush(RGB(200,0,0));
         break;
      default:
	      hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
         break;
   }
	return hbr;
}


//
// BN_CLICK handlers for color selectors
//

void CGraphPropDlg::OnCbBackground() 
{
}
