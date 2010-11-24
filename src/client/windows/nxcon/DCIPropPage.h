#if !defined(AFX_DCIPROPPAGE_H__45FF1964_0A50_43D7_9F4A_BDFB03DD7A4C__INCLUDED_)
#define AFX_DCIPROPPAGE_H__45FF1964_0A50_43D7_9F4A_BDFB03DD7A4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCIPropPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCIPropDlg dialog

class CDCIPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDCIPropPage)

// Construction
public:
	DWORD m_dwProxyNode;
	DWORD m_dwResourceId;
	NXC_OBJECT *m_pNode;
	CDCIPropPage();   // standard constructor
   virtual ~CDCIPropPage();

// Dialog Data
	//{{AFX_DATA(CDCIPropPage)
	enum { IDD = IDD_DCI_COLLECTION };
	CButton	m_wndCheckCustomPort;
	CComboBox	m_wndComboResources;
	CEdit	m_wndEditName;
	CComboBox	m_wndOriginList;
	CComboBox	m_wndTypeList;
	CButton	m_wndSelectButton;
	int		m_iPollingInterval;
	int		m_iRetentionTime;
	CString	m_strName;
	int		m_iDataType;
	int		m_iOrigin;
	int		m_iStatus;
	CString	m_strDescription;
	BOOL	m_bAdvSchedule;
	int		m_snmpPort;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDCIPropPage)
	public:
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	NXC_OBJECT * m_pCluster;
	void EnablePollingInterval(BOOL bEnable);
	NXC_AGENT_PARAM *m_pParamList;
	DWORD m_dwNumParams;
	void SelectAgentItem(void);
	void SelectInternalItem(void);
	void SelectSNMPItem(void);

	// Generated message map functions
	//{{AFX_MSG(CDCIPropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSelect();
	afx_msg void OnSelchangeComboOrigin();
	afx_msg void OnCheckSchedule();
	afx_msg void OnSelchangeComboDt();
	afx_msg void OnSelchangeComboResources();
	afx_msg void OnButtonSelectProxy();
	afx_msg void OnCheckCustomPort();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCIPROPPAGE_H__45FF1964_0A50_43D7_9F4A_BDFB03DD7A4C__INCLUDED_)
