// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxcon.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
   ON_UPDATE_COMMAND_UI(ID_INDICATOR_STATE, OnUpdateState)
END_MESSAGE_MAP()

static UINT indicators[] =
{
   ID_SEPARATOR,
	ID_INDICATOR_STATE,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};


//
// CMainFrame construction/destruction
//

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}


//
// Handler for WM_CREATE message
//

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndToolBar))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		 !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnDestroy() 
{
   WINDOWPLACEMENT wndPlacement;

	CMDIFrameWnd::OnDestroy();

   // Save window placement
   GetWindowPlacement(&wndPlacement);
   AfxGetApp()->WriteProfileBinary(_T("General"), _T("WindowPlacement"),
                                   (BYTE *)&wndPlacement, sizeof(WINDOWPLACEMENT));
}


//
// Called by framework to update client library state indicator
//

void CMainFrame::OnUpdateState(CCmdUI *pCmdUI)
{
   static char *pszStateText[] =
   {
      "Disconnected",
      "Connecting to server...",
      "Idle",
      "Loading objects...",
      "Loading events..."
   };
   pCmdUI->Enable();
   pCmdUI->SetText(pszStateText[theApp.GetClientState()]);
}
