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
// Mask for any severity match
//

#define ANY_SEVERITY (RF_SEVERITY_INFO | RF_SEVERITY_WARNING | RF_SEVERITY_MINOR | \
                      RF_SEVERITY_MAJOR | RF_SEVERITY_CRITICAL)


//
// Image codes
//

#define IMG_ANY      0
#define IMG_NODE     1
#define IMG_SUBNET   2


//
// Column numbers
//

#define COL_RULE      0
#define COL_SOURCE    1
#define COL_EVENT     2
#define COL_SEVERITY  3
#define COL_ALARM     4
#define COL_ACTION    5
#define COL_COMMENT   6


/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor

IMPLEMENT_DYNCREATE(CEventPolicyEditor, CMDIChildWnd)

CEventPolicyEditor::CEventPolicyEditor()
{
   m_pEventPolicy = theApp.m_pEventPolicy;
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
   LoadBitmapIntoList(pImageList, IDB_PSYM_ANY, PSYM_MASK_COLOR);
   LoadBitmapIntoList(pImageList, IDB_PSYM_NODE, PSYM_MASK_COLOR);
   LoadBitmapIntoList(pImageList, IDB_PSYM_SUBNET, PSYM_MASK_COLOR);
	
   // Create rule list control
   GetClientRect(&rect);
   m_wndRuleList.Create(WS_CHILD | WS_VISIBLE, rect, this, ID_RULE_LIST);
   m_wndRuleList.SetImageList(pImageList);

   // Setup columns
   m_wndRuleList.InsertColumn(0, "No.", 35, CF_CENTER | CF_TITLE_COLOR | CF_NON_SELECTABLE);
   m_wndRuleList.InsertColumn(1, "Source", 150);
   m_wndRuleList.InsertColumn(2, "Event", 150);
   m_wndRuleList.InsertColumn(3, "Severity", 80);
   m_wndRuleList.InsertColumn(4, "Alarm", 150);
   m_wndRuleList.InsertColumn(5, "Action", 150);
   m_wndRuleList.InsertColumn(6, "Comments", 200, CF_NON_SELECTABLE);

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
   int iMenu;

   switch(m_wndRuleList.GetCurrentColumn())
   {
      case 1:     // Source
      case 2:     // Event
      case 3:     // Severity
      case 5:     // Action
         iMenu = 4;
         break;
      case 4:     // Alarm
      case 6:     // Comment
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
   DWORD i;
   int iImage;

   // Rule number
   sprintf(szBuffer, "%d", iRow + 1);
   if (m_wndRuleList.GetNumItems(iRow, COL_RULE) == 0)
      m_wndRuleList.AddItem(iRow, COL_RULE, szBuffer);
   else
      m_wndRuleList.ReplaceItem(iRow, COL_RULE, 0, szBuffer);

   // Source list
   m_wndRuleList.ClearCell(iRow, COL_SOURCE);
   if (m_pEventPolicy->pRuleList[iRow].dwNumSources == 0)
   {
      m_wndRuleList.AddItem(iRow, COL_SOURCE, "Any", IMG_ANY);
   }
   else
   {
      NXC_OBJECT *pObject;

      for(i = 0; i < m_pEventPolicy->pRuleList[iRow].dwNumSources; i++)
      {
         pObject = NXCFindObjectById(m_pEventPolicy->pRuleList[iRow].pdwSourceList[i]);
         if (pObject != NULL)
         {
            switch(pObject->iClass)
            {
               case OBJECT_NODE:
                  iImage = IMG_NODE;
                  break;
               case OBJECT_SUBNET:
                  iImage = IMG_SUBNET;
                  break;
               default:
                  iImage = -1;
                  break;
            }
            m_wndRuleList.AddItem(iRow, COL_SOURCE, pObject->szName, iImage);
         }
      }
   }
   
   // Event list
   m_wndRuleList.ClearCell(iRow, 2);
   if (m_pEventPolicy->pRuleList[iRow].dwNumEvents == 0)
   {
      m_wndRuleList.AddItem(iRow, 2, "Any", IMG_ANY);
   }
   else
   {
   }

   // Severity
   m_wndRuleList.ClearCell(iRow, 3);
   if ((m_pEventPolicy->pRuleList[iRow].dwFlags & ANY_SEVERITY) == ANY_SEVERITY)
   {
      m_wndRuleList.AddItem(iRow, 3, "Any", IMG_ANY);
   }
   else
   {
   }

   // Enable/disable selection
   m_wndRuleList.EnableCellSelection(iRow, COL_SOURCE, m_pEventPolicy->pRuleList[iRow].dwNumSources != 0);
   m_wndRuleList.EnableCellSelection(iRow, COL_EVENT, m_pEventPolicy->pRuleList[iRow].dwNumEvents != 0);
   m_wndRuleList.EnableCellSelection(iRow, COL_SEVERITY, (m_pEventPolicy->pRuleList[iRow].dwFlags & ANY_SEVERITY) != ANY_SEVERITY);
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
   while(iRow != -1)
   {
      if (bEnable)
         m_pEventPolicy->pRuleList[iRow].dwFlags &= ~RF_DISABLED;
      else
         m_pEventPolicy->pRuleList[iRow].dwFlags |= RF_DISABLED;
      m_wndRuleList.EnableRow(iRow, bEnable);
      iRow = m_wndRuleList.GetNextRow(iRow, RLF_SELECTED);
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
      case COL_SOURCE:  // Source
         AddSource();
         break;
      case COL_EVENT:  // Event
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
   int iColumn;

   // Negation available only for source, event and severity columns
   iColumn = m_wndRuleList.GetCurrentColumn();
   pCmdUI->Enable((iColumn >= COL_SOURCE) && (iColumn <= COL_SEVERITY));
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


//
// Add new source object to current row
//

void CEventPolicyEditor::AddSource()
{
   CObjectSelDlg dlg;
   DWORD i, j;
   int iRow;

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
               (DWORD *)MemReAlloc(m_pEventPolicy->pRuleList[iRow].pdwSourceList,
                  sizeof(DWORD) * m_pEventPolicy->pRuleList[iRow].dwNumSources);
            m_pEventPolicy->pRuleList[iRow].pdwSourceList[j] = dlg.m_pdwObjectList[i];
         }
      }
      UpdateRow(iRow);
   }
}


