// WebBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "WebBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWebBrowser

IMPLEMENT_DYNCREATE(CWebBrowser, CMDIChildWnd)

CWebBrowser::CWebBrowser()
{
   m_pszStartURL = _tcsdup(_T(""));
}

CWebBrowser::CWebBrowser(TCHAR *pszStartURL)
{
   m_pszStartURL = _tcsdup(pszStartURL);
}

CWebBrowser::~CWebBrowser()
{
   safe_free(m_pszStartURL);
}


BEGIN_MESSAGE_MAP(CWebBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CWebBrowser)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWebBrowser message handlers

BOOL CWebBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_IEXPLORER));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CWebBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   m_pwndBrowser = (CHtmlView *)RUNTIME_CLASS(CHtmlView)->CreateObject();
   m_pwndBrowser->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect, this, ID_BROWSER_CTRL);
   m_pwndBrowser->Navigate(m_pszStartURL);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CWebBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_pwndBrowser->SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CWebBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_pwndBrowser->SetFocus();
}
