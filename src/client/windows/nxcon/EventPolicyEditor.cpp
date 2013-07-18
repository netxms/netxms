// EventPolicyEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventPolicyEditor.h"
#include "RuleSeverityDlg.h"
#include "RuleAlarmDlg.h"
#include "RuleScriptDlg.h"
#include "RuleOptionsDlg.h"
#include "ActionSelDlg.h"
#include "RuleSituationDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Mask for any severity match
//

#define ANY_SEVERITY (RF_SEVERITY_INFO | RF_SEVERITY_WARNING | RF_SEVERITY_MINOR | \
                      RF_SEVERITY_MAJOR | RF_SEVERITY_CRITICAL)


//
// Column numbers
//

#define COL_RULE      0
#define COL_SOURCE    1
#define COL_EVENT     2
#define COL_SEVERITY  3
#define COL_SCRIPT    4
#define COL_ALARM     5
#define COL_SITUATION 6
#define COL_ACTION    7
#define COL_OPTIONS   8
#define COL_COMMENT   9


/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor

IMPLEMENT_DYNCREATE(CEventPolicyEditor, CMDIChildWnd)

CEventPolicyEditor::CEventPolicyEditor()
{
   m_pEventPolicy = theApp.m_pEventPolicy;
   m_pImageList = NULL;
   m_bIsModified = FALSE;
	m_clipboard = NULL;
}