//
// Delete element
//

void CEventPolicyEditor::OnPolicyDelete() 
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
                  m_wndRuleList.ReplaceItem(iRow, iCol, 0, "Any", IMG_ANY);
                  m_wndRuleList.EnableCellSelection(iRow, iCol, FALSE);
               }
               else
               {
                  m_wndRuleList.DeleteItem(iRow, iCol, iItem);
               }
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
                  m_wndRuleList.ReplaceItem(iRow, iCol, 0, "Any", IMG_ANY);
                  m_wndRuleList.EnableCellSelection(iRow, iCol, FALSE);
               }
               else
               {
                  m_wndRuleList.DeleteItem(iRow, iCol, iItem);
               }
            }
            break;
         case COL_SEVERITY:     // Severity
            break;
      }
   }
}


//
// Delete selected rule(s)
//

void CEventPolicyEditor::OnPolicyDeleterule() 
{
   int i, iRow;
   char szBuffer[32];

   iRow = m_wndRuleList.GetNextRow(-1, RLF_SELECTED);
   while(iRow != -1)
   {
      m_wndRuleList.DeleteRow(iRow);
      NXCDeletePolicyRule(m_pEventPolicy, iRow);
      for(i = iRow; i < (int)m_pEventPolicy->dwNumRules; i++)
      {
         sprintf(szBuffer, "%d", i + 1);
         m_wndRuleList.ReplaceItem(i, 0, 0, szBuffer);
      }
      iRow = m_wndRuleList.GetNextRow(iRow - 1, RLF_SELECTED);
   }
}
