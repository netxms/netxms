#if !defined(AFX_OBJECTSUBORDINATEVIEW_H__AA84BE9D_F28D_4081_A7A7_012D00BDD76D__INCLUDED_)
#define AFX_OBJECTSUBORDINATEVIEW_H__AA84BE9D_F28D_4081_A7A7_012D00BDD76D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectSubordinateView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectSubordinateView window

class CObjectSubordinateView : public CWnd
{
// Construction
public:
	CObjectSubordinateView();

// Attributes
public:

// Operations
public:
	int CompareListItems(NXC_OBJECT *obj1, NXC_OBJECT *obj2);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectSubordinateView)
	//}}AFX_VIRTUAL

// Implementation
public:
	void Refresh(void);
	virtual ~CObjectSubordinateView();

	// Generated message map functions
protected:
	void DoActionOnSubordinates(int nAction);
	NXC_OBJECT * m_pObject;
	int m_iSortImageBase;
	int m_iSortDir;
	int m_iSortMode;
	CImageList m_imageList;
	CListCtrl m_wndListCtrl;
	//{{AFX_MSG(CObjectSubordinateView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSubordinateDelete();
	afx_msg void OnSubordinateManage();
	afx_msg void OnSubordinateUnmanage();
	afx_msg void OnSubordinateUnbind();
	afx_msg void OnSubordinateProperties();
	//}}AFX_MSG
   afx_msg LRESULT OnSetObject(WPARAM wParam, LPARAM lParam);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTSUBORDINATEVIEW_H__AA84BE9D_F28D_4081_A7A7_012D00BDD76D__INCLUDED_)
