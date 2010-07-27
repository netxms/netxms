#if !defined(AFX_NODELASTVALUESVIEW_H__501CA1B3_77F4_4563_B333_778F50ACB686__INCLUDED_)
#define AFX_NODELASTVALUESVIEW_H__501CA1B3_77F4_4563_B333_778F50ACB686__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodeLastValuesView.h : header file
//

#include <nxqueue.h>


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
	int CompareListItems(LPARAM nItem1, LPARAM nItem2);
	void WorkerThread(void);
	virtual ~CNodeLastValuesView();

	// Generated message map functions
protected:
	BOOL m_bUseMultipliers;
	NXC_DCI_VALUE * m_pItemList;
	DWORD m_dwNumItems;
	Queue m_workerQueue;
	THREAD m_hWorkerThread;
	UINT_PTR m_nTimer;
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
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnItemGraph();
	afx_msg void OnItemExportdata();
	afx_msg void OnItemShowdata();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLatstvaluesUsemultipliers();
	afx_msg void OnItemCleardata();
	//}}AFX_MSG
   afx_msg LRESULT OnSetObject(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRequestCompleted(WPARAM wParam, LPARAM lParam);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODELASTVALUESVIEW_H__501CA1B3_77F4_4563_B333_778F50ACB686__INCLUDED_)
