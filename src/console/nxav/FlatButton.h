#if !defined(AFX_FLATBUTTON_H__83895B89_BE46_484C_A33C_B728C78AAFD7__INCLUDED_)
#define AFX_FLATBUTTON_H__83895B89_BE46_484C_A33C_B728C78AAFD7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FlatButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFlatButton window

class CFlatButton : public CWnd
{
// Construction
public:
	CFlatButton();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFlatButton)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFlatButton();

	// Generated message map functions
protected:
	BOOL m_bMouseHover;
	void SetMouseTracking(void);
	CFont m_fontNormal;
	CFont m_fontFocused;
	//{{AFX_MSG(CFlatButton)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
   afx_msg int OnMouseHover(WPARAM wParam, LPARAM lParam);
   afx_msg int OnMouseLeave(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FLATBUTTON_H__83895B89_BE46_484C_A33C_B728C78AAFD7__INCLUDED_)
