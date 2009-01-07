#if !defined(AFX_MAPLINKPROPDLG_H__536B7284_C804_4C55_B4EB_9EB358F165AD__INCLUDED_)
#define AFX_MAPLINKPROPDLG_H__536B7284_C804_4C55_B4EB_9EB358F165AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapLinkPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapLinkPropDlg dialog

class CMapLinkPropDlg : public CDialog
{
// Construction
public:
	DWORD m_dwParentObject;
	DWORD m_dwObject2;
	DWORD m_dwObject1;
	CMapLinkPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMapLinkPropDlg)
	enum { IDD = IDD_MAP_LINK };
	CString	m_strPort1;
	CString	m_strPort2;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapLinkPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMapLinkPropDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelectObject1();
	afx_msg void OnSelectObject2();
	afx_msg void OnSelectPort1();
	afx_msg void OnSelectPort2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPLINKPROPDLG_H__536B7284_C804_4C55_B4EB_9EB358F165AD__INCLUDED_)