CEventPolicyEditor::~CEventPolicyEditor()
{
   delete m_pImageList;
	NXCDestroyEventPolicy(m_clipboard);
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
	ON_UPDATE_COMMAND_UI(ID_POLICY_NEGATECELL, OnUpdatePolicyNegatecell)
	ON_UPDATE_COMMAND_UI(ID_POLICY_DISABLERULE, OnUpdatePolicyDisablerule)
	ON_UPDATE_COMMAND_UI(ID_POLICY_DELETERULE, OnUpdatePolicyDeleterule)
	ON_COMMAND(ID_POLICY_DISABLERULE, OnPolicyDisablerule)
	ON_COMMAND(ID_POLICY_ENABLERULE, OnPolicyEnablerule)
	ON_UPDATE_COMMAND_UI(ID_POLICY_ENABLERULE, OnUpdatePolicyEnablerule)
	ON_COMMAND(ID_POLICY_ADD, OnPolicyAdd)
	ON_UPDATE_COMMAND_UI(ID_POLICY_ADD, OnUpdatePolicyAdd)
	ON_COMMAND(ID_POLICY_DELETE, OnPolicyDelete)
	ON_UPDATE_COMMAND_UI(ID_POLICY_DELETE, OnUpdatePolicyDelete)
	ON_COMMAND(ID_POLICY_DELETERULE, OnPolicyDeleterule)
	ON_COMMAND(ID_POLICY_EDIT, OnPolicyEdit)
	ON_COMMAND(ID_POLICY_SAVE, OnPolicySave)
	ON_UPDATE_COMMAND_UI(ID_POLICY_SAVE, OnUpdatePolicySave)
	ON_COMMAND(ID_POLICY_NEGATECELL, OnPolicyNegatecell)
	ON_COMMAND(ID_POLICY_COPY, OnPolicyCopy)
	ON_UPDATE_COMMAND_UI(ID_POLICY_COPY, OnUpdatePolicyCopy)
	ON_COMMAND(ID_POLICY_PASTE, OnPolicyPaste)
	ON_UPDATE_COMMAND_UI(ID_POLICY_PASTE, OnUpdatePolicyPaste)
	ON_COMMAND(ID_POLICY_CUT, OnPolicyCut)
	ON_UPDATE_COMMAND_UI(ID_POLICY_CUT, OnUpdatePolicyCut)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, ID_RULE_LIST, OnRuleListDblClk)
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
   CBitmap bmp;
   DWORD i;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create image list for rule list control
   m_pImageList = new CImageList;
   m_pImageList->Create(g_pObjectSmallImageList);
   m_iImageAny = m_pImageList->GetImageCount();
   LoadBitmapIntoList(m_pImageList, IDB_ANY, PSYM_MASK_COLOR);
   LoadBitmapIntoList(m_pImageList, IDB_NONE, PSYM_MASK_COLOR);
   m_pImageList->Add(theApp.LoadIcon(IDI_SITUATION));
   m_pImageList->Add(theApp.LoadIcon(IDI_INSTANCE));
   m_iImageSeverityBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_WARNING));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MINOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MAJOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_CRITICAL));
   m_pImageList->Add(theApp.LoadIcon(IDI_UNKNOWN));
   m_pImageList->Add(theApp.LoadIcon(IDI_ACK));
   m_iImageActionsBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_EXEC));
   m_pImageList->Add(theApp.LoadIcon(IDI_REXEC));
   m_pImageList->Add(theApp.LoadIcon(IDI_EMAIL));
   m_pImageList->Add(theApp.LoadIcon(IDI_SMS));
   m_pImageList->Add(theApp.LoadIcon(IDI_FORWARD_EVENT));
   m_iImageOptionsBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_STOP));
	
   // Create rule list control
   GetClientRect(&rect);
   m_wndRuleList.Create(WS_CHILD | WS_VISIBLE, rect, this, ID_RULE_LIST);
   m_wndRuleList.SetImageList(m_pImageList);

   // Setup columns
   m_wndRuleList.InsertColumn(0, _T("No."), 35, CF_CENTER | CF_TITLE_COLOR | CF_NON_SELECTABLE);
   m_wndRuleList.InsertColumn(1, _T("Source"), 150);
   m_wndRuleList.InsertColumn(2, _T("Event"), 150);
   m_wndRuleList.InsertColumn(3, _T("Severity"), 90, CF_NON_SELECTABLE);
   m_wndRuleList.InsertColumn(4, _T("Script"), 150, CF_TEXTBOX | CF_NON_SELECTABLE);
   m_wndRuleList.InsertColumn(5, _T("Alarm"), 150, CF_NON_SELECTABLE);
   m_wndRuleList.InsertColumn(6, _T("Situation"), 150, CF_NON_SELECTABLE);
   m_wndRuleList.InsertColumn(7, _T("Action"), 150);
   m_wndRuleList.InsertColumn(8, _T("Options"), 150, CF_NON_SELECTABLE);
   m_wndRuleList.InsertColumn(9, _T("Comments"), 200, CF_TEXTBOX | CF_NON_SELECTABLE);
   m_wndRuleList.RestoreColumns(_T("EventPolicyEditor"), _T("RuleList"));

   // Fill rule list with existing rules
   for(i = 0; i < m_pEventPolicy->dwNumRules; i++)
   {
      m_wndRuleList.InsertRow(i);
      UpdateRow(i);
      if (m_pEventPolicy->pRuleList[i].dwFlags & RF_DISABLED)
         m_wndRuleList.EnableRow(i, FALSE);
   }      

   theApp.OnViewCreate(VIEW_EPP_EDITOR, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CEventPolicyEditor::OnDestroy() 
{
   NXCDestroyEventPolicy(theApp.m_pEventPolicy);
   theApp.OnViewDestroy(VIEW_EPP_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CLOSE message handler
//

void CEventPolicyEditor::OnClose() 
{
   DWORD dwResult;
   int iAnswer = IDNO;

   m_wndRuleList.SaveColumns(_T("EventPolicyEditor"), _T("RuleList"));

   if (m_bIsModified)
   {
      iAnswer = MessageBox(_T("Event processing policy was modified. Do you want to save changes?"),
                           _T("Confirmation"), MB_YESNOCANCEL | MB_ICONQUESTION);

      if (iAnswer == IDYES)
      {
         dwResult = DoRequestArg2(NXCSaveEventPolicy, g_hSession, 
                                  m_pEventPolicy, _T("Saving event processing policy..."));
         if (dwResult != RCC_SUCCESS)
         {
            theApp.ErrorBox(dwResult, _T("Error saving event processing policy: %s"));
            iAnswer = IDCANCEL;  // Will not close window if there are errors
         }
      }
   }

   if (iAnswer != IDCANCEL)
   {
	   dwResult = DoRequestArg1(NXCCloseEventPolicy, g_hSession, 
                               _T("Unlocking event processing policy..."));
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, _T("Error unlocking event processing policy: %s"));
	   CMDIChildWnd::OnClose();
   }
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
   int iMenu;

   switch(m_wndRuleList.GetCurrentColumn())
   {
      case COL_SOURCE:
      case COL_EVENT:
      case COL_ACTION:
         iMenu = 4;
         break;
      case COL_SEVERITY:
      case COL_SCRIPT:
      case COL_ALARM:
		case COL_SITUATION:
		case COL_OPTIONS:
      case COL_COMMENT:
         iMenu = 5;
         break;
      default:
         iMenu = 3;  // Row operations only
         break;
   }
   pMenu = theApp.GetContextMenu(iMenu);
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
   TCHAR szBuffer[32];

   // Position for new rule
   iPos = (iInsertBefore > (int)m_pEventPolicy->dwNumRules) ? 
      (int)m_pEventPolicy->dwNumRules : iInsertBefore;

   // Extend rule list
   m_pEventPolicy->dwNumRules++;
   m_pEventPolicy->pRuleList = (NXC_EPP_RULE *)realloc(m_pEventPolicy->pRuleList,
         sizeof(NXC_EPP_RULE) * m_pEventPolicy->dwNumRules);
   if (iPos < (int)m_pEventPolicy->dwNumRules - 1)
      memmove(&m_pEventPolicy->pRuleList[iPos + 1], &m_pEventPolicy->pRuleList[iPos],
              sizeof(NXC_EPP_RULE) * ((int)m_pEventPolicy->dwNumRules - iPos - 1));

   // Setup empty rule
   memset(&m_pEventPolicy->pRuleList[iPos], 0, sizeof(NXC_EPP_RULE));
   m_pEventPolicy->pRuleList[iPos].dwId = (DWORD)iPos;
   m_pEventPolicy->pRuleList[iPos].dwFlags = ANY_SEVERITY;
	m_pEventPolicy->pRuleList[iPos].pSituationAttrList = new StringMap;

   // Insert new row into rule list view
   m_wndRuleList.InsertRow(iPos);
   UpdateRow(iPos);

   // Renumber all rows below new
   for(iPos++; iPos < (int)m_pEventPolicy->dwNumRules; iPos++)
   {
      _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), iPos + 1);
      m_wndRuleList.ReplaceItem(iPos, 0, 0, szBuffer);
   }

   Modify();
}


//
// Update display row with data from in-memory policy
//

