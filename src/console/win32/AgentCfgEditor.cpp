// AgentCfgEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AgentCfgEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define MAX_LINE_SIZE      4096

#define COLOR_AUTO         RGB(0, 0, 0)
#define COLOR_COMMENT      RGB(80, 127, 80)
#define COLOR_SECTION      RGB(0, 0, 192)


/////////////////////////////////////////////////////////////////////////////
// CAgentCfgEditor

IMPLEMENT_DYNCREATE(CAgentCfgEditor, CMDIChildWnd)

CAgentCfgEditor::CAgentCfgEditor()
{
   m_dwNodeId = 0;
   m_bParserActive = FALSE;
}

CAgentCfgEditor::CAgentCfgEditor(DWORD dwNodeId)
{
   m_dwNodeId = dwNodeId;
}

CAgentCfgEditor::~CAgentCfgEditor()
{
}


BEGIN_MESSAGE_MAP(CAgentCfgEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CAgentCfgEditor)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(ID_EDIT_CTRL, OnEditCtrlChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentCfgEditor message handlers

BOOL CAgentCfgEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CAgentCfgEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CHARFORMAT cf;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   m_wndEdit.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOHSCROLL | 
                    ES_AUTOVSCROLL | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL,
                    rect, this, ID_EDIT_CTRL);

   cf.cbSize = sizeof(CHARFORMAT);
   cf.dwMask = CFM_FACE | CFM_BOLD | CFM_COLOR;
   cf.dwEffects = CFE_AUTOCOLOR;
   cf.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
   _tcscpy(cf.szFaceName, _T("Courier New"));
   m_wndEdit.SetDefaultCharFormat(cf);

   m_wndEdit.SetEventMask(m_wndEdit.GetEventMask() | ENM_CHANGE);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	
	return 0;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAgentCfgEditor::OnViewRefresh() 
{
   DWORD dwResult;
   TCHAR *pszConfig;

   dwResult = DoRequestArg3(NXCGetAgentConfig, g_hSession, (void *)m_dwNodeId,
                            &pszConfig, _T("Requesting agent's configuration file..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndEdit.SetWindowText(pszConfig);
      free(pszConfig);
      ParseFile();
   }
   else
   {
      m_wndEdit.SetWindowText(_T(""));
      theApp.ErrorBox(dwResult, _T("Error getting agent's configuration file: %s"));
   }
}


//
// WM_SIZE message handler
//

void CAgentCfgEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndEdit.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CAgentCfgEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndEdit.SetFocus();	
}


//
// UI command update handlers
//

void CAgentCfgEditor::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndEdit.CanUndo());
}

void CAgentCfgEditor::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndEdit.CanPaste(CF_TEXT));
}

void CAgentCfgEditor::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_wndEdit.GetSelectionType() & (SEL_TEXT | SEL_MULTICHAR)) ? TRUE : FALSE);
}

void CAgentCfgEditor::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_wndEdit.GetSelectionType() & (SEL_TEXT | SEL_MULTICHAR)) ? TRUE : FALSE);
}


//
// WM_COMMAND::ID_EDIT_UNDO message handler
//

void CAgentCfgEditor::OnEditUndo() 
{
   m_wndEdit.Undo();
}


//
// WM_COMMAND::ID_EDIT_COPY message handler
//

void CAgentCfgEditor::OnEditCopy() 
{
   m_wndEdit.Copy();
}


//
// WM_COMMAND::ID_EDIT_CUT message handler
//

void CAgentCfgEditor::OnEditCut() 
{
   m_wndEdit.Cut();
}


//
// WM_COMMAND::ID_EDIT_DELETE message handler
//

void CAgentCfgEditor::OnEditDelete() 
{
   m_wndEdit.Clear();
}


//
// WM_COMMAND::ID_EDIT_PASTE message handler
//

void CAgentCfgEditor::OnEditPaste() 
{
   m_wndEdit.PasteSpecial(CF_TEXT);
}


//
// WM_COMMAND::ID_EDIT_SELECT_ALL message handler
//

