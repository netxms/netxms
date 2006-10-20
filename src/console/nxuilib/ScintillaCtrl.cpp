// ScintillaCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "nxuilib.h"
#include "ScintillaCtrl.h"
#include "Scintilla.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScintillaCtrl

CScintillaCtrl::CScintillaCtrl()
{
}

CScintillaCtrl::~CScintillaCtrl()
{
}


BEGIN_MESSAGE_MAP(CScintillaCtrl, CWnd)
	//{{AFX_MSG_MAP(CScintillaCtrl)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CScintillaCtrl message handlers


//
// Create control
//

BOOL CScintillaCtrl::Create(LPCTSTR lpszWindowName, DWORD dwStyle,
                            const RECT &rect, CWnd *pParentWnd, UINT nID, DWORD dwExStyle)
{
   if (!CWnd::CreateEx(dwExStyle, _T("Scintilla"), lpszWindowName,
                       dwStyle, rect, pParentWnd, nID))
      return FALSE;

   SetDefaults();
   return TRUE;
}


//
// Set text in control
//

void CScintillaCtrl::SetText(LPCTSTR lpszText)
{
   if (lpszText != NULL)
   {
#ifdef _UNICODE
      char *pBuffer;
      int nSize;

      nSize = wcslen(lpszText) * 4 + 1;
      pBuffer = (char *)malloc(nSize);
      WideCharToMultiByte(CP_UTF8, 0, lpszText, -1, pBuffer, nSize, NULL, NULL);
      SendMessage(SCI_SETTEXT, 0, (LPARAM)pBuffer);
      free(pBuffer);
#else
      SendMessage(SCI_SETTEXT, 0, (LPARAM)lpszText);
#endif
   }
   GotoPosition(0);
}


//
// Get text from control
//

void CScintillaCtrl::GetText(CString &strText)
{
   LONG nLen;
   char *pszText;
#ifdef _UNICODE
   WCHAR *pszUniText;
#endif

   nLen = SendMessage(SCI_GETLENGTH, 0, 0) + 1;
   if (nLen > 0)
   {
      pszText = (char *)malloc(nLen);
      *pszText = 0;
      SendMessage(SCI_GETTEXT, nLen, (LPARAM)pszText);
#ifdef _UNICODE
      pszUniText = (WCHAR *)malloc(nLen * sizeof(WCHAR));
      MultiByteToWideChar(CP_UTF8, 0, pszText, -1, pszUniText, nLen);
      strText = pszUniText;
      free(pszUniText);
#else
      strText = pszText;
#endif
      free(pszText);
   }
   else
   {
      strText = _T("");
   }
}


//
// Cut selected text to clipboard
//

void CScintillaCtrl::Cut()
{
   SendMessage(SCI_CUT, 0, 0);
}


//
// Copy selected text to clipboard
//

void CScintillaCtrl::Copy()
{
   SendMessage(SCI_COPY, 0, 0);
}


//
// Paste text from clipboard
//

void CScintillaCtrl::Paste()
{
   SendMessage(SCI_PASTE, 0, 0);
}


//
// Clear selection
//

void CScintillaCtrl::Clear()
{
   SendMessage(SCI_CLEAR, 0, 0);
}


//
// Select all text
//

void CScintillaCtrl::SelectAll()
{
   SendMessage(SCI_SELECTALL, 0, 0);
}


//
// Undo operation
//

void CScintillaCtrl::Undo()
{
   SendMessage(SCI_UNDO, 0, 0);
}


//
// Redo operation
//

void CScintillaCtrl::Redo()
{
   SendMessage(SCI_REDO, 0, 0);
}


//
// Returns a flag if we can undo the last action
//

BOOL CScintillaCtrl::CanUndo()
{
   return SendMessage(SCI_CANUNDO, 0, 0) != 0;
}


//
// Returns a flag if we can redo the last action
//

BOOL CScintillaCtrl::CanRedo()
{
   return SendMessage(SCI_CANREDO, 0, 0) != 0;
}


//
// Returns a flag if there is text in the clipboard which we can paste
//

BOOL CScintillaCtrl::CanPaste()
{
   return SendMessage(SCI_CANPASTE, 0, 0) != 0;
}


//
// Goto specific position
//

void CScintillaCtrl::GotoPosition(LONG nPos)
{
	SendMessage(SCI_GOTOPOS, nPos, 0);
}


//
// Load external lexer module
//

BOOL CScintillaCtrl::LoadLexer(char *pszModule)
{
   SendMessage(SCI_LOADLEXERLIBRARY, 0, (LPARAM)pszModule);
   return TRUE;
}


