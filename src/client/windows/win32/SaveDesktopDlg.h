#if !defined(AFX_SAVEDESKTOPDLG_H__8472F8DD_83D5_4DEB_B5FD_645BAB0B9576__INCLUDED_)
#define AFX_SAVEDESKTOPDLG_H__8472F8DD_83D5_4DEB_B5FD_645BAB0B9576__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaveDesktopDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaveDesktopDlg dialog

class CSaveDesktopDlg : public CDialog
{
// Construction
public:
	BOOL m_bRestore;
	CSaveDesktopDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveDesktopDlg)
	enum { IDD = IDD_DESKTOP_SAVE_AS };
	CListCtrl	m_wndListCtrl;
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveDesktopDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CSaveDesktopDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnItemchangedListDesktops(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListDesktops(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAVEDESKTOPDLG_H__8472F8DD_83D5_4DEB_B5FD_645BAB0B9576__INCLUDED_)
