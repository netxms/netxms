#if !defined(AFX_GRAPHPROPDLG_H__2C17056D_6D61_4B55_9D1A_C7BE1338047F__INCLUDED_)
#define AFX_GRAPHPROPDLG_H__2C17056D_6D61_4B55_9D1A_C7BE1338047F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGraphPropDlg dialog

class CGraphPropDlg : public CDialog
{
// Construction
public:
	CGraphPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGraphPropDlg)
	enum { IDD = IDD_GRAPH_PROPERTIES };
	DWORD	m_dwRefreshInterval;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGraphPropDlg)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnCbBackground();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPROPDLG_H__2C17056D_6D61_4B55_9D1A_C7BE1338047F__INCLUDED_)
