#if !defined(AFX_ACTIONSELDLG_H__79C67B60_B619_4AE0_A40D_82D153048479__INCLUDED_)
#define AFX_ACTIONSELDLG_H__79C67B60_B619_4AE0_A40D_82D153048479__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CActionSelDlg dialog

class CActionSelDlg : public CDialog
{
// Construction
public:
	DWORD *m_pdwActionList;
	DWORD m_dwNumActions;
	CActionSelDlg(CWnd* pParent = NULL);   // standard constructor
   virtual ~CActionSelDlg();

// Dialog Data
	//{{AFX_DATA(CActionSelDlg)
	enum { IDD = IDD_SELECT_ACTION };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActionSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CActionSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkListActions(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONSELDLG_H__79C67B60_B619_4AE0_A40D_82D153048479__INCLUDED_)