//
// Set lexer by name
//

BOOL CScintillaCtrl::SetLexer(char *pszLexerName)
{
   int nLexer;

   nLexer = SendMessage(SCI_GETLEXER, 0, 0);
   SendMessage(SCI_SETLEXERLANGUAGE, 0, (LPARAM)pszLexerName);
   return (nLexer != SendMessage(SCI_GETLEXER, 0, 0));
}


//
// Refresh content
//

void CScintillaCtrl::Refresh(void)
{
   SendMessage(SCI_COLOURISE, 0, -1);
}


//
// Get modification flag
//

BOOL CScintillaCtrl::GetModify(void)
{
   return SendMessage(SCI_GETMODIFY, 0, 0);
}


//
// Set keyword list
//

void CScintillaCtrl::SetKeywords(int nSet, char *pszKeywordList)
{
   SendMessage(SCI_SETKEYWORDS, nSet, (LPARAM)pszKeywordList);
}


//
// Mark document as unmodified (saved)
//

void CScintillaCtrl::SetSavePoint()
{
   SendMessage(SCI_SETSAVEPOINT, 0, 0);
}


//
// Empty undo/redo buffer
//

void CScintillaCtrl::EmptyUndoBuffer()
{
   SendMessage(SCI_EMPTYUNDOBUFFER, 0, 0);
}


//
// Set default settings
//

void CScintillaCtrl::SetDefaults(void)
{
   // Set UNICODE mode
#ifdef _UNICODE
   SendMessage(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
#else
   SendMessage(SCI_SETCODEPAGE, 0, 0);
#endif

   // Default style
   SendMessage(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)"Courier New");
   SendMessage(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
   SendMessage(SCI_STYLECLEARALL, 0, 0);

   // Set tab width and indentation to 3 characters
   SendMessage(SCI_SETTABWIDTH, 3, 0);
	SendMessage(SCI_SETINDENT, 3, 0);
	SendMessage(SCI_SETINDENTATIONGUIDES, FALSE, 0);

   // Set end of line mode to CRLF
   SendMessage(SCI_CONVERTEOLS, 2, 0);
   SendMessage(SCI_SETEOLMODE, 2, 0);

   // Set default styles
   SendMessage(SCI_STYLESETFORE, NX_STYLE_DEFAULT, RGB(0, 0, 0));

   SendMessage(SCI_STYLESETFORE, NX_STYLE_COMMENT, RGB(128, 128, 128));
   //SendMessage(SCI_STYLESETITALIC, NX_STYLE_COMMENT, TRUE);

   SendMessage(SCI_STYLESETFORE, NX_STYLE_ML_COMMENT, RGB(128, 128, 128));
   //SendMessage(SCI_STYLESETITALIC, NX_STYLE_ML_COMMENT, TRUE);

   SendMessage(SCI_STYLESETFORE, NX_STYLE_KEYWORD, RGB(0, 0, 92));
   SendMessage(SCI_STYLESETBOLD, NX_STYLE_KEYWORD, TRUE);

   SendMessage(SCI_STYLESETFORE, NX_STYLE_SYMBOL, RGB(0, 0, 0));
   //SendMessage(SCI_STYLESETBACK, NX_STYLE_SYMBOL, RGB(0, 255, 0));
   SendMessage(SCI_STYLESETBOLD, NX_STYLE_SYMBOL, TRUE);

   SendMessage(SCI_STYLESETFORE, NX_STYLE_STRING, RGB(0, 0, 192));

   SendMessage(SCI_STYLESETFORE, NX_STYLE_CONSTANT, RGB(0, 0, 192));

   SendMessage(SCI_STYLESETFORE, NX_STYLE_ERROR, RGB(255, 255, 255));
   SendMessage(SCI_STYLESETBACK, NX_STYLE_ERROR, RGB(192, 0, 0));

   SendMessage(SCI_STYLESETFORE, NX_STYLE_SECTION, RGB(255, 255, 255));
   SendMessage(SCI_STYLESETBACK, NX_STYLE_SECTION, RGB(128, 128, 255));
   SendMessage(SCI_STYLESETBOLD, NX_STYLE_SECTION, TRUE);
   SendMessage(SCI_STYLESETEOLFILLED, NX_STYLE_SECTION, TRUE);

   // Selection colors
   SendMessage(SCI_SETSELFORE, TRUE, RGB(255, 255, 255));
   SendMessage(SCI_SETSELBACK, TRUE, RGB(0, 0, 0));

   // Other settings
   SendMessage(SCI_USEPOPUP, 0, 0);
}
