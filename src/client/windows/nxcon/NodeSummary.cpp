// NodeSummary.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodeSummary.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define X_MARGIN     5
#define Y_MARGIN     5

/////////////////////////////////////////////////////////////////////////////
// CNodeSummary

CNodeSummary::CNodeSummary()
{
   memset(m_dwNodeStats, 0, sizeof(DWORD) * OBJECT_STATUS_COUNT);
   m_dwTotalNodes = 0;
}

CNodeSummary::~CNodeSummary()
{
}


BEGIN_MESSAGE_MAP(CNodeSummary, CWnd)
	//{{AFX_MSG_MAP(CNodeSummary)
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNodeSummary message handlers


//
// Custom create method
//

BOOL CNodeSummary::Create(DWORD dwStyle, const RECT &rect, CWnd *pParentWnd, UINT nID, CCreateContext *pContext)
{
	return CWnd::Create(NULL, _T("Node Status Summary"), dwStyle, rect, pParentWnd, nID, pContext);
}


//
// PreCreateWindow()
//

BOOL CNodeSummary::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         CreateSolidBrush(RGB(255, 255, 255)), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CNodeSummary::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	HDC hdc = ::GetDC(m_hWnd);
   m_fontTitle.CreateFont(-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
	::ReleaseDC(m_hWnd, hdc);
	UpdateStatus();

	return 0;
}


//
// WM_PAINT message handler
//

void CNodeSummary::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   int i, x, y = Y_MARGIN, iTabStop, iLineHeight, iCounterWidth;
   int iChartTop, iChartBottom;
   RECT rect;
   CSize size;
   TCHAR szBuffer[256];

   GetClientRect(&rect);
   x = X_MARGIN + X_MARGIN / 2;
	
   // Draw title
   dc.SelectObject(&m_fontTitle);
   dc.TextOut(x, Y_MARGIN, _T("Node Status Summary"), 19);
   y += dc.GetTextExtent(_T("X"), 1).cy + 2;
   dc.MoveTo(X_MARGIN, y);
   dc.LineTo(rect.right - Y_MARGIN, y);
   y += Y_MARGIN * 2;
   iChartTop = y;

   // Draw legend
   dc.SelectObject(&m_fontNormal);
   size = dc.GetTextExtent(_T("0000"), 4);
   iLineHeight = size.cy;
   iCounterWidth = size.cx;
   iTabStop = 100;
   for(i = 0; i < OBJECT_STATUS_COUNT; i++, y+= 20)
   {
      dc.FillSolidRect(x, y, iLineHeight, iLineHeight, g_statusColorTable[i]);
      dc.Draw3dRect(x, y, iLineHeight, iLineHeight, RGB(0, 0, 0), RGB(0, 0, 0));
      dc.SetBkColor(RGB(255, 255, 255));
      _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%s:\t%d"), g_szStatusTextSmall[i], m_dwNodeStats[i]);
      dc.TabbedTextOut(x + iLineHeight + 5, y, szBuffer, (int)_tcslen(szBuffer), 1, 
                       &iTabStop, x + iLineHeight + 5);
   }
   iChartBottom = y;

   // Total
   dc.MoveTo(X_MARGIN, y);
   dc.LineTo(x + iLineHeight + iTabStop + 30, y);
   y += 5;
   _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("Total:\t%d"), m_dwTotalNodes);
   dc.TabbedTextOut(x + iLineHeight + 5, y, szBuffer, (int)_tcslen(szBuffer), 1, 
                    &iTabStop, x + iLineHeight + 5);

   // Pie chart
   rect.left = 200;
   rect.top = iChartTop;
   rect.bottom = iChartBottom;
   rect.right = rect.left + (rect.bottom - rect.top);
   DrawPieChart(dc, &rect, OBJECT_STATUS_COUNT, m_dwNodeStats, g_statusColorTable);
}


//
// Update status information
//

void CNodeSummary::UpdateStatus()
{
   NXC_OBJECT_INDEX *pIndex;
   UINT32 i, dwNumObjects;
   
   memset(m_dwNodeStats, 0, sizeof(DWORD) * OBJECT_STATUS_COUNT);
   m_dwTotalNodes = 0;
   NXCLockObjectIndex(g_hSession);
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(g_hSession, &dwNumObjects);
   for(i = 0; i < dwNumObjects; i++)
      if (pIndex[i].pObject->iClass == OBJECT_NODE)
      {
         m_dwNodeStats[pIndex[i].pObject->iStatus]++;
         m_dwTotalNodes++;
      }
   NXCUnlockObjectIndex(g_hSession);
}


//
// Refresh window
//

void CNodeSummary::Refresh()
{
	UpdateStatus();
   Invalidate();
   UpdateWindow();
}
