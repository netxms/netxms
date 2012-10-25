// ObjectView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectView.h"
#include "ObjectBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TITLE_BAR_HEIGHT      33
#define TITLE_BAR_COLOR       RGB(0, 0, 128)

#define SEARCH_BAR_HEIGHT		55
#define SEARCH_TEXT_WIDTH		200
#define SEARCH_BAR_X_MARGIN	5

#define MAX_COMMON_TAB        0


/////////////////////////////////////////////////////////////////////////////
// CObjectView

CObjectView::CObjectView()
{
   m_pObject = NULL;
   memset(m_pTabWnd, 0, sizeof(CWnd *) * MAX_TABS);
	m_bShowSearchBar = FALSE;
	m_nTitleBarOffset = 0;
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
	ON_COMMAND(ID_SEARCH_FIND_FIRST, OnSearchFindFirst)
	ON_COMMAND(ID_SEARCH_FIND_NEXT, OnSearchFindNext)
	ON_COMMAND(ID_SEARCH_NEXT_INSTANCE, OnSearchNextInstance)
	ON_COMMAND(ID_SEARCH_CLOSE, OnSearchClose)
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
	CDC *pdc;
   LRESULT nTemp;
	CBitmap bm;
	CFont *pf;
   static TBBUTTON tbButtons[] =
   {
      { 0, ID_SEARCH_FIND_NEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, "", 0, 0 },
      { 1, ID_SEARCH_FIND_FIRST, TBSTATE_ENABLED, TBSTYLE_BUTTON, "", 0, 1 },
      { 2, ID_SEARCH_NEXT_INSTANCE, TBSTATE_ENABLED, TBSTYLE_BUTTON, "", 0, 2 },
      { 3, ID_SEARCH_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, "", 0, 3 }
	};

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create fonts
	pdc = GetDC();
   m_fontHeader.CreateFont(-MulDiv(12, GetDeviceCaps(pdc->m_hDC, LOGPIXELSY), 72),
                           0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                           VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
   m_fontTabs.CreateFont(-MulDiv(8, GetDeviceCaps(pdc->m_hDC, LOGPIXELSY), 72),
                         0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                         VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
	
	pf = pdc->SelectObject(&m_fontTabs);
	m_nSearchTextOffset = pdc->GetTextExtent(_T("Find:"), 5).cx + SEARCH_BAR_X_MARGIN + 5;
	pdc->SelectObject(pf);

	// Create controls for search bar
	m_wndSearchText.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, rect, this, ID_EDIT_CTRL);
	m_wndSearchText.SetFont(&m_fontTabs, FALSE);
	m_wndSearchText.SetReturnCommand(ID_SEARCH_FIND_NEXT);
	m_wndSearchText.SetEscapeCommand(ID_SEARCH_CLOSE);
	m_wndSearchButtons.Create(WS_CHILD | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_LIST, rect, this, -1);
   m_imageListSearch.Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 1);
	bm.LoadBitmap(IDB_CLOSE);
   m_imageListSearch.Add(theApp.LoadIcon(IDI_FIND));
   m_imageListSearch.Add(theApp.LoadIcon(IDI_AGAIN));
   m_imageListSearch.Add(theApp.LoadIcon(IDI_NEXT));
	m_imageListSearch.Add(&bm, RGB(0, 0, 0));
	m_wndSearchButtons.SetImageList(&m_imageListSearch);
	m_wndSearchButtons.AddStrings(L"Find next\0Find from top\0Next instance\0Close\0");
	m_wndSearchButtons.AddButtons(sizeof(tbButtons) / sizeof(TBBUTTON), tbButtons);
	//m_wndSearchButtons.SetButtonSize(CSize(60, 22));
	m_wndSearchHint.Create(_T("HINT: Use ip: prefix to search by IP address, and / prefix to search by comment"),
	                       WS_CHILD, rect, this, ID_SEARCH_HINT);
	m_wndSearchHint.SetFont(&m_fontTabs, FALSE);

   // Create image list for tabs
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
   m_imageList.Add(theApp.LoadIcon(IDI_ALARM));
   m_imageList.Add(theApp.LoadIcon(IDI_TREE));
   m_imageList.Add(theApp.LoadIcon(IDI_CLUSTER));
   m_imageList.Add(theApp.LoadIcon(IDI_GRAPH));
   m_imageList.Add(theApp.LoadIcon(IDI_TOPOLOGY));
   m_imageList.Add(theApp.LoadIcon(IDI_DOCUMENT));
   m_imageList.Add(theApp.LoadIcon(IDI_SUBORDINATES));

   GetClientRect(&rect);
   rect.top += TITLE_BAR_HEIGHT;
   m_wndTabCtrl.Create(WS_CHILD | WS_VISIBLE | /*TCS_FIXEDWIDTH |*/ TCS_FORCELABELLEFT, rect, this, ID_TAB_CTRL);
   m_wndTabCtrl.SetMinTabWidth(110);
   m_wndTabCtrl.SetFont(&m_fontTabs);
   m_wndTabCtrl.SetImageList(&m_imageList);
	
   // Create tab views
   m_wndOverview.Create(NULL, _T("Overview"), WS_CHILD, rect, this, 1);
   m_wndAlarms.Create(NULL, _T("Alarms"), WS_CHILD, rect, this, 2);
   m_wndClusterView.Create(NULL, _T("Cluster"), WS_CHILD, rect, this, 3);
   m_wndPerfView.Create(NULL, _T("Performance"), WS_CHILD, rect, this, 4);
   m_wndLastValuesView.Create(NULL, _T("Last Values"), WS_CHILD, rect, this, 5);
   m_wndSubordinateView.Create(NULL, _T("Subordinates"), WS_CHILD, rect, this, 6);

   // Create common tabs
   CreateTab(0, _T("Overview"), 0, &m_wndOverview);

   // Activate current tab
   nTemp = theApp.GetProfileInt(_T("ObjectView"), _T("ActiveTab"), 0);
   if ((nTemp < 0) || (nTemp >= m_wndTabCtrl.GetItemCount()))
      nTemp = 0;
   m_wndTabCtrl.SetCurSel((int)nTemp);
   OnTabChange(NULL, &nTemp);

   ReleaseDC(pdc);
	return 0;
}


