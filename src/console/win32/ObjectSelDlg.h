#if !defined(AFX_OBJECTSELDLG_H__79C83A28_C41C_49C1_A2B3_8EC4388579A5__INCLUDED_)
#define AFX_OBJECTSELDLG_H__79C83A28_C41C_49C1_A2B3_8EC4388579A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectSelDlg dialog

class CObjectSelDlg : public CDialog
{
// Construction
public:
	DWORD m_dwNumObjects;
	DWORD *m_pdwObjectList;
	CObjectSelDlg(CWnd* pParent = NULL);   // standard constructor
   virtual ~CObjectSelDlg();

// Dialog Data
	//{{AFX_DATA(CObjectSelDlg)
	enum { IDD = IDD_SELECT_OBJECT };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjectSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkListObjects(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CImageList m_imageList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTSELDLG_H__79C83A28_C41C_49C1_A2B3_8EC4388579A5__INCLUDED_)
