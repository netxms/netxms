// WaitView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "WaitView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BOX_COUNT          10
#define BOX_SIZE           10
#define BOX_MARGIN         25
#define MAX_STAGES         10


//
// Static data
//

static COLORREF m_rgbBoxColors[MAX_STAGES] =
{
   RGB(53, 128, 21),
   RGB(61, 147, 23),
   RGB(68, 165, 27),
   RGB(75, 183, 30),
   RGB(83, 201, 33),
   RGB(90, 220, 35),
   RGB(104, 222, 54),
   RGB(118, 225, 72),
   RGB(132, 228, 90),
   RGB(146, 232, 108)
};


/////////////////////////////////////////////////////////////////////////////
// CWaitView

CWaitView::CWaitView()
{
   m_iActiveBox = 0;
   m_iStage = 0;
   m_iStageDir = 1;
}

CWaitView::~CWaitView()
{
}


BEGIN_MESSAGE_MAP(CWaitView, CWnd)
	//{{AFX_MSG_MAP(CWaitView)
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWaitView message handlers

BOOL CWaitView::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         ::LoadCursor(NULL, IDC_WAIT), 
                                         GetSysColorBrush(COLOR_3DSHADOW), 
                                         NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_DESTROY message handler
//

void CWaitView::OnDestroy() 
{
   if (m_nTimer != 0)
      KillTimer(m_nTimer);
	CWnd::OnDestroy();
}


//
// WM_PAINT message handler
//

void CWaitView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   RECT rect;
   int i, x, y;

   GetClientRect(&rect);

   // Draw boxes
   y = (rect.bottom - BOX_SIZE) / 2;
   x = (rect.right - (BOX_COUNT * BOX_SIZE + (BOX_COUNT - 1) * BOX_MARGIN)) / 2;
   for(i = 0; i < BOX_COUNT; i++)
   {
      dc.Draw3dRect(x, y, BOX_SIZE, BOX_SIZE, RGB(46, 109, 18), RGB(159, 234, 128));
      dc.FillSolidRect(x + 1, y + 1, BOX_SIZE - 2, BOX_SIZE - 2,
                       (i == m_iActiveBox) ? m_rgbBoxColors[m_iStage] : m_rgbBoxColors[0]);
      x += BOX_SIZE + BOX_MARGIN;
   }
}


//
// WM_TIMER message handler
//

void CWaitView::OnTimer(UINT nIDEvent) 
{
   m_iStage += m_iStageDir;
   if (m_iStage == MAX_STAGES - 1)
   {
      m_iStageDir = -1;
   }
   else if (m_iStage == 0)
   {
      m_iStageDir = 1;
      m_iActiveBox++;
      if (m_iActiveBox == BOX_COUNT)
         m_iActiveBox = 0;
   }
   InvalidateRect(NULL, FALSE);
}


//
// Start wait picture
//

void CWaitView::Start()
{
   if (m_nTimer != 0)
      KillTimer(m_nTimer);

   m_iActiveBox = 0;
   m_iStage = 0;
   m_iStageDir = 1;

   m_nTimer = SetTimer(1, 100, NULL);
}


//
// Stop wait picture
//

void CWaitView::Stop()
{
   if (m_nTimer != 0)
   {
      KillTimer(m_nTimer);
      m_nTimer = 0;
   }
}
