// ObjectView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TITLE_BAR_HEIGHT      33
#define TITLE_BAR_COLOR       RGB(0, 0, 128)

#define MAX_COMMON_TAB        1


/////////////////////////////////////////////////////////////////////////////
// CObjectView

CObjectView::CObjectView()
{
   m_pObject = NULL;
   memset(m_pTabWnd, 0, sizeof(CWnd *) * MAX_TABS);
}

CObjectView::~CObjectView()
{
}


BEGIN_MESSAGE_MAP(CObjectView, CWnd)
	//{{AFX_MSG_MAP(CObjectView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
   ON_NOTIFY(TCN_SELCHANGE, ID_TAB_CTRL, OnTabChange)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectView message handlers

BOOL CObjectView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CObjectView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   HDC hdc;
   LRESULT nTemp;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create fonts
   hdc = ::GetDC(m_hWnd);
   m_fontHeader.CreateFont(-MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                           0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                           VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
   m_fontTabs.CreateFont(-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                         0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                         VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
   ::ReleaseDC(m_hWnd, hdc);

   // Create image list for tabs
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
   m_imageList.Add(theApp.LoadIcon(IDI_ALARM));

   GetClientRect(&rect);
   rect.top += TITLE_BAR_HEIGHT;
   m_wndTabCtrl.Create(WS_CHILD | WS_VISIBLE | TCS_FIXEDWIDTH | TCS_FORCELABELLEFT, rect, this, ID_TAB_CTRL);
   m_wndTabCtrl.SetFont(&m_fontTabs);
   m_wndTabCtrl.SetImageList(&m_imageList);
	
   // Create tab views
   m_wndOverview.Create(NULL, _T("Overview"), WS_CHILD, rect, this, 1);
   m_wndAlarms.Create(NULL, _T("Alarms"), WS_CHILD, rect, this, 2);

   // Create common tabs
   CreateTab(0, _T("Overview"), 0, &m_wndOverview);
   CreateTab(1, _T("Alarms"), 1, &m_wndAlarms);

   // Activate current tab
   nTemp = theApp.GetProfileInt(_T("ObjectView"), _T("ActiveTab"), 0);
   if ((nTemp < 0) || (nTemp >= m_wndTabCtrl.GetItemCount()))
      nTemp = 0;
   m_wndTabCtrl.SetCurSel(nTemp);
   OnTabChange(NULL, &nTemp);

	return 0;
}


//
// WM_SIZE message handler
//

void CObjectView::OnSize(UINT nType, int cx, int cy) 
{
   RECT rect;
   int nOffset;

	CWnd::OnSize(nType, cx, cy);
   m_wndTabCtrl.GetItemRect(0, &rect);
   m_wndTabCtrl.SetWindowPos(NULL, -1, TITLE_BAR_HEIGHT, cx + 2, (rect.bottom - rect.top) + 6, SWP_NOZORDER);

   nOffset = TITLE_BAR_HEIGHT + (rect.bottom - rect.top) + 6;
   m_wndOverview.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);
   m_wndAlarms.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CObjectView::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	
}


//
// Refresh view
//

void CObjectView::Refresh()
{
   RECT rect;

   GetClientRect(&rect);
   rect.bottom = TITLE_BAR_HEIGHT;
   InvalidateRect(&rect, FALSE);
}


//
// Set current object
//

void CObjectView::SetCurrentObject(NXC_OBJECT *pObject)
{
   int i, nItem, nCurrTabId, nTab;
   LRESULT nTemp;

   nCurrTabId = m_pTabWnd[m_wndTabCtrl.GetCurSel()]->GetDlgCtrlID();
   m_pObject = pObject;

   // Remove all class-specific tabs
   nItem = m_wndTabCtrl.GetItemCount();
   while(nItem-- > (MAX_COMMON_TAB + 1))
   {
      m_pTabWnd[nItem]->ShowWindow(SW_HIDE);
      m_wndTabCtrl.DeleteItem(nItem);
      m_pTabWnd[nItem] = NULL;
   }

   // Add tabs depending on object class
   if (m_pObject != NULL)
   {
      switch(m_pObject->iClass)
      {
         case OBJECT_NODE:
            break;
         case OBJECT_SUBNET:
            break;
         default:
            break;
      }
   }

   // Try to re-select same tab
   for(i = 0, nTab = 0; (i < MAX_TABS) && (m_pTabWnd[i] != NULL); i++)
   {
      if (m_pTabWnd[i]->GetDlgCtrlID() == nCurrTabId)
      {
         nTab = i;
         break;
      }
   }
   m_wndTabCtrl.SetCurSel(nTab);
   OnTabChange(NULL, &nTemp);

   // Refresh object view
   Refresh();
   for(i = 0; (m_pTabWnd[i] != NULL) && (i < MAX_TABS); i++)
      m_pTabWnd[i]->SendMessage(NXCM_SET_OBJECT, 0, (LPARAM)m_pObject);
}


//
// WM_PAINT message handler
//

void CObjectView::OnPaint() 
{
   RECT rect;
   CFont *pOldFont;
	CPaintDC dc(this); // device context for painting

   GetClientRect(&rect);
   dc.FillSolidRect(0, 0, rect.right, TITLE_BAR_HEIGHT, TITLE_BAR_COLOR);

   if (m_pObject != NULL)
   {
      pOldFont = dc.SelectObject(&m_fontHeader);
      dc.SetTextColor(RGB(255, 255, 255));
      dc.SetBkColor(TITLE_BAR_COLOR);
      dc.TextOut(10, 5, m_pObject->szName);
      dc.SelectObject(pOldFont);
   }

   // Draw divider between header and tab control
   rect.bottom = TITLE_BAR_HEIGHT;
   rect.top = rect.bottom - 3;
   rect.left--;
   rect.right++;
	dc.Draw3dRect(&rect, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DDKSHADOW));
	InflateRect(&rect, -1, -1);
	dc.FillSolidRect(&rect, GetSysColor(COLOR_3DFACE));
}


//
// Handler for tab change notification
//

void CObjectView::OnTabChange(LPNMHDR lpnmh, LRESULT *pResult)
{
   int i;

   for(i = 0; (m_pTabWnd[i] != NULL) && (i < MAX_TABS); i++)
      m_pTabWnd[i]->ShowWindow(SW_HIDE);
   m_pTabWnd[m_wndTabCtrl.GetCurSel()]->ShowWindow(SW_SHOW);
   *pResult = 0;
}


//
// Create new tab
//

void CObjectView::CreateTab(int nIndex, TCHAR *pszName, int nImage, CWnd *pWnd)
{
   m_wndTabCtrl.InsertItem(nIndex, pszName, nImage);
   m_pTabWnd[nIndex] = pWnd;
}


//
// WM_DESTROY message handler
//

void CObjectView::OnDestroy() 
{
   theApp.WriteProfileInt(_T("ObjectView"), _T("ActiveTab"), m_wndTabCtrl.GetCurSel());
	CWnd::OnDestroy();
}


//
// Overrided OnCmdMsg which will route all commands
// through active tab first
//

BOOL CObjectView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   CWnd *pWnd;

   if (::IsWindow(m_wndTabCtrl.m_hWnd) && (nID != ID_VIEW_REFRESH))
   {
      pWnd = m_pTabWnd[m_wndTabCtrl.GetCurSel()];
      if (pWnd != NULL)
         if (pWnd->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
            return TRUE;
   }

	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


//
// Handler for alarm updates
//

void CObjectView::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   m_wndAlarms.OnAlarmUpdate(dwCode, pAlarm);
}
