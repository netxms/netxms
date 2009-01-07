#if !defined(AFX_SITUATIONSELDLG_H__F0279929_37FF_4017_A480_4BB96DD24731__INCLUDED_)
#define AFX_SITUATIONSELDLG_H__F0279929_37FF_4017_A480_4BB96DD24731__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SituationSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSituationSelDlg dialog

class CSituationSelDlg : public CDialog
{
// Construction
public:
	DWORD m_dwSituation;
	CSituationSelDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSituationSelDlg)
	enum { IDD = IDD_SELECT_SITUATION };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSituationSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CSituationSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkListSituations(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SITUATIONSELDLG_H__F0279929_37FF_4017_A480_4BB96DD24731__INCLUDED_)