void CEventPolicyEditor::UpdateRow(int iRow)
{
   TCHAR szBuffer[MAX_DB_STRING];
   DWORD i;

   // Rule number
   _sntprintf_s(szBuffer, MAX_DB_STRING, _TRUNCATE, _T("%d"), iRow + 1);
   if (m_wndRuleList.GetNumItems(iRow, COL_RULE) == 0)
      m_wndRuleList.AddItem(iRow, COL_RULE, szBuffer);
   else
      m_wndRuleList.ReplaceItem(iRow, COL_RULE, 0, szBuffer);

   // Source list
   m_wndRuleList.ClearCell(iRow, COL_SOURCE);
   if (m_pEventPolicy->pRuleList[iRow].dwNumSources == 0)
   {
      m_wndRuleList.AddItem(iRow, COL_SOURCE, _T("Any"), m_iImageAny);
   }
   else
   {
      NXC_OBJECT *pObject;

      for(i = 0; i < m_pEventPolicy->pRuleList[iRow].dwNumSources; i++)
      {
         pObject = NXCFindObjectById(g_hSession, m_pEventPolicy->pRuleList[iRow].pdwSourceList[i]);
         if (pObject != NULL)
            m_wndRuleList.AddItem(iRow, COL_SOURCE, pObject->szName, pObject->iClass);
      }
   }
   m_wndRuleList.SetNegationFlag(iRow, COL_SOURCE, (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_NEGATED_SOURCE) ? TRUE : FALSE);
   
   // Event list
   m_wndRuleList.ClearCell(iRow, COL_EVENT);
   if (m_pEventPolicy->pRuleList[iRow].dwNumEvents == 0)
   {
      m_wndRuleList.AddItem(iRow, COL_EVENT, _T("Any"), m_iImageAny);
   }
   else
   {
      for(i = 0; i < m_pEventPolicy->pRuleList[iRow].dwNumEvents; i++)
         m_wndRuleList.AddItem(iRow, COL_EVENT, 
             (TCHAR *)NXCGetEventName(g_hSession, m_pEventPolicy->pRuleList[iRow].pdwEventList[i]),
             m_iImageSeverityBase + NXCGetEventSeverity(g_hSession, m_pEventPolicy->pRuleList[iRow].pdwEventList[i]));
   }
   m_wndRuleList.SetNegationFlag(iRow, COL_EVENT, (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_NEGATED_EVENTS) ? TRUE : FALSE);

   // Severity
   m_wndRuleList.ClearCell(iRow, COL_SEVERITY);
   if ((m_pEventPolicy->pRuleList[iRow].dwFlags & ANY_SEVERITY) == ANY_SEVERITY)
   {
      m_wndRuleList.AddItem(iRow, COL_SEVERITY, _T("Any"), m_iImageAny);
   }
   else
   {
      DWORD dwMask;

      for(i = 0, dwMask = RF_SEVERITY_INFO; i < 5; i++, dwMask <<= 1)
         if (m_pEventPolicy->pRuleList[iRow].dwFlags & dwMask)
            m_wndRuleList.AddItem(iRow, COL_SEVERITY, g_szStatusTextSmall[i], 
                                  m_iImageSeverityBase + i);
   }

   // Script
   m_wndRuleList.SetCellText(iRow, COL_SCRIPT, m_pEventPolicy->pRuleList[iRow].pszScript);

   // Alarm
   if (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_GENERATE_ALARM)
   {
      if (m_wndRuleList.GetNumItems(iRow, COL_ALARM) == 0)
         m_wndRuleList.AddItem(iRow, COL_ALARM,
			                      (m_pEventPolicy->pRuleList[iRow].wAlarmSeverity == SEVERITY_TERMINATE) ? m_pEventPolicy->pRuleList[iRow].szAlarmKey : m_pEventPolicy->pRuleList[iRow].szAlarmMessage,
                               m_iImageSeverityBase + m_pEventPolicy->pRuleList[iRow].wAlarmSeverity);
      else
         m_wndRuleList.ReplaceItem(iRow, COL_ALARM, 0,
			                          (m_pEventPolicy->pRuleList[iRow].wAlarmSeverity == SEVERITY_TERMINATE) ? m_pEventPolicy->pRuleList[iRow].szAlarmKey : m_pEventPolicy->pRuleList[iRow].szAlarmMessage,
                                   m_iImageSeverityBase + m_pEventPolicy->pRuleList[iRow].wAlarmSeverity);
   }
   else
   {
      if (m_wndRuleList.GetNumItems(iRow, COL_ALARM) == 0)
         m_wndRuleList.AddItem(iRow, COL_ALARM, _T("None"), m_iImageAny + 1);
      else
         m_wndRuleList.ReplaceItem(iRow, COL_ALARM, 0, _T("None"), m_iImageAny + 1);
   }

   // Action
   m_wndRuleList.ClearCell(iRow, COL_ACTION);
   if (m_pEventPolicy->pRuleList[iRow].dwNumActions > 0)
   {
      NXC_ACTION *pAction;

      LockActions();
      for(i = 0; i < m_pEventPolicy->pRuleList[iRow].dwNumActions; i++)
      {
         pAction = FindActionById(m_pEventPolicy->pRuleList[iRow].pdwActionList[i]);
         if (pAction != NULL)
            m_wndRuleList.AddItem(iRow, COL_ACTION, pAction->szName, 
                                  m_iImageActionsBase + pAction->iType);
      }
      UnlockActions();
   }
   else
   {
      m_wndRuleList.AddItem(iRow, COL_ACTION, _T("None"), m_iImageAny + 1);
   }

   // Options
   m_wndRuleList.ClearCell(iRow, COL_OPTIONS);
   if (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_STOP_PROCESSING)
   {
      m_wndRuleList.AddItem(iRow, COL_OPTIONS, _T("Stop processing"), m_iImageOptionsBase);
   }
   if (m_wndRuleList.GetNumItems(iRow, COL_OPTIONS) == 0)
   {
      m_wndRuleList.AddItem(iRow, COL_OPTIONS, _T("None"), m_iImageAny + 1);
   }

   // Situation
   m_wndRuleList.ClearCell(iRow, COL_SITUATION);
   if (m_pEventPolicy->pRuleList[iRow].dwSituationId != 0)
   {
      m_wndRuleList.AddItem(iRow, COL_SITUATION,
		                      theApp.GetSituationName(m_pEventPolicy->pRuleList[iRow].dwSituationId, szBuffer),
									 m_iImageAny + 2);
      m_wndRuleList.AddItem(iRow, COL_SITUATION,
		                      m_pEventPolicy->pRuleList[iRow].szSituationInstance,
									 m_iImageAny + 3);
   }
	else
   {
      m_wndRuleList.AddItem(iRow, COL_SITUATION, _T("None"), m_iImageAny + 1);
   }

   // Comment
   m_wndRuleList.SetCellText(iRow, COL_COMMENT, m_pEventPolicy->pRuleList[iRow].pszComment);

   // Enable/disable selection
   m_wndRuleList.EnableCellSelection(iRow, COL_SOURCE, m_pEventPolicy->pRuleList[iRow].dwNumSources != 0);
   m_wndRuleList.EnableCellSelection(iRow, COL_EVENT, m_pEventPolicy->pRuleList[iRow].dwNumEvents != 0);
   m_wndRuleList.EnableCellSelection(iRow, COL_ACTION, m_pEventPolicy->pRuleList[iRow].dwNumActions != 0);
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
   InsertNewRule(m_wndRuleList.GetCurrentRow());
}


