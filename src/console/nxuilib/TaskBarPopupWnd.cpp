// TaskBarPopupWnd.cpp : implementation file
//

#include "stdafx.h"
#include "nxuilib.h"
#include "TaskBarPopupWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMER_OPEN      101
#define TIMER_STAY      102
#define TIMER_CLOSE     103

#define MARGIN          5


/////////////////////////////////////////////////////////////////////////////
// CTaskBarPopupWnd

CTaskBarPopupWnd::CTaskBarPopupWnd()
{
   m_nStep = 15;                 // change size by 15 pixels when animating
   m_dwAnimationInterval = 40;   // 40 milliseconds between animation steps
   m_dwStayTime = 5000;          // Stay open fo 5 seconds
}

CTaskBarPopupWnd::~CTaskBarPopupWnd()
{
}


BEGIN_MESSAGE_MAP(CTaskBarPopupWnd, CWnd)
	//{{AFX_MSG_MAP(CTaskBarPopupWnd)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTaskBarPopupWnd message handlers

BOOL CTaskBarPopupWnd::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS, LoadCursor(NULL, IDC_HAND), NULL, NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CTaskBarPopupWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   CDC *pdc;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create bitmap for window content and do initial drawing
   pdc = GetDC();
   m_bmpContent.CreateCompatibleBitmap(pdc, m_nWidth, m_nHeight);
   ReleaseDC(pdc);
   DrawOnBitmap();

   ShowWindow(SW_SHOWNA);
   SetTimer(TIMER_OPEN, m_dwAnimationInterval, NULL);
	
	return 0;
}


//
// WM_TIMER message handler
//

void CTaskBarPopupWnd::OnTimer(UINT nIDEvent) 
{
   int nWidth, nHeight;

   nWidth = m_rcCurrSize.right - m_rcCurrSize.left;
   nHeight = m_rcCurrSize.bottom - m_rcCurrSize.top;
   switch(nIDEvent)
   {
      case TIMER_OPEN:
         if (nWidth < m_nWidth)
         {
            if (m_nIncX < 0)
            {
               m_rcCurrSize.left -= min(-m_nIncX, m_nWidth - nWidth);
            }
            else
            {
               m_rcCurrSize.right += min(m_nIncX, m_nWidth - nWidth);
            }
         }
         if (nHeight < m_nHeight)
         {
            if (m_nIncY < 0)
            {
               m_rcCurrSize.top -= min(-m_nIncY, m_nHeight - nHeight);
            }
            else
            {
               m_rcCurrSize.bottom += min(m_nIncY, m_nHeight - nHeight);
            }
         }

         // Change window size and repaint
         SetWindowPos(NULL, m_rcCurrSize.left, m_rcCurrSize.top,
                      m_rcCurrSize.right - m_rcCurrSize.left,
                      m_rcCurrSize.bottom - m_rcCurrSize.top,
                      SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOCOPYBITS | SWP_NOACTIVATE);

         if ((nWidth >= m_nWidth) && (nHeight >= m_nHeight))
         {
            KillTimer(TIMER_OPEN);
            SetTimer(TIMER_STAY, m_dwStayTime, NULL);
         }
         break;
      case TIMER_STAY:
         KillTimer(TIMER_STAY);
         SetTimer(TIMER_CLOSE, m_dwAnimationInterval, NULL);
         break;
      case TIMER_CLOSE:
         if ((m_nIncX != 0) && (nWidth > 0))
         {
            if (m_nIncX < 0)
            {
               m_rcCurrSize.left += min(-m_nIncX, nWidth);
            }
            else
            {
               m_rcCurrSize.right -= min(m_nIncX, nWidth);
            }
         }
         if ((m_nIncY != 0) && (nHeight > 0))
         {
            if (m_nIncY < 0)
            {
               m_rcCurrSize.top += min(-m_nIncY, nHeight);
            }
            else
            {
               m_rcCurrSize.bottom -= min(m_nIncY, nHeight);
            }
         }

         // Change window size and repaint
         SetWindowPos(NULL, m_rcCurrSize.left, m_rcCurrSize.top,
                      m_rcCurrSize.right - m_rcCurrSize.left,
                      m_rcCurrSize.bottom - m_rcCurrSize.top,
                      SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOCOPYBITS | SWP_NOACTIVATE);

         if ((nWidth <= 0) || (nHeight <= 0))
         {
            KillTimer(TIMER_CLOSE);
            DestroyWindow();
         }
         break;
      default:
         break;
   }
}


//
// WM_PAINT message handler
//

