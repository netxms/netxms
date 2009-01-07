#if !defined(AFX_DISCOVERYPROPGENERAL_H__E0B511BD_B88C_436C_95F6_84465D1A671D__INCLUDED_)
#define AFX_DISCOVERYPROPGENERAL_H__E0B511BD_B88C_436C_95F6_84465D1A671D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiscoveryPropGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropGeneral dialog

class CDiscoveryPropGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CDiscoveryPropGeneral)

// Construction
public:
	CDiscoveryPropGeneral();
	~CDiscoveryPropGeneral();

// Dialog Data
	//{{AFX_DATA(CDiscoveryPropGeneral)
	enum { IDD = IDD_DISCOVERY_GENERAL };
	BOOL	m_bAllowAgent;
	BOOL	m_bAllowRange;
	BOOL	m_bAllowSNMP;
	CString	m_strScript;
	CString	m_strCommunity;
	int		m_nMode;
	int		m_nFilter;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDiscoveryPropGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnFilterSelection(void);
	void OnModeSelection(void);
	// Generated message map functions
	//{{AFX_MSG(CDiscoveryPropGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioNone();
	afx_msg void OnRadioPassive();
	afx_msg void OnRadioActive();
	afx_msg void OnRadioDisabled();
	afx_msg void OnRadioAuto();
	afx_msg void OnRadioCustom();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISCOVERYPROPGENERAL_H__E0B511BD_B88C_436C_95F6_84465D1A671D__INCLUDED_)