//
// WM_COMMAND::ID_POLICY_INSERTRULE_BELOW message handler
//

void CEventPolicyEditor::OnPolicyInsertruleBelow() 
{
   InsertNewRule(m_wndRuleList.GetCurrentRow() + 1);
}


//
// Enable or disable selected rows
//

void CEventPolicyEditor::EnableSelectedRows(BOOL bEnable)
{
   int iRow;

   iRow = m_wndRuleList.GetNextRow(-1, RLF_SELECTED);
   if (iRow != -1)
   {
      while(iRow != -1)
      {
         if (bEnable)
            m_pEventPolicy->pRuleList[iRow].dwFlags &= ~RF_DISABLED;
         else
            m_pEventPolicy->pRuleList[iRow].dwFlags |= RF_DISABLED;
         m_wndRuleList.EnableRow(iRow, bEnable);
         iRow = m_wndRuleList.GetNextRow(iRow, RLF_SELECTED);
      }
      Modify();
   }
}


//
// WM_COMMAND::ID_POLICY_DISABLERULE message handler
//

void CEventPolicyEditor::OnPolicyDisablerule() 
{
   EnableSelectedRows(FALSE);
}


//
// WM_COMMAND::ID_POLICY_ENABLERULE message handler
//

void CEventPolicyEditor::OnPolicyEnablerule() 
{
   EnableSelectedRows(TRUE);
}


//
// WM_COMMAND::ID_POLICY_ADD message handler
//

void CEventPolicyEditor::OnPolicyAdd() 
{
   switch(m_wndRuleList.GetCurrentColumn())
   {
      case COL_SOURCE:
         AddSource();
         break;
      case COL_EVENT:
         AddEvent();
         break;
      case COL_ACTION:
         AddAction();
         break;
   }
}


//
// ON_COMMAND_UPDATE_UI handlers
//

void CEventPolicyEditor::OnUpdatePolicyInsertruleBelow(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndRuleList.GetCurrentRow() != -1);
}

void CEventPolicyEditor::OnUpdatePolicyInsertruleAbove(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndRuleList.GetCurrentRow() != -1);
}

void CEventPolicyEditor::OnUpdatePolicyNegatecell(CCmdUI* pCmdUI) 
{
   int nRow, nCol;

   // Negation available only for source, event and severity columns
   nCol = m_wndRuleList.GetCurrentColumn();
   if ((nCol == COL_SOURCE) || (nCol == COL_EVENT))
   {
      nRow = m_wndRuleList.GetCurrentRow();
      pCmdUI->Enable(TRUE);
      if ((nRow >= 0) && (nRow < (int)m_pEventPolicy->dwNumRules))
      {
         pCmdUI->SetCheck((m_pEventPolicy->pRuleList[nRow].dwFlags & 
            ((nCol == COL_SOURCE) ? RF_NEGATED_SOURCE : RF_NEGATED_EVENTS)) ? TRUE : FALSE);
      }
      else
      {
         pCmdUI->SetCheck(FALSE);
      }
   }
   else
   {
      pCmdUI->Enable(FALSE);
      pCmdUI->SetCheck(FALSE);
   }
}

void CEventPolicyEditor::OnUpdatePolicyDisablerule(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndRuleList.GetCurrentRow() != -1);
}

void CEventPolicyEditor::OnUpdatePolicyDeleterule(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndRuleList.GetCurrentRow() != -1);
}

void CEventPolicyEditor::OnUpdatePolicyEnablerule(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndRuleList.GetCurrentRow() != -1);
}

void CEventPolicyEditor::OnUpdatePolicyAdd(CCmdUI* pCmdUI) 
{
   int iColumn;

   iColumn = m_wndRuleList.GetCurrentColumn();
   pCmdUI->Enable((m_wndRuleList.GetCurrentRow() != -1) &&
                  ((iColumn == COL_SOURCE) || (iColumn == COL_EVENT) || 
                   (iColumn == COL_SEVERITY) || (iColumn == COL_ACTION)));
}

void CEventPolicyEditor::OnUpdatePolicyDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndRuleList.GetCurrentItem() != -1);
}

void CEventPolicyEditor::OnUpdatePolicySave(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_bIsModified);
}


//
// Add new source object to current row
//

