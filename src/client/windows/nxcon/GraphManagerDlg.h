#if !defined(AFX_GRAPHMANAGERDLG_H__027BBA60_60D2_41CC_8E30_ACD183114302__INCLUDED_)
#define AFX_GRAPHMANAGERDLG_H__027BBA60_60D2_41CC_8E30_ACD183114302__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphManagerDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGraphManagerDlg dialog

class CGraphManagerDlg : public CDialog
{
// Construction
public:
	CGraphManagerDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGraphManagerDlg)
	enum { IDD = IDD_MANAGE_GRAPHS };
	CTreeCtrl	m_wndTreeCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphManagerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL DeleteGraph(HTREEITEM hItem);
	void CreateGraphTree(HTREEITEM hRoot, TCHAR *pszCurrPath, DWORD *pdwStart);
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CGraphManagerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemexpandedTreeGraphs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnButtonDelete();
	afx_msg void OnGraphmanagerDelete();
	afx_msg void OnButtonProperties();
	afx_msg void OnButtonSettings();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHMANAGERDLG_H__027BBA60_60D2_41CC_8E30_ACD183114302__INCLUDED_)
