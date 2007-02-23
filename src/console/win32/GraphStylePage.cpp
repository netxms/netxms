// GraphStylePage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphStylePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraphStylePage property page

IMPLEMENT_DYNCREATE(CGraphStylePage, CPropertyPage)

CGraphStylePage::CGraphStylePage() : CPropertyPage(CGraphStylePage::IDD)
{
	//{{AFX_DATA_INIT(CGraphStylePage)
	//}}AFX_DATA_INIT
}

CGraphStylePage::~CGraphStylePage()
{
}

void CGraphStylePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGraphStylePage)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LIST_ITEMS, m_wndListCtrl);
}


BEGIN_MESSAGE_MAP(CGraphStylePage, CPropertyPage)
	//{{AFX_MSG_MAP(CGraphStylePage)
	ON_WM_DRAWITEM()
	//}}AFX_MSG_MAP
	ON_MESSAGE(CLN_SETITEMS, OnComboListSetItems)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphStylePage message handlers

BOOL CGraphStylePage::OnInitDialog() 
{
	int i, nItem;
	TCHAR szBuffer[16];

	CPropertyPage::OnInitDialog();

	// Setup list control
	m_wndListCtrl.InsertColumn(0, _T("Pos"), LVCFMT_LEFT, 45);
	m_wndListCtrl.InsertColumn(1, _T("Color"), LVCFMT_LEFT, 60);
	m_wndListCtrl.InsertColumn(2, _T("Style"), LVCFMT_LEFT, 70);
	m_wndListCtrl.InsertColumn(3, _T("Width"), LVCFMT_LEFT, 70);
	m_wndListCtrl.InsertColumn(4, _T("Thr."), LVCFMT_LEFT, 50);
	m_wndListCtrl.InsertColumn(5, _T("Avg."), LVCFMT_LEFT, 50);
	m_wndListCtrl.InsertColumn(6, _T("Trend"), LVCFMT_LEFT, 50);

	m_wndListCtrl.SetReadOnlyColumn(0);
	m_wndListCtrl.SetColorPickerColumn(1);
	m_wndListCtrl.SetComboColumn(2);
	m_wndListCtrl.SetComboColumn(3);
	m_wndListCtrl.SetComboColumn(4);
	m_wndListCtrl.SetComboColumn(5);
	m_wndListCtrl.SetComboColumn(6);

	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// Fill list control with data
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
	{
		_stprintf(szBuffer, _T("%d"), i + 1);
		nItem = m_wndListCtrl.InsertItem(i, szBuffer);
		if (nItem != -1)
		{
			_stprintf(szBuffer, _T("%08X"), m_styles[i].rgbColor);
			m_wndListCtrl.SetItemText(nItem, 1, szBuffer);
			m_wndListCtrl.SetItemText(nItem, 2, g_szGraphType[m_styles[i].nType]);
			if (m_styles[i].nLineWidth == 0)
			{
				m_wndListCtrl.SetItemText(nItem, 3, _T("Automatic"));
			}
			else
			{
				_stprintf(szBuffer, _T("%d"), m_styles[i].nLineWidth);
				m_wndListCtrl.SetItemText(nItem, 3, szBuffer);
			}
			m_wndListCtrl.SetItemText(nItem, 4, m_styles[i].bShowThresholds ? _T("Yes") : _T("No"));
			m_wndListCtrl.SetItemText(nItem, 5, m_styles[i].bShowAverage ? _T("Yes") : _T("No"));
			m_wndListCtrl.SetItemText(nItem, 6, m_styles[i].bShowTrend ? _T("Yes") : _T("No"));
		}
	}

	return TRUE;
}


//
// Handler for CLN_SETITEMS message
//

LRESULT CGraphStylePage::OnComboListSetItems(WPARAM wParam, LPARAM lParam)
{
	CStringList *pComboList = reinterpret_cast<CStringList *>(lParam);

	pComboList->RemoveAll();
	switch(wParam)
	{
		case 2:
			pComboList->AddTail("Line");
			pComboList->AddTail("Area");
			pComboList->AddTail("Stacked");
			break;
		case 3:
			pComboList->AddTail("Automatic");
			pComboList->AddTail("1");
			pComboList->AddTail("2");
			pComboList->AddTail("3");
			pComboList->AddTail("4");
			pComboList->AddTail("5");
			break;
		case 4:
		case 5:
		case 6:
			pComboList->AddTail("Yes");
			pComboList->AddTail("No");
			break;
		default:
			break;
	}
	
	return TRUE;
}


//
// WM_DRAWITEM message handler
//

