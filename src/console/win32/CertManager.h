#if !defined(AFX_CERTMANAGER_H__DB0EB581_BCC0_4086_AD2E_C9DA2F15B94D__INCLUDED_)
#define AFX_CERTMANAGER_H__DB0EB581_BCC0_4086_AD2E_C9DA2F15B94D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CertManager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCertManager frame

class CCertManager : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CCertManager)
protected:
	CCertManager();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCertManager)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	NXC_CERT_LIST * m_pCertList;
	CImageList m_imageList;
	int m_iSortDir;
	int m_iSortMode;
	CListCtrl m_wndListCtrl;
	virtual ~CCertManager();

	// Generated message map functions
	//{{AFX_MSG(CCertManager)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnCertificateImport();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CERTMANAGER_H__DB0EB581_BCC0_4086_AD2E_C9DA2F15B94D__INCLUDED_)
