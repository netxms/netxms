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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphPropDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CGraphPropDlg::OnInitDialog() 
{
   int i;
   static int piItemList[MAX_GRAPH_ITEMS] = 
      { IDC_CB_ITEM1, IDC_CB_ITEM2, IDC_CB_ITEM3, IDC_CB_ITEM4,
        IDC_CB_ITEM5, IDC_CB_ITEM6, IDC_CB_ITEM7, IDC_CB_ITEM8,
        IDC_CB_ITEM9, IDC_CB_ITEM10, IDC_CB_ITEM11, IDC_CB_ITEM12,
        IDC_CB_ITEM13, IDC_CB_ITEM14, IDC_CB_ITEM15, IDC_CB_ITEM16 };

	CDialog::OnInitDialog();

   // Init color selectors
   m_wndCSBackground.m_rgbColor = m_rgbBackground;
   m_wndCSText.m_rgbColor = m_rgbText;
   m_wndCSAxisLines.m_rgbColor = m_rgbAxisLines;
   m_wndCSGridLines.m_rgbColor = m_rgbGridLines;
   m_wndCSLabelBkgnd.m_rgbColor = m_rgbLabelBkgnd;
   m_wndCSLabelText.m_rgbColor = m_rgbLabelText;

   // Subclass color selectors
   m_wndCSBackground.SubclassDlgItem(IDC_CB_BACKGROUND, this);
   m_wndCSText.SubclassDlgItem(IDC_CB_TEXT, this);
   m_wndCSAxisLines.SubclassDlgItem(IDC_CB_AXIS, this);
   m_wndCSGridLines.SubclassDlgItem(IDC_CB_GRID, this);
   m_wndCSLabelBkgnd.SubclassDlgItem(IDC_CB_LABELBK, this);
   m_wndCSLabelText.SubclassDlgItem(IDC_CB_LABEL, this);
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
   {
      m_pwndCSItem[i].m_rgbColor = m_rgbItems[i];
      m_pwndCSItem[i].SubclassDlgItem(piItemList[i], this);
   }
	
	return TRUE;
}


//
// "OK" button handler
//

void CGraphPropDlg::OnOK() 
{
   int i;

	CDialog::OnOK();

   m_rgbBackground = m_wndCSBackground.m_rgbColor;
   m_rgbText = m_wndCSText.m_rgbColor;
   m_rgbAxisLines = m_wndCSAxisLines.m_rgbColor;
   m_rgbGridLines = m_wndCSGridLines.m_rgbColor;
   m_rgbLabelText = m_wndCSLabelText.m_rgbColor;
   m_rgbLabelBkgnd = m_wndCSLabelBkgnd.m_rgbColor;
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
      m_rgbItems[i] = m_pwndCSItem[i].m_rgbColor;
}
