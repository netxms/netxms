// EventPolicyEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventPolicyEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Transparent color for images in rule list
//

#define MASK_COLOR      RGB(255,255,255)


//
// Mask for any severity match
//

#define ANY_SEVERITY (RF_SEVERITY_INFO | RF_SEVERITY_WARNING | RF_SEVERITY_MINOR | \
                      RF_SEVERITY_MAJOR | RF_SEVERITY_CRITICAL)


//
// Image codes
//

#define IMG_ANY      0


/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor

IMPLEMENT_DYNCREATE(CEventPolicyEditor, CMDIChildWnd)

CEventPolicyEditor::CEventPolicyEditor()
{
   m_pEventPolicy = theApp.m_pEventPolicy;
   m_iCurrRow = -1;
   m_iCurrCol = -1;
}

CEventPolicyEditor::~CEventPolicyEditor()
{
}


BEGIN_MESSAGE_MAP(CEventPolicyEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CEventPolicyEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_POLICY_INSERTRULE_TOP, OnPolicyInsertruleTop)
	ON_COMMAND(ID_POLICY_INSERTRULE_BOTTOM, OnPolicyInsertruleBottom)
	ON_COMMAND(ID_POLICY_INSERTRULE_ABOVE, OnPolicyInsertruleAbove)
	ON_COMMAND(ID_POLICY_INSERTRULE_BELOW, OnPolicyInsertruleBelow)
	ON_UPDATE_COMMAND_UI(ID_POLICY_INSERTRULE_BELOW, OnUpdatePolicyInsertruleBelow)
	ON_UPDATE_COMMAND_UI(ID_POLICY_INSERTRULE_ABOVE, OnUpdatePolicyInsertruleAbove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor message handlers


//
// Redefined PreCreateWindow()
//

BOOL CEventPolicyEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_RULEMGR));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CEventPolicyEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CImageList *pImageList;
   CBitmap bmp;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create image list for rule list control
   pImageList = new CImageList;
   pImageList->Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 4);
   bmp.LoadBitmap(IDB_PSYM_ANY);
   pImageList->Add(&bmp, MASK_COLOR);
	
   // Create rule list control
   GetClientRect(&rect);
   m_wndRuleList.Create(WS_CHILD | WS_VISIBLE, rect, this, ID_RULE_LIST);
   m_wndRuleList.SetImageList(pImageList);

   // Setup columns
   m_wndRuleList.InsertColumn(0, "No.", 35, CF_CENTER | CF_TITLE_COLOR);
   m_wndRuleList.InsertColumn(1, "Source", 120);
   m_wndRuleList.InsertColumn(2, "Event", 120);
   m_wndRuleList.InsertColumn(3, "Severity", 80);
   m_wndRuleList.InsertColumn(4, "Action", 150);
   m_wndRuleList.InsertColumn(5, "Comments", 200);

   theApp.OnViewCreate(IDR_EPP_EDITOR, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CEventPolicyEditor::OnDestroy() 
{
   NXCDestroyEventPolicy(theApp.m_pEventPolicy);
   theApp.OnViewDestroy(IDR_EPP_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CLOSE message handler
//

void CEventPolicyEditor::OnClose() 
{
   DWORD dwResult;

	dwResult = DoRequest(NXCCloseEventPolicy, "Unlocking event processing policy...");
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, "Error unlocking event processing policy: %s");
	CMDIChildWnd::OnClose();
}


//
// WM_SIZE message handler
//

void CEventPolicyEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndRuleList.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_CONTEXTMENU message handler
//

void CEventPolicyEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   m_iCurrRow = m_wndRuleList.RowFromPoint(point.x, point.y);
   m_iCurrCol = m_wndRuleList.ColumnFromPoint(point.x, point.y);
   pMenu = theApp.GetContextMenu(3);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// WM_SETFOCUS message handler
//

void CEventPolicyEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndRuleList.SetFocus();
}


//
// Insert new rule into policy
//

void CEventPolicyEditor::InsertNewRule(int iInsertBefore)
{
   int iPos;
   char szBuffer[32];

   // Position for new rule
   iPos = (iInsertBefore > (int)m_pEventPolicy->dwNumRules) ? 
      (int)m_pEventPolicy->dwNumRules : iInsertBefore;

   // Extend rule list
   m_pEventPolicy->dwNumRules++;
   m_pEventPolicy->pRuleList = (NXC_EPP_RULE *)MemReAlloc(m_pEventPolicy->pRuleList,
         sizeof(NXC_EPP_RULE) * m_pEventPolicy->dwNumRules);
   if (iPos < (int)m_pEventPolicy->dwNumRules - 1)
      memmove(&m_pEventPolicy->pRuleList[iPos + 1], &m_pEventPolicy->pRuleList[iPos],
              sizeof(NXC_EPP_RULE) * ((int)m_pEventPolicy->dwNumRules - iPos - 1));

   // Setup empty rule
   memset(&m_pEventPolicy->pRuleList[iPos], 0, sizeof(NXC_EPP_RULE));
   m_pEventPolicy->pRuleList[iPos].dwId = (DWORD)iPos;
   m_pEventPolicy->pRuleList[iPos].dwFlags = ANY_SEVERITY;

   // Insert new row into rule list view
   m_wndRuleList.InsertRow(iPos);
   UpdateRow(iPos);

   // Renumber all rows below new
   for(iPos++; iPos < (int)m_pEventPolicy->dwNumRules; iPos++)
   {
      sprintf(szBuffer, "%d", iPos + 1);
      m_wndRuleList.ReplaceItem(iPos, 0, 0, szBuffer);
   }
}


//
// Update display row with data from in-memory policy
//

void CEventPolicyEditor::UpdateRow(int iRow)
{
   char szBuffer[256];

   // Rule number
   sprintf(szBuffer, "%d", iRow + 1);
   m_wndRuleList.AddItem(iRow, 0, szBuffer);

   // Source list
   if (m_pEventPolicy->pRuleList[iRow].dwNumSources == 0)
   {
      m_wndRuleList.AddItem(iRow, 1, "Any", IMG_ANY);
   }
   else
   {
   }
   
   // Event list
   if (m_pEventPolicy->pRuleList[iRow].dwNumEvents == 0)
   {
      m_wndRuleList.AddItem(iRow, 2, "Any", IMG_ANY);
   }
   else
   {
   }

   // Severity
   if ((m_pEventPolicy->pRuleList[iRow].dwFlags & ANY_SEVERITY) == ANY_SEVERITY)
   {
      m_wndRuleList.AddItem(iRow, 3, "Any", IMG_ANY);
   }
   else
   {
   }
}

//
// WM_COMMAND::ID_POLICY_INSERTRULE_TOP message handler
//

void CEventPolicyEditor::OnPolicyInsertruleTop() 
{
   InsertNewRule(0);
}


//
// WM_COMMAND::ID_POLICY_INSERTRULE_BOTTOM message handler
//

void CEventPolicyEditor::OnPolicyInsertruleBottom() 
{
   InsertNewRule(0x7FFFFFFF);
}


//
// WM_COMMAND::ID_POLICY_INSERTRULE_ABOVE message handler
//

void CEventPolicyEditor::OnPolicyInsertruleAbove() 
{
   InsertNewRule(m_iCurrRow);
}


//
// WM_COMMAND::ID_POLICY_INSERTRULE_BELOW message handler
//

void CEventPolicyEditor::OnPolicyInsertruleBelow() 
{
   InsertNewRule(m_iCurrRow + 1);
}


//
// ON_COMMAND_UPDATE_UI handlers
//

void CEventPolicyEditor::OnUpdatePolicyInsertruleBelow(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_iCurrRow != -1) && (m_iCurrCol != -1));
}

void CEventPolicyEditor::OnUpdatePolicyInsertruleAbove(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_iCurrRow != -1) && (m_iCurrCol != -1));
}