void CEventPolicyEditor::AddSource(void)
{
   CObjectSelDlg dlg;
   DWORD i, j;
   int iRow;

   dlg.m_dwAllowedClasses = SCL_NODE | SCL_CLUSTER | SCL_CONTAINER | SCL_SUBNET | SCL_NETWORK | SCL_SERVICEROOT;
   if (dlg.DoModal() == IDOK)
   {
      iRow = m_wndRuleList.GetCurrentRow();
      for(i = 0; i < dlg.m_dwNumObjects; i++)
      {
         // Check if object already in the list
         for(j = 0; j < m_pEventPolicy->pRuleList[iRow].dwNumSources; j++)
            if (m_pEventPolicy->pRuleList[iRow].pdwSourceList[j] == dlg.m_pdwObjectList[i])
               break;
         if (j == m_pEventPolicy->pRuleList[iRow].dwNumSources)
         {
            // New object, add it to source list
            m_pEventPolicy->pRuleList[iRow].dwNumSources++;
            m_pEventPolicy->pRuleList[iRow].pdwSourceList = 
               (UINT32 *)realloc(m_pEventPolicy->pRuleList[iRow].pdwSourceList,
                  sizeof(UINT32) * m_pEventPolicy->pRuleList[iRow].dwNumSources);
            m_pEventPolicy->pRuleList[iRow].pdwSourceList[j] = dlg.m_pdwObjectList[i];
            Modify();
         }
      }
      UpdateRow(iRow);
      Modify();
   }
}


//
// Add new event to current rule
//

void CEventPolicyEditor::AddEvent(void)
{
   CEventSelDlg dlg;
   DWORD i, j;
   int iRow;

   if (dlg.DoModal() == IDOK)
   {
      iRow = m_wndRuleList.GetCurrentRow();
      for(i = 0; i < dlg.m_dwNumEvents; i++)
      {
         // Check if object already in the list
         for(j = 0; j < m_pEventPolicy->pRuleList[iRow].dwNumEvents; j++)
            if (m_pEventPolicy->pRuleList[iRow].pdwEventList[j] == dlg.m_pdwEventList[i])
               break;
         if (j == m_pEventPolicy->pRuleList[iRow].dwNumEvents)
         {
            // New object, add it to source list
            m_pEventPolicy->pRuleList[iRow].dwNumEvents++;
            m_pEventPolicy->pRuleList[iRow].pdwEventList = 
               (UINT32 *)realloc(m_pEventPolicy->pRuleList[iRow].pdwEventList,
                  sizeof(UINT32) * m_pEventPolicy->pRuleList[iRow].dwNumEvents);
            m_pEventPolicy->pRuleList[iRow].pdwEventList[j] = dlg.m_pdwEventList[i];
            Modify();
         }
      }
      UpdateRow(iRow);
      Modify();
   }
}


//
// Delete element
//

void CEventPolicyEditor::OnPolicyDelete(void) 
{
   int iRow, iCol, iItem;

   iRow = m_wndRuleList.GetCurrentRow();
   iCol = m_wndRuleList.GetCurrentColumn();
   iItem = m_wndRuleList.GetCurrentItem();
   if (iItem != -1)
   {
      switch(iCol)
      {
         case COL_SOURCE:     // Source
            if (m_pEventPolicy->pRuleList[iRow].dwNumSources > 0)
            {
               m_pEventPolicy->pRuleList[iRow].dwNumSources--;
               memmove(&m_pEventPolicy->pRuleList[iRow].pdwSourceList[iItem],
                       &m_pEventPolicy->pRuleList[iRow].pdwSourceList[iItem + 1],
                       sizeof(DWORD) * (m_pEventPolicy->pRuleList[iRow].dwNumSources - iItem));
               if (m_pEventPolicy->pRuleList[iRow].dwNumSources == 0)
               {
                  m_wndRuleList.ReplaceItem(iRow, iCol, 0, _T("Any"), m_iImageAny);
                  m_wndRuleList.EnableCellSelection(iRow, iCol, FALSE);
               }
               else
               {
                  m_wndRuleList.DeleteItem(iRow, iCol, iItem);
               }
               Modify();
            }
            break;
         case COL_EVENT:     // Event
            if (m_pEventPolicy->pRuleList[iRow].dwNumEvents > 0)
            {
               m_pEventPolicy->pRuleList[iRow].dwNumEvents--;
               memmove(&m_pEventPolicy->pRuleList[iRow].pdwEventList[iItem],
                       &m_pEventPolicy->pRuleList[iRow].pdwEventList[iItem + 1],
                       sizeof(DWORD) * (m_pEventPolicy->pRuleList[iRow].dwNumEvents - iItem));
               if (m_pEventPolicy->pRuleList[iRow].dwNumEvents == 0)
               {
                  m_wndRuleList.ReplaceItem(iRow, iCol, 0, _T("Any"), m_iImageAny);
                  m_wndRuleList.EnableCellSelection(iRow, iCol, FALSE);
               }
               else
               {
                  m_wndRuleList.DeleteItem(iRow, iCol, iItem);
               }
               Modify();
            }
            break;
         case COL_ACTION:
            if (m_pEventPolicy->pRuleList[iRow].dwNumActions > 0)
            {
               m_pEventPolicy->pRuleList[iRow].dwNumActions--;
               memmove(&m_pEventPolicy->pRuleList[iRow].pdwActionList[iItem],
                       &m_pEventPolicy->pRuleList[iRow].pdwActionList[iItem + 1],
                       sizeof(DWORD) * (m_pEventPolicy->pRuleList[iRow].dwNumActions - iItem));
               if (m_pEventPolicy->pRuleList[iRow].dwNumActions == 0)
               {
                  m_wndRuleList.ReplaceItem(iRow, iCol, 0, _T("None"), m_iImageAny + 1);
                  m_wndRuleList.EnableCellSelection(iRow, iCol, FALSE);
               }
               else
               {
                  m_wndRuleList.DeleteItem(iRow, iCol, iItem);
               }
               Modify();
            }
            break;
      }
   }
}


