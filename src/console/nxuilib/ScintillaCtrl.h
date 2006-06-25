#if !defined(AFX_SCINTILLACTRL_H__F9DB945E_AEB7_4699_BBB0_283B0A1FAA30__INCLUDED_)
#define AFX_SCINTILLACTRL_H__F9DB945E_AEB7_4699_BBB0_283B0A1FAA30__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ScintillaCtrl.h : header file
//

#include <nxlexer_styles.h>


/////////////////////////////////////////////////////////////////////////////
// CScintillaCtrl window

class NXUILIB_EXPORTABLE CScintillaCtrl : public CWnd
{
// Construction
public:
	CScintillaCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScintillaCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetDefaults(void);
	void EmptyUndoBuffer(void);
	void SetSavePoint(void);
	void SetKeywords(int nSet, TCHAR *pszKeywordList);
	BOOL GetModify(void);
	void Refresh(void);
	BOOL SetLexer(TCHAR *pszLexerName);
	BOOL LoadLexer(TCHAR *pszModule);
	void GotoPosition(LONG nPos);
	BOOL CanPaste(void);
	BOOL CanRedo(void);
	BOOL CanUndo(void);
	void Redo(void);
	void Undo(void);
	void SelectAll(void);
	void Clear(void);
	void Paste(void);
	void Copy(void);
	void Cut(void);
	void GetText(CString &strText);
	void SetText(LPCSTR lpszText);
	BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT &rect,
               CWnd *pParentWnd, UINT nID, DWORD dwExStyle = 0);
	virtual ~CScintillaCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CScintillaCtrl)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCINTILLACTRL_H__F9DB945E_AEB7_4699_BBB0_283B0A1FAA30__INCLUDED_)
