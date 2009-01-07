#if !defined(AFX_EDITBOX_H__C9AF5371_49AF_4379_BB9B_39D77BEC1471__INCLUDED_)
#define AFX_EDITBOX_H__C9AF5371_49AF_4379_BB9B_39D77BEC1471__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditBox.h : header file
//


//
// Events
//

#define EDITBOX_ENTER_PRESSED    1


/////////////////////////////////////////////////////////////////////////////
// CEditBox window

class CEditBox : public CEdit
{
// Construction
public:
	CEditBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditBox)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITBOX_H__C9AF5371_49AF_4379_BB9B_39D77BEC1471__INCLUDED_)
