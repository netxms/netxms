#if !defined(AFX_MAPSELDLG_H__0DFC478A_5189_4146_909D_6060BE4C109D__INCLUDED_)
#define AFX_MAPSELDLG_H__0DFC478A_5189_4146_909D_6060BE4C109D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapSelDlg dialog

class CMapSelDlg : public CDialog
{
// Construction
public:
	DWORD m_dwMapId;
	CMapSelDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMapSelDlg)
	enum { IDD = IDD_SELECT_MAP };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMapSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkListMaps(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPSELDLG_H__0DFC478A_5189_4146_909D_6060BE4C109D__INCLUDED_)
