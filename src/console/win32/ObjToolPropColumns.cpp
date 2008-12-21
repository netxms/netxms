// ObjToolPropColumns.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjToolPropColumns.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns property page

IMPLEMENT_DYNCREATE(CObjToolPropColumns, CPropertyPage)

CObjToolPropColumns::CObjToolPropColumns() : CPropertyPage(CObjToolPropColumns::IDD)
{
	//{{AFX_DATA_INIT(CObjToolPropColumns)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_pColumnList = NULL;
}

CObjToolPropColumns::~CObjToolPropColumns()
{
   safe_free(m_pColumnList);
}

void CObjToolPropColumns::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjToolPropColumns)
	DDX_Control(pDX, IDC_LIST_COLUMNS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjToolPropColumns, CPropertyPage)
	//{{AFX_MSG_MAP(CObjToolPropColumns)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjToolPropColumns::OnInitDialog() 
{
   RECT rect;
   DWORD i;
   int iItem;

	CPropertyPage::OnInitDialog();

   // Setup list control
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(1, _T("Format"), LVCFMT_LEFT, 80);
   if (m_iToolType == TOOL_TYPE_TABLE_AGENT)
      m_wndListCtrl.InsertColumn(2, _T("SubStr"), LVCFMT_LEFT,
                                 rect.right - GetSystemMetrics(SM_CXVSCROLL) - 180);
   else
      m_wndListCtrl.InsertColumn(2, _T("OID"), LVCFMT_LEFT,
                                 rect.right - GetSystemMetrics(SM_CXVSCROLL) - 180);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Fill list control with data
   for(i = 0; i < m_dwNumColumns; i++)
   {
      iItem = m_wndListCtrl.InsertItem(i, m_pColumnList[i].szName);
      if (iItem != -1)
      {
         m_wndListCtrl.SetItemData(iItem, i);
         m_wndListCtrl.SetItemText(iItem, 1, g_szToolColFmt[m_pColumnList[i].nFormat]);
         if (m_iToolType == TOOL_TYPE_TABLE_AGENT)
         {
            TCHAR szBuffer[32];

            _sntprintf(szBuffer, 32, _T("%d"), m_pColumnList[i].nSubstr);
            m_wndListCtrl.SetItemText(iItem, 2, szBuffer);
         }
         else
         {
            m_wndListCtrl.SetItemText(iItem, 2, m_pColumnList[i].szOID);
         }
      }
   }
	
	return TRUE;
}
