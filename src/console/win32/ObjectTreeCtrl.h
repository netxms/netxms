#if !defined(AFX_OBJECTTREECTRL_H__B0E88EBA_0FD5_4879_8E97_0728934C24A3__INCLUDED_)
#define AFX_OBJECTTREECTRL_H__B0E88EBA_0FD5_4879_8E97_0728934C24A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectTreeCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectTreeCtrl window

class CObjectTreeCtrl : public CTreeCtrl
{
// Construction
public:
	CObjectTreeCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectTreeCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectTreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CObjectTreeCtrl)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTTREECTRL_H__B0E88EBA_0FD5_4879_8E97_0728934C24A3__INCLUDED_)
