#if !defined(AFX_GRAPHSTYLEPAGE_H__DEE89944_697A_4864_A7F0_2A18C3433AA5__INCLUDED_)
#define AFX_GRAPHSTYLEPAGE_H__DEE89944_697A_4864_A7F0_2A18C3433AA5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphStylePage.h : header file
//
#include "GraphFrame.h"


/////////////////////////////////////////////////////////////////////////////
// CGraphStylePage dialog

class CGraphStylePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGraphStylePage)

// Construction
public:
	GRAPH_ITEM_STYLE m_styles[MAX_GRAPH_ITEMS];
	CGraphStylePage();
	~CGraphStylePage();

// Dialog Data
	//{{AFX_DATA(CGraphStylePage)
	enum { IDD = IDD_GRAPH_PROP_STYLES };
	//}}AFX_DATA
	CComboListCtrl m_wndListCtrl;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGraphStylePage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void DrawTextCell(int nSubItem, LPDRAWITEMSTRUCT lpDrawItemStruct);
	// Generated message map functions
	//{{AFX_MSG(CGraphStylePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	afx_msg LRESULT OnComboListSetItems(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHSTYLEPAGE_H__DEE89944_697A_4864_A7F0_2A18C3433AA5__INCLUDED_)