//
// Delete selected rules
//

void CEventPolicyEditor::DeleteSelectedRules()
{
   int i, iRow;
   TCHAR szBuffer[32];

   iRow = m_wndRuleList.GetNextRow(-1, RLF_SELECTED);
   if (iRow != -1)
   {
      while(iRow != -1)
      {
         m_wndRuleList.DeleteRow(iRow);
         NXCDeletePolicyRule(m_pEventPolicy, iRow);
         for(i = iRow; i < (int)m_pEventPolicy->dwNumRules; i++)
         {
            _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), i + 1);
            m_wndRuleList.ReplaceItem(i, 0, 0, szBuffer);
         }
         iRow = m_wndRuleList.GetNextRow(iRow - 1, RLF_SELECTED);
      }
      Modify();
   }
}


//
// Delete selected rule(s)
//

void CEventPolicyEditor::OnPolicyDeleterule(void) 
{
	DeleteSelectedRules();
}


//
// Edit current cell
//

void CEventPolicyEditor::OnPolicyEdit() 
{
   int iRow, iColumn;

   iRow = m_wndRuleList.GetCurrentRow();
   iColumn = m_wndRuleList.GetCurrentColumn();
   if (iRow != -1)
      switch(iColumn)
      {
         case COL_RULE:
            EnableSelectedRows((m_pEventPolicy->pRuleList[iRow].dwFlags & RF_DISABLED) ? TRUE : FALSE);
            break;
         case COL_SEVERITY:
            EditSeverity(iRow);
            break;
         case COL_SCRIPT:
            EditScript(iRow);
            break;
         case COL_ALARM:
            EditAlarm(iRow);
            break;
         case COL_SITUATION:
            EditSituation(iRow);
            break;
			case COL_OPTIONS:
				EditOptions(iRow);
				break;
         case COL_COMMENT:
            EditComment(iRow);
            break;
         default:
            break;
      }
}


//
// Edit severity cell
//

void CEventPolicyEditor::EditSeverity(int iRow)
{
   CRuleSeverityDlg dlg;

   dlg.m_bNormal = (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_SEVERITY_INFO) ? TRUE : FALSE;
   dlg.m_bWarning = (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_SEVERITY_WARNING) ? TRUE : FALSE;
   dlg.m_bMinor = (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_SEVERITY_MINOR) ? TRUE : FALSE;
   dlg.m_bMajor = (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_SEVERITY_MAJOR) ? TRUE : FALSE;
   dlg.m_bCritical = (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_SEVERITY_CRITICAL) ? TRUE : FALSE;
   if (dlg.DoModal() == IDOK)
   {
      DWORD dwSeverity = 0;

      if (dlg.m_bNormal)
         dwSeverity |= RF_SEVERITY_INFO;
      if (dlg.m_bWarning)
         dwSeverity |= RF_SEVERITY_WARNING;
      if (dlg.m_bMinor)
         dwSeverity |= RF_SEVERITY_MINOR;
      if (dlg.m_bMajor)
         dwSeverity |= RF_SEVERITY_MAJOR;
      if (dlg.m_bCritical)
         dwSeverity |= RF_SEVERITY_CRITICAL;

      m_pEventPolicy->pRuleList[iRow].dwFlags &= ~(ANY_SEVERITY);
      m_pEventPolicy->pRuleList[iRow].dwFlags |= dwSeverity;
      Modify();
      UpdateRow(iRow);
   }
}


//
// Edit alarm cell
//

void CEventPolicyEditor::EditAlarm(int iRow)
{
   CRuleAlarmDlg dlg;

   dlg.m_nMode = (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_GENERATE_ALARM) ? ((m_pEventPolicy->pRuleList[iRow].wAlarmSeverity == SEVERITY_TERMINATE) ? 1 : 0) : -1;
   if (dlg.m_nMode == 0)
   {
      dlg.m_strMessage = m_pEventPolicy->pRuleList[iRow].szAlarmMessage;
      dlg.m_strKey = m_pEventPolicy->pRuleList[iRow].szAlarmKey;
      dlg.m_iSeverity = m_pEventPolicy->pRuleList[iRow].wAlarmSeverity;
		dlg.m_dwTimeout = m_pEventPolicy->pRuleList[iRow].dwAlarmTimeout;
		dlg.m_dwTimeoutEvent = m_pEventPolicy->pRuleList[iRow].dwAlarmTimeoutEvent;
   }
   else if (dlg.m_nMode == 1)
   {
      dlg.m_strAckKey = m_pEventPolicy->pRuleList[iRow].szAlarmKey;
		dlg.m_useRegexp = (m_pEventPolicy->pRuleList[iRow].dwFlags & RF_TERMINATE_BY_REGEXP) ? TRUE : FALSE;
   }
   if (dlg.DoModal() == IDOK)
   {
      if (dlg.m_nMode != -1)
      {
         m_pEventPolicy->pRuleList[iRow].dwFlags |= RF_GENERATE_ALARM;
			if (dlg.m_nMode == 0)
			{
				m_pEventPolicy->pRuleList[iRow].wAlarmSeverity = dlg.m_iSeverity;
				nx_strncpy(m_pEventPolicy->pRuleList[iRow].szAlarmMessage, (LPCTSTR)dlg.m_strMessage, MAX_DB_STRING);
				nx_strncpy(m_pEventPolicy->pRuleList[iRow].szAlarmKey, (LPCTSTR)dlg.m_strKey, MAX_DB_STRING);
				m_pEventPolicy->pRuleList[iRow].dwAlarmTimeout = dlg.m_dwTimeout;
				m_pEventPolicy->pRuleList[iRow].dwAlarmTimeoutEvent = dlg.m_dwTimeoutEvent;
			}
			else
			{
				m_pEventPolicy->pRuleList[iRow].wAlarmSeverity = SEVERITY_TERMINATE;
				nx_strncpy(m_pEventPolicy->pRuleList[iRow].szAlarmKey, (LPCTSTR)dlg.m_strAckKey, MAX_DB_STRING);
				if (dlg.m_useRegexp)
					m_pEventPolicy->pRuleList[iRow].dwFlags |= RF_TERMINATE_BY_REGEXP;
				else
					m_pEventPolicy->pRuleList[iRow].dwFlags &= ~RF_TERMINATE_BY_REGEXP;
			}
      }
      else
      {
         m_pEventPolicy->pRuleList[iRow].dwFlags &= ~RF_GENERATE_ALARM;
      }
      Modify();
      UpdateRow(iRow);
   }
}


