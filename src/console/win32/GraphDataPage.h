#if !defined(AFX_GRAPHDATAPAGE_H__02985AAF_9B98_42A1_84D1_C44757B0AB80__INCLUDED_)
#define AFX_GRAPHDATAPAGE_H__02985AAF_9B98_42A1_84D1_C44757B0AB80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphDataPage.h : header file
//
#include "GraphFrame.h"

/////////////////////////////////////////////////////////////////////////////
// CGraphDataPage dialog

class CGraphDataPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGraphDataPage)

// Construction
public:
	int CompareListItems(int nItem1, int nItem2);
	DWORD m_dwNumItems;
   DCIInfo *m_ppItems[MAX_GRAPH_ITEMS];
	CGraphDataPage();
	~CGraphDataPage();

// Dialog Data
	//{{AFX_DATA(CGraphDataPage)
	enum { IDD = IDD_GRAPH_PROP_DATA };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGraphDataPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_nSortDir;
	int m_nSortMode;
	CImageList m_imageList;
	void AddListItem(DWORD dwIndex);
	// Generated message map functions
	//{{AFX_MSG(CGraphDataPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonAdd();
	afx_msg void OnColumnclickListDci(LPNMHDR pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedListDci(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonDown();
	afx_msg void OnButtonUp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHDATAPAGE_H__02985AAF_9B98_42A1_84D1_C44757B0AB80__INCLUDED_)