void CTaskBarPopupWnd::OnPaint() 
{
	CPaintDC dc(this);
   CDC dcMem;              // In-memory dc
   CBitmap *pOldBitmap;
   RECT rect;
   int nWidth, nHeight;

   nWidth = m_rcCurrSize.right - m_rcCurrSize.left;
   nHeight = m_rcCurrSize.bottom - m_rcCurrSize.top;

   GetClientRect(&rect);

   // Create in-memory DC containig window image
   dcMem.CreateCompatibleDC(&dc);
   pOldBitmap = dcMem.SelectObject(&m_bmpContent);

   // Move drawing from in-memory bitmap to screen
   dc.BitBlt(0, 0, rect.right, rect.bottom, &dcMem, m_nWidth - nWidth, m_nHeight - nHeight, SRCCOPY);

   // Cleanup
   dcMem.SelectObject(pOldBitmap);
   dcMem.DeleteDC();
}


//
// Create popup window
//

BOOL CTaskBarPopupWnd::Create(int nWidth, int nHeight, CWnd *pParentWnd)
{
   RECT rcWorkarea;
   APPBARDATA abd;

   m_nWidth = nWidth;
   m_nHeight = nHeight;
   
   m_rcClient.left = 0;
   m_rcClient.top = 0;
   m_rcClient.right = nWidth - GetSystemMetrics(SM_CXSIZEFRAME) * 2;
   m_rcClient.bottom = nHeight - GetSystemMetrics(SM_CYSIZEFRAME) * 2;

   // Determine desktop size and position of task bar
   SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkarea, 0);
   abd.cbSize = sizeof(APPBARDATA);
   abd.hWnd = NULL;
   SHAppBarMessage(ABM_GETTASKBARPOS, &abd);

   switch(abd.uEdge)
   {
      case ABE_LEFT:
         m_rcCurrSize.left = rcWorkarea.left + MARGIN;
         m_rcCurrSize.right = m_rcCurrSize.left;
         m_rcCurrSize.bottom = rcWorkarea.bottom - MARGIN;
         m_rcCurrSize.top = m_rcCurrSize.bottom - nHeight;
         m_nIncX = m_nStep;
         m_nIncY = 0;
         break;
      case ABE_RIGHT:
         m_rcCurrSize.right = rcWorkarea.right - MARGIN;
         m_rcCurrSize.left = m_rcCurrSize.right;
         m_rcCurrSize.bottom = rcWorkarea.bottom - MARGIN;
         m_rcCurrSize.top = m_rcCurrSize.bottom - nHeight;
         m_nIncX = -m_nStep;
         m_nIncY = 0;
         break;
      case ABE_TOP:
         m_rcCurrSize.right = rcWorkarea.right - MARGIN;
         m_rcCurrSize.left = m_rcCurrSize.right - nWidth;
         m_rcCurrSize.top = rcWorkarea.top + MARGIN;
         m_rcCurrSize.bottom = m_rcCurrSize.top;
         m_nIncX = 0;
         m_nIncY = m_nStep;
         break;
      default:    // Including ABE_BOTTOM
         m_rcCurrSize.right = rcWorkarea.right - MARGIN;
         m_rcCurrSize.left = m_rcCurrSize.right - nWidth;
         m_rcCurrSize.bottom = rcWorkarea.bottom - MARGIN;
         m_rcCurrSize.top = m_rcCurrSize.bottom;
         m_nIncX = 0;
         m_nIncY = -m_nStep;
         break;
   }

   return CreateEx(WS_EX_TOPMOST | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE,
                   NULL, _T("CTaskBarPopupWnd"),
                   WS_POPUP | WS_THICKFRAME,
                   m_rcCurrSize.left, m_rcCurrSize.top,
                   m_rcCurrSize.right - m_rcCurrSize.left, m_rcCurrSize.bottom - m_rcCurrSize.top,
                   pParentWnd->m_hWnd, 0, NULL);
}


//
// Draw content (supposed to be overriden in derived classes)
//

void CTaskBarPopupWnd::DrawContent(CDC &dc)
{
   dc.FillSolidRect(0, 0, m_nWidth, m_nHeight, GetSysColor(COLOR_WINDOW));
   dc.MoveTo(0, 0);
   dc.LineTo(m_nWidth, m_nHeight);
}


//
// Draw window content on bitmap
//

void CTaskBarPopupWnd::DrawOnBitmap()
{
   CDC *pdc, dc;
   CBitmap *pOldBitmap;

   pdc = GetDC();
   dc.CreateCompatibleDC(pdc);
   ReleaseDC(pdc);
   pOldBitmap = dc.SelectObject(&m_bmpContent);
   DrawContent(dc);
   dc.SelectObject(pOldBitmap);
   dc.DeleteDC();
}


//
// Update window content
//

void CTaskBarPopupWnd::Update()
{
   DrawOnBitmap();
   InvalidateRect(NULL, FALSE);
}


//
// Set attributes
//

void CTaskBarPopupWnd::SetAttr(DWORD dwStayTime, DWORD dwAnimInt, int nStep)
{
   m_dwStayTime = dwStayTime;
   m_dwAnimationInterval = dwAnimInt;
   m_nStep = nStep;
}


//
// Overrided PostNcDestroy()
//

void CTaskBarPopupWnd::PostNcDestroy() 
{
	CWnd::PostNcDestroy();
   delete this;
}
