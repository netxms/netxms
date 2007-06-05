#if !defined(AFX_NODELASTVALUESVIEW_H__501CA1B3_77F4_4563_B333_778F50ACB686__INCLUDED_)
#define AFX_NODELASTVALUESVIEW_H__501CA1B3_77F4_4563_B333_778F50ACB686__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodeLastValuesView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodeLastValuesView window

class CNodeLastValuesView : public CWnd
{
// Construction
public:
	CNodeLastValuesView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodeLastValuesView)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNodeLastValuesView();

	// Generated message map functions
protected:
	Queue m_workerQueue;
	THREAD m_hWorkerThread;
	int m_nTimer;
	NXC_OBJECT * m_pObject;
	int m_iSortImageBase;
	int m_iSortDir;
	int m_iSortMode;
	CImageList m_imageList;
	CListCtrl m_wndListCtrl;
	//{{AFX_MSG(CNodeLastValuesView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
   afx_msg void OnSetObject(WPARAM wParam, NXC_OBJECT *pObject);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODELASTVALUESVIEW_H__501CA1B3_77F4_4563_B333_778F50ACB686__INCLUDED_)
