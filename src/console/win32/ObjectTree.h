#if !defined(AFX_OBJECTTREE_H__CED434DA_525B_41C8_8157_CB743D5045AC__INCLUDED_)
#define AFX_OBJECTTREE_H__CED434DA_525B_41C8_8157_CB743D5045AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectTree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectTree window

class CObjectTree : public CWnd
{
// Construction
public:
	CObjectTree();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectTree)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectTree();

	// Generated message map functions
protected:
	CImageList m_imageList;
	CTreeCtrl *m_pwndTreeCtrl;
	//{{AFX_MSG(CObjectTree)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL m_bHideNonOperational;
	BOOL m_bHideUnknown;
	BOOL m_bHideUnmanaged;
	BOOL m_bHideNormal;
	BOOL m_bUseIcons;
	void SetTreeCtrl(CTreeCtrl *pCtrl) { m_pwndTreeCtrl = pCtrl; }
private:
	BOOL IsStatusVisible(int nStatus);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTTREE_H__CED434DA_525B_41C8_8157_CB743D5045AC__INCLUDED_)