//
// WM_SIZE message handler
//

void CObjectView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	AdjustView();
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
   rect.bottom = TITLE_BAR_HEIGHT + m_nTitleBarOffset;
   InvalidateRect(&rect, FALSE);

   m_wndOverview.Refresh();
   m_wndClusterView.Refresh();
   m_wndSubordinateView.Refresh();
}


//
// Set current object
//

void CObjectView::SetCurrentObject(NXC_OBJECT *pObject)
{
   int i, nItem, nCurrTabId, nTab;
   LRESULT nTemp;
   RECT rect;

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
	nTab = 1;
   if (m_pObject != NULL)
   {
		if (m_pObject->iClass == OBJECT_NODE)
		{
         CreateTab(nTab++, _T("Last Values"), 6, &m_wndLastValuesView);
         CreateTab(nTab++, _T("Performance"), 4, &m_wndPerfView);
		}
      switch(m_pObject->iClass)
      {
			case OBJECT_CLUSTER:
            CreateTab(nTab++, _T("Cluster"), 3, &m_wndClusterView);
         case OBJECT_NODE:
         case OBJECT_SUBNET:
         case OBJECT_NETWORK:
         case OBJECT_CONTAINER:
         case OBJECT_SERVICEROOT:
            CreateTab(nTab++, _T("Alarms"), 1, &m_wndAlarms);
         default:
            break;
      }
		if ((m_pObject->iClass != OBJECT_INTERFACE) &&
			 (m_pObject->iClass != OBJECT_CONDITION) &&
			 (m_pObject->iClass != OBJECT_NETWORKSERVICE))
		{
         CreateTab(nTab++, _T("Subordinates"), 7, &m_wndSubordinateView);
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
   GetClientRect(&rect);
   rect.bottom = TITLE_BAR_HEIGHT + m_nTitleBarOffset;
   InvalidateRect(&rect, FALSE);
   for(i = 0; (m_pTabWnd[i] != NULL) && (i < MAX_TABS); i++)
      m_pTabWnd[i]->SendMessage(NXCM_SET_OBJECT, 0, (LPARAM)m_pObject);
}


//
// WM_PAINT message handler
//

void CObjectView::OnPaint() 
{
   RECT rect, rcText;
   CFont *pOldFont;
	CPaintDC dc(this); // device context for painting

   GetClientRect(&rect);

	// Draw search box
	if (m_bShowSearchBar)
	{
		dc.FillSolidRect(0, 0, rect.right, SEARCH_BAR_HEIGHT - 3, GetSysColor(COLOR_3DFACE));

		//m_imageListSearch.Draw(&dc, 0, CPoint(5, 9), ILD_TRANSPARENT);

      pOldFont = dc.SelectObject(&m_fontTabs);
		rcText.left = SEARCH_BAR_X_MARGIN;
		rcText.top = 7;
		rcText.bottom = 27;
		rcText.right = m_nSearchTextOffset;
      dc.DrawText(_T("Find:"), 5, &rcText, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
      dc.SelectObject(pOldFont);

		// Draw divider between search bar and header
		rect.bottom = SEARCH_BAR_HEIGHT;
		rect.top = rect.bottom - 1;
		dc.FillSolidRect(&rect, GetSysColor(COLOR_WINDOWFRAME));
		rect.bottom = SEARCH_BAR_HEIGHT - 1;
		rect.top = rect.bottom - 3;
		rect.left--;
		rect.right++;
		dc.Draw3dRect(&rect, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DDKSHADOW));
		InflateRect(&rect, -1, -1);
		dc.FillSolidRect(&rect, GetSysColor(COLOR_3DFACE));
	}

	// Draw title
   dc.FillSolidRect(0, m_nTitleBarOffset, rect.right, TITLE_BAR_HEIGHT, TITLE_BAR_COLOR);

   if (m_pObject != NULL)
   {
      pOldFont = dc.SelectObject(&m_fontHeader);
      dc.SetTextColor(RGB(255, 255, 255));
      dc.SetBkColor(TITLE_BAR_COLOR);
      dc.TextOut(10, 5 + m_nTitleBarOffset, m_pObject->szName);
      dc.SelectObject(pOldFont);
   }

   // Draw divider between header and tab control
   rect.bottom = TITLE_BAR_HEIGHT + m_nTitleBarOffset;
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


//
// Show or hide search bar
//

void CObjectView::ShowSearchBar(BOOL bShow)
{
	RECT rect;

	if ((bShow && m_bShowSearchBar) == (bShow || m_bShowSearchBar))
	{
		if (bShow)
			m_wndSearchText.SetFocus();
		return;	// Nothing to change
	}

	m_bShowSearchBar = bShow;
	if (m_bShowSearchBar)
	{
		m_nTitleBarOffset = SEARCH_BAR_HEIGHT;
		m_wndSearchText.ShowWindow(SW_SHOW);
		m_wndSearchButtons.ShowWindow(SW_SHOW);
		m_wndSearchHint.ShowWindow(SW_SHOW);
		m_wndSearchText.SetFocus();
	}
	else
	{
		m_nTitleBarOffset = 0;
		m_wndSearchText.ShowWindow(SW_HIDE);
		m_wndSearchButtons.ShowWindow(SW_HIDE);
		m_wndSearchHint.ShowWindow(SW_HIDE);
	}
	AdjustView();

	GetClientRect(&rect);
   rect.bottom = TITLE_BAR_HEIGHT + m_nTitleBarOffset;
   InvalidateRect(&rect, FALSE);
}


//
// Adjust components inside view
//

void CObjectView::AdjustView()
{
   RECT rect;
   int cx, cy, nOffset;

	GetClientRect(&rect);
	cx = rect.right;
	cy = rect.bottom;

   m_wndTabCtrl.GetItemRect(0, &rect);
   m_wndTabCtrl.SetWindowPos(NULL, -1, TITLE_BAR_HEIGHT + m_nTitleBarOffset,
	                          cx + 2, (rect.bottom - rect.top) + 6, SWP_NOZORDER);

   nOffset = TITLE_BAR_HEIGHT + (rect.bottom - rect.top) + 6 + m_nTitleBarOffset;
   m_wndOverview.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);
   m_wndAlarms.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);
   m_wndClusterView.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);
   m_wndPerfView.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);
   m_wndLastValuesView.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);
   m_wndSubordinateView.SetWindowPos(NULL, 0, nOffset, cx, cy - nOffset, SWP_NOZORDER);

	if (m_bShowSearchBar)
	{
		m_wndSearchText.SetWindowPos(NULL, m_nSearchTextOffset, 7, SEARCH_TEXT_WIDTH, 20, SWP_NOZORDER);
		m_wndSearchButtons.SetWindowPos(NULL, m_nSearchTextOffset + SEARCH_TEXT_WIDTH + 5, 6,
		                                cx - m_nSearchTextOffset - SEARCH_TEXT_WIDTH - 10, 22, SWP_NOZORDER);
		m_wndSearchHint.SetWindowPos(NULL, SEARCH_BAR_X_MARGIN, 31, cx - SEARCH_BAR_X_MARGIN * 2, 14, SWP_NOZORDER);
	}
}


//
// Handler for search bar "Close" button
//

void CObjectView::OnSearchClose() 
{
	ShowSearchBar(FALSE);
	GetParent()->GetParent()->SendMessage(NXCM_ACTIVATE_OBJECT_TREE);
}


//
// Handler for search bar "Find first" button
//

void CObjectView::OnSearchFindFirst() 
{
	CString strName;

	m_wndSearchText.GetWindowText(strName);
	GetParent()->GetParent()->SendMessage(NXCM_FIND_OBJECT, OBJECT_FIND_FIRST, (LPARAM)((LPCTSTR)strName));
	m_wndSearchText.SetFocus();
}


//
// Handler for search bar "Find next" button
//

void CObjectView::OnSearchFindNext() 
{
	CString strName;

	m_wndSearchText.GetWindowText(strName);
	GetParent()->GetParent()->SendMessage(NXCM_FIND_OBJECT, OBJECT_FIND_NEXT, (LPARAM)((LPCTSTR)strName));
	m_wndSearchText.SetFocus();
}


//
// Handler for search bar "Next" button
//

void CObjectView::OnSearchNextInstance() 
{
	GetParent()->GetParent()->SendMessage(NXCM_FIND_OBJECT, OBJECT_NEXT_INSTANCE, 0);
}