//
// Edit situation cell
//

void CEventPolicyEditor::EditSituation(int row)
{
   CRuleSituationDlg dlg;

	dlg.m_dwSituation = m_pEventPolicy->pRuleList[row].dwSituationId;
	dlg.m_strInstance = m_pEventPolicy->pRuleList[row].szSituationInstance;
	dlg.m_attrList = *m_pEventPolicy->pRuleList[row].pSituationAttrList;
   if (dlg.DoModal() == IDOK)
   {
		m_pEventPolicy->pRuleList[row].dwSituationId = dlg.m_dwSituation;
		nx_strncpy(m_pEventPolicy->pRuleList[row].szSituationInstance, (LPCTSTR)dlg.m_strInstance, MAX_DB_STRING);
		*m_pEventPolicy->pRuleList[row].pSituationAttrList = dlg.m_attrList;
      Modify();
      UpdateRow(row);
   }
}


//
// Edit comment cell
//

void CEventPolicyEditor::EditComment(int iRow)
{
   CRuleCommentDlg dlg;

   if (m_pEventPolicy->pRuleList[iRow].pszComment != NULL)
      dlg.m_strText = m_pEventPolicy->pRuleList[iRow].pszComment;
   if (dlg.DoModal() == IDOK)
   {
      safe_free(m_pEventPolicy->pRuleList[iRow].pszComment);
      m_pEventPolicy->pRuleList[iRow].pszComment = _tcsdup((LPCTSTR)dlg.m_strText);
      Modify();
      UpdateRow(iRow);
   }
}


//
// Edit script cell
//

void CEventPolicyEditor::EditScript(int iRow)
{
   CRuleScriptDlg dlg;

   if (m_pEventPolicy->pRuleList[iRow].pszScript != NULL)
      dlg.m_strText = m_pEventPolicy->pRuleList[iRow].pszScript;
   if (dlg.DoModal() == IDOK)
   {
      safe_free(m_pEventPolicy->pRuleList[iRow].pszScript);
      m_pEventPolicy->pRuleList[iRow].pszScript = _tcsdup((LPCTSTR)dlg.m_strText);
      Modify();
      UpdateRow(iRow);
   }
}


//
// Edit options cell
//

void CEventPolicyEditor::EditOptions(int row)
{
   CRuleOptionsDlg dlg;

   dlg.m_bStopProcessing = (m_pEventPolicy->pRuleList[row].dwFlags & RF_STOP_PROCESSING) ? TRUE : FALSE;
   if (dlg.DoModal() == IDOK)
   {
		if (dlg.m_bStopProcessing)
			m_pEventPolicy->pRuleList[row].dwFlags |= RF_STOP_PROCESSING;
		else
			m_pEventPolicy->pRuleList[row].dwFlags &= ~RF_STOP_PROCESSING;
      Modify();
      UpdateRow(row);
   }
}


//
// Add new action to rule
//

void CEventPolicyEditor::AddAction()
{
   CActionSelDlg dlg;
   DWORD i, j;
   int iRow;

   if (dlg.DoModal() == IDOK)
   {
      iRow = m_wndRuleList.GetCurrentRow();
      for(i = 0; i < dlg.m_dwNumActions; i++)
      {
         // Check if object already in the list
         for(j = 0; j < m_pEventPolicy->pRuleList[iRow].dwNumActions; j++)
            if (m_pEventPolicy->pRuleList[iRow].pdwActionList[j] == dlg.m_pdwActionList[i])
               break;
         if (j == m_pEventPolicy->pRuleList[iRow].dwNumActions)
         {
            // New object, add it to source list
            m_pEventPolicy->pRuleList[iRow].dwNumActions++;
            m_pEventPolicy->pRuleList[iRow].pdwActionList = 
               (UINT32 *)realloc(m_pEventPolicy->pRuleList[iRow].pdwActionList,
                  sizeof(UINT32) * m_pEventPolicy->pRuleList[iRow].dwNumActions);
            m_pEventPolicy->pRuleList[iRow].pdwActionList[j] = dlg.m_pdwActionList[i];
            Modify();
         }
      }
      UpdateRow(iRow);
      Modify();
   }
}


//
// Modify frame title when policy was modified
//

void CEventPolicyEditor::ModifyTitle()
{
   SetTitle(GetTitle() + _T(" (Modified)")); 
   OnUpdateFrameTitle(TRUE);
}


//
// WM_COMMAND::ID_POLICY_SAVE message handler
//

