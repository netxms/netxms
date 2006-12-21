#if !defined(AFX_CREATEMPDLG_H__95DE362E_813B_45AE_B3EA_95B9409C34B4__INCLUDED_)
#define AFX_CREATEMPDLG_H__95DE362E_813B_45AE_B3EA_95B9409C34B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateMPDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCreateMPDlg dialog

class CCreateMPDlg : public CDialog
{
// Construction
public:
	DWORD * m_pdwTemplateList;
	DWORD * m_pdwTrapList;
	DWORD * m_pdwEventList;
	DWORD m_dwNumTraps;
	DWORD m_dwNumTemplates;
	DWORD m_dwNumEvents;
	CCreateMPDlg(CWnd* pParent = NULL);   // standard constructor
   ~CCreateMPDlg();

// Dialog Data
	//{{AFX_DATA(CCreateMPDlg)
	enum { IDD = IDD_CREATE_MP };
	CTreeCtrl	m_wndTreeCtrl;
	CString	m_strDescription;
	CString	m_strFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateMPDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	NXC_TRAP_CFG_ENTRY *m_pTrapCfg;
	DWORD m_dwTrapCfgSize;
	void CreateList(HTREEITEM hRoot, DWORD *pdwCount, DWORD **ppdwList);
	void AddTemplate(DWORD dwId);
	void AddEvent(DWORD dwId);
	HTREEITEM m_hTrapRoot;
	HTREEITEM m_hTemplateRoot;
	HTREEITEM m_hEventRoot;
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CCreateMPDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAddEvent();
	afx_msg void OnButtonAddTemplate();
	afx_msg void OnButtonDelete();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATEMPDLG_H__95DE362E_813B_45AE_B3EA_95B9409C34B4__INCLUDED_)