void CAgentCfgEditor::OnEditSelectAll() 
{
   m_wndEdit.SetSel(0, -1);
}


//
// Highlight text
//

void CAgentCfgEditor::HighlightText(int nStartPos, int nEndPos, COLORREF rgbColor)
{
   CHARFORMAT cf;

   m_wndEdit.SetSel(nStartPos, nEndPos);
   cf.cbSize = sizeof(CHARFORMAT);
   cf.dwMask = CFM_COLOR;
   cf.dwEffects = (rgbColor == COLOR_AUTO) ? CFE_AUTOCOLOR : 0;
   cf.crTextColor = rgbColor;
   m_wndEdit.SetSelectionCharFormat(cf);
   m_wndEdit.SetSel(-1, -1);
}


//
// Parse entire file
//

void CAgentCfgEditor::ParseFile()
{
   int i, nLineCount;

   OnStartParsing();

   nLineCount = m_wndEdit.GetLineCount();
   for(i = 0; i < nLineCount; i++)
      ParseLine(i);

   OnStopParsing();
}


//
// Parse specific line
//

void CAgentCfgEditor::ParseLine(int nLine)
{
   int nCurrPos, nEndPos, nIndex, nElementStart = -1;
   COLORREF rgbElementColor;
   TCHAR szLine[MAX_LINE_SIZE];
   BOOL bFirstChar;

   m_wndEdit.GetLine(nLine, szLine, MAX_LINE_SIZE);
   nCurrPos = m_wndEdit.LineIndex(nLine);
   nEndPos = nCurrPos + m_wndEdit.LineLength(nCurrPos);

   // Reset line to default color
   HighlightText(nCurrPos, nEndPos, COLOR_AUTO);

   // Skip leading spaces and tabs
   for(nIndex = 0; 
       (nCurrPos < nEndPos) && 
       ((szLine[nIndex] == _T(' ')) || 
        (szLine[nIndex] == _T('\t'))); nIndex++, nCurrPos++);

   for(bFirstChar = TRUE; nCurrPos < nEndPos; nIndex++, nCurrPos++)
   {
      switch(szLine[nIndex])
      {
         case _T('#'):
            if (nElementStart != -1)
               HighlightText(nElementStart, nCurrPos - 1, rgbElementColor);
            HighlightText(nCurrPos, nEndPos, COLOR_COMMENT);
            nCurrPos = nEndPos;
            nElementStart = -1;
            break;
         case _T('*'):
            if (bFirstChar)
            {
               nElementStart = nCurrPos;
               rgbElementColor = COLOR_SECTION;
            }
            break;
         default:
            break;
      }
      bFirstChar = FALSE;
   }

   if (nElementStart != -1)
      HighlightText(nElementStart, nEndPos, rgbElementColor);
}


//
// Handle edit control updates
//

void CAgentCfgEditor::OnEditCtrlChange()
{
   int nCurrLine;

   if (!m_bParserActive)
   {
      OnStartParsing();

      nCurrLine = m_wndEdit.LineFromChar(-1);
      if (nCurrLine > 0)
         ParseLine(nCurrLine - 1);
      ParseLine(nCurrLine);
      if (nCurrLine < m_wndEdit.GetLineCount() - 1)
         ParseLine(nCurrLine + 1);

      OnStopParsing();
   }
}


//
// Handle start of parsing process
//

void CAgentCfgEditor::OnStartParsing()
{
   m_bParserActive = TRUE;
   m_wndEdit.SetEventMask(m_wndEdit.GetEventMask() & ~ENM_CHANGE);
   m_wndEdit.SetRedraw(FALSE);
   m_wndEdit.GetSel(m_crSavedSelection);
}


//
// Handle termination of parsing process
//

void CAgentCfgEditor::OnStopParsing()
{
   m_wndEdit.SetSel(m_crSavedSelection);
   m_wndEdit.SetRedraw(TRUE);
   m_wndEdit.SetEventMask(m_wndEdit.GetEventMask() | ENM_CHANGE);
   m_wndEdit.Invalidate(FALSE);
   m_bParserActive = FALSE;
}