void CEventPolicyEditor::OnPolicySave() 
{
   DWORD dwResult;

   dwResult = DoRequestArg2(NXCSaveEventPolicy, g_hSession, m_pEventPolicy, _T("Saving event processing policy..."));
   if (dwResult != RCC_SUCCESS)
   {
      theApp.ErrorBox(dwResult, _T("Error saving event processing policy: %s"));
   }
   else
   {
      TCHAR szBuffer[256], *ptr;

      m_bIsModified = FALSE;
      ::LoadString(theApp.m_hInstance, IDR_EPP_EDITOR, szBuffer, 256);
      ptr = _tcschr(&szBuffer[1], _T('\n'));
      if (ptr != NULL)
         *ptr = 0;
      SetTitle(&szBuffer[1]);
      OnUpdateFrameTitle(TRUE);
   }
}


//
// Handler for double click inside rule list
//

void CEventPolicyEditor::OnRuleListDblClk(LPNMHDR pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_POLICY_EDIT, 0);
}


//
// WM_COMMAND::ID_POLICY_NEGATECELL message handler
//

void CEventPolicyEditor::OnPolicyNegatecell() 
{
   int nRow;

   nRow = m_wndRuleList.GetCurrentRow();
   switch(m_wndRuleList.GetCurrentColumn())
   {
      case COL_SOURCE:
         if (m_pEventPolicy->pRuleList[nRow].dwFlags & RF_NEGATED_SOURCE)
         {
            m_pEventPolicy->pRuleList[nRow].dwFlags &= ~RF_NEGATED_SOURCE;
            m_wndRuleList.SetNegationFlag(nRow, COL_SOURCE, FALSE);
         }
         else
         {
            m_pEventPolicy->pRuleList[nRow].dwFlags |= RF_NEGATED_SOURCE;
            m_wndRuleList.SetNegationFlag(nRow, COL_SOURCE, TRUE);
         }
         Modify();
         break;
      case COL_EVENT:
         if (m_pEventPolicy->pRuleList[nRow].dwFlags & RF_NEGATED_EVENTS)
         {
            m_pEventPolicy->pRuleList[nRow].dwFlags &= ~RF_NEGATED_EVENTS;
            m_wndRuleList.SetNegationFlag(nRow, COL_EVENT, FALSE);
         }
         else
         {
            m_pEventPolicy->pRuleList[nRow].dwFlags |= RF_NEGATED_EVENTS;
            m_wndRuleList.SetNegationFlag(nRow, COL_EVENT, TRUE);
         }
         Modify();
         break;
      default:
         break;
   }
}


//
// Copy selected rules
//

void CEventPolicyEditor::CopySelectedRules()
{
   int row;

	NXCDestroyEventPolicy(m_clipboard);
	m_clipboard = NXCCreateEmptyEventPolicy();

   row = m_wndRuleList.GetNextRow(-1, RLF_SELECTED);
   while(row != -1)
   {
		NXCAddPolicyRule(m_clipboard, NXCCopyEventPolicyRule(&m_pEventPolicy->pRuleList[row]), TRUE);
      row = m_wndRuleList.GetNextRow(row, RLF_SELECTED);
   }
}

//
// WM_COMMAND::ID_POLICY_COPY message handlers
//

void CEventPolicyEditor::OnPolicyCopy() 
{
	CopySelectedRules();
}

void CEventPolicyEditor::OnUpdatePolicyCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_wndRuleList.GetSelectionCount() > 0);
}


//
// WM_COMMAND::ID_POLICY_CUT message handlers
//

void CEventPolicyEditor::OnPolicyCut() 
{
	CopySelectedRules();
	DeleteSelectedRules();
}

void CEventPolicyEditor::OnUpdatePolicyCut(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_wndRuleList.GetSelectionCount() > 0);
}


//
// WM_COMMAND::ID_POLICY_PASTE message handlers
//

void CEventPolicyEditor::OnPolicyPaste() 
{
	int i, base;
	TCHAR buffer[32];

	if (m_clipboard == NULL)
		return;

	base = m_wndRuleList.GetCurrentRow();
	if (base == -1)
		base = (int)m_pEventPolicy->dwNumRules;

   // Extend rule list
   m_pEventPolicy->dwNumRules += m_clipboard->dwNumRules;
   m_pEventPolicy->pRuleList = (NXC_EPP_RULE *)realloc(m_pEventPolicy->pRuleList,
         sizeof(NXC_EPP_RULE) * m_pEventPolicy->dwNumRules);
   if (base < (int)(m_pEventPolicy->dwNumRules - m_clipboard->dwNumRules))
      memmove(&m_pEventPolicy->pRuleList[base + (int)m_clipboard->dwNumRules],
		        &m_pEventPolicy->pRuleList[base],
              sizeof(NXC_EPP_RULE) * ((int)(m_pEventPolicy->dwNumRules - m_clipboard->dwNumRules) - base));

   // Copy data to new rows, renumber them, and insert into rule list view
	for(i = 0; i < (int)m_clipboard->dwNumRules; i++)
	{
		NXCCopyEventPolicyRuleToBuffer(&m_pEventPolicy->pRuleList[i + base], &m_clipboard->pRuleList[i]);
		m_pEventPolicy->pRuleList[i + base].dwId = i + base;
	   m_wndRuleList.InsertRow(i + base);
		UpdateRow(i + base);
	}

   // Renumber all rows after new
   for(i = base + (int)m_clipboard->dwNumRules; i < (int)m_pEventPolicy->dwNumRules; i++)
   {
		m_pEventPolicy->pRuleList[i].dwId = i;
      _sntprintf_s(buffer, 32, _TRUNCATE, _T("%d"), i + 1);
      m_wndRuleList.ReplaceItem(i, 0, 0, buffer);
   }

   Modify();
}

void CEventPolicyEditor::OnUpdatePolicyPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_clipboard != NULL); 
}
