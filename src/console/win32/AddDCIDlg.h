#if !defined(AFX_ADDDCIDLG_H__0F6F8D79_E8C3_4E45_A29D_5A1FC3FF0DD4__INCLUDED_)
#define AFX_ADDDCIDLG_H__0F6F8D79_E8C3_4E45_A29D_5A1FC3FF0DD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddDCIDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddDCIDlg dialog

class CAddDCIDlg : public CDialog
{
// Construction
public:
	CString m_strItemName;
	DWORD m_dwItemId;
	DWORD m_dwNodeId;
	CAddDCIDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddDCIDlg)
	enum { IDD = IDD_ADD_DCI };
	CListCtrl	m_wndListDCI;
	CListCtrl	m_wndListNodes;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddDCIDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CAddDCIDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedListNodes(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDDCIDLG_H__0F6F8D79_E8C3_4E45_A29D_5A1FC3FF0DD4__INCLUDED_)