void CGraphStylePage::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CRect rcSubItem;
	TCHAR szBuffer[256];
	HBRUSH hBrush;
	HPEN hPen, hOldPen;
	COLORREF rgbTextColor;

	if (nIDCtl != IDC_LIST_ITEMS)
		return;

	rgbTextColor = GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
	FillRect(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem,
	         GetSysColorBrush((lpDrawItemStruct->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW));
	SetTextColor(lpDrawItemStruct->hDC, rgbTextColor);

	DrawTextCell(0, lpDrawItemStruct);
	DrawTextCell(2, lpDrawItemStruct);
	DrawTextCell(4, lpDrawItemStruct);
	DrawTextCell(5, lpDrawItemStruct);
	DrawTextCell(6, lpDrawItemStruct);

	// Draw "color" column
	m_wndListCtrl.GetSubItemRect(lpDrawItemStruct->itemID, 1, LVIR_BOUNDS, rcSubItem);
	InflateRect(&rcSubItem, -8, -4);
	m_wndListCtrl.GetItemText(lpDrawItemStruct->itemID, 1, szBuffer, 256);
	hBrush = CreateSolidBrush(_tcstoul(szBuffer, NULL, 16));
	FillRect(lpDrawItemStruct->hDC, &rcSubItem, hBrush);
	InflateRect(&rcSubItem, 1, 1);
	hPen = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWFRAME));
	hOldPen = (HPEN)SelectObject(lpDrawItemStruct->hDC, hPen);
	hBrush = (HBRUSH)SelectObject(lpDrawItemStruct->hDC, GetStockObject(NULL_BRUSH));
	Rectangle(lpDrawItemStruct->hDC, rcSubItem.left, rcSubItem.top, rcSubItem.right, rcSubItem.bottom);
	SelectObject(lpDrawItemStruct->hDC, hOldPen);
	SelectObject(lpDrawItemStruct->hDC, hBrush);
	DeleteObject(hPen);
	DeleteObject(hBrush);

	// Draw "line width" column
	m_wndListCtrl.GetSubItemRect(lpDrawItemStruct->itemID, 3, LVIR_BOUNDS, rcSubItem);
	InflateRect(&rcSubItem, -4, 0);
	m_wndListCtrl.GetItemText(lpDrawItemStruct->itemID, 3, szBuffer, 256);
	if (_tcsicmp(szBuffer, _T("Automatic")))
	{
		int y, w;
		LOGBRUSH lb;

		w = _tcstol(szBuffer, NULL, 10);
		lb.lbStyle = BS_SOLID;
		lb.lbColor = rgbTextColor;
		hPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, w, &lb, 0, NULL);
		hOldPen = (HPEN)SelectObject(lpDrawItemStruct->hDC, hPen);
		y = rcSubItem.top + (rcSubItem.bottom - rcSubItem.top) / 2;
		MoveToEx(lpDrawItemStruct->hDC, rcSubItem.left, y, NULL);
		LineTo(lpDrawItemStruct->hDC, rcSubItem.left + 20, y);
		SelectObject(lpDrawItemStruct->hDC, hOldPen);
		DeleteObject(hPen);
		rcSubItem.left += 24;
	}
	DrawText(lpDrawItemStruct->hDC, szBuffer, _tcslen(szBuffer), &rcSubItem, DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
}


//
// Draw simple text cell
//

void CGraphStylePage::DrawTextCell(int nSubItem, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CRect rcSubItem;
	TCHAR szBuffer[256];

	m_wndListCtrl.GetSubItemRect(lpDrawItemStruct->itemID, nSubItem, LVIR_BOUNDS, rcSubItem);
	InflateRect(&rcSubItem, -4, 0);
	m_wndListCtrl.GetItemText(lpDrawItemStruct->itemID, nSubItem, szBuffer, 256);
	DrawText(lpDrawItemStruct->hDC, szBuffer, _tcslen(szBuffer), &rcSubItem, DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
}


//
// OK button handler
//

void CGraphStylePage::OnOK() 
{
	int i, j;
	TCHAR szBuffer[256];

	for(i = 0; i < MAX_GRAPH_ITEMS; i++)
	{
		// Color
		m_wndListCtrl.GetItemText(i, 1, szBuffer, 256);
		m_styles[i].rgbColor = _tcstoul(szBuffer, NULL, 16);

		// Type
		m_wndListCtrl.GetItemText(i, 2, szBuffer, 256);
		for(j = 0; g_szGraphType[j] != NULL; j++)
			if (!_tcsicmp(szBuffer, g_szGraphType[j]))
			{
				m_styles[i].nType = j;
				break;
			}

		// Line width
		m_wndListCtrl.GetItemText(i, 3, szBuffer, 256);
		if (!_tcsicmp(szBuffer, _T("Automatic")))
			m_styles[i].nLineWidth = 0;
		else
			m_styles[i].nLineWidth = _tcstol(szBuffer, NULL, 10);

		// Show thresholds
		m_wndListCtrl.GetItemText(i, 4, szBuffer, 256);
		m_styles[i].bShowThresholds = !_tcsicmp(szBuffer, _T("Yes"));

		// Show average
		m_wndListCtrl.GetItemText(i, 5, szBuffer, 256);
		m_styles[i].bShowAverage = !_tcsicmp(szBuffer, _T("Yes"));

		// Show trend
		m_wndListCtrl.GetItemText(i, 6, szBuffer, 256);
		m_styles[i].bShowTrend = !_tcsicmp(szBuffer, _T("Yes"));
	}

	CPropertyPage::OnOK();
}
