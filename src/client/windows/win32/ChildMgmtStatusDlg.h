#if !defined(AFX_CHILDMGMTSTATUSDLG_H__E694E8F6_3236_4CB5_9AD2_4835FE94D5BF__INCLUDED_)
#define AFX_CHILDMGMTSTATUSDLG_H__E694E8F6_3236_4CB5_9AD2_4835FE94D5BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChildMgmtStatusDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChildMgmtStatusDlg dialog

class CChildMgmtStatusDlg : public CDialog
{
// Construction
public:
	NXC_OBJECT * m_pObject;
	CChildMgmtStatusDlg(CWnd* pParent = NULL);   // standard constructor
	CComboListCtrl m_wndListCtrl;

// Dialog Data
	//{{AFX_DATA(CChildMgmtStatusDlg)
	enum { IDD = IDD_SET_CHILD_MGMT_STATUS };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildMgmtStatusDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CChildMgmtStatusDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg LRESULT OnComboListSetItems(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDMGMTSTATUSDLG_H__E694E8F6_3236_4CB5_9AD2_4835FE94D5BF__INCLUDED_)
