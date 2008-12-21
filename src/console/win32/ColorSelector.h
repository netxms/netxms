#if !defined(AFX_COLORSELECTOR_H__3F309585_A334_4065_9BBB_4A096DD48D05__INCLUDED_)
#define AFX_COLORSELECTOR_H__3F309585_A334_4065_9BBB_4A096DD48D05__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColorSelector.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CColorSelector window

class CColorSelector : public CWnd
{
// Construction
public:
	CColorSelector();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorSelector)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	COLORREF m_rgbColor;
	virtual ~CColorSelector();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorSelector)
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnPaint();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bMouseDown;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORSELECTOR_H__3F309585_A334_4065_9BBB_4A096DD48D05__INCLUDED_)
