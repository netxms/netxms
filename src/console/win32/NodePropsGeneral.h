#if !defined(AFX_NODEPROPSGENERAL_H__AB0C9F79_7DD3_4B52_988E_32E4D06D59A9__INCLUDED_)
#define AFX_NODEPROPSGENERAL_H__AB0C9F79_7DD3_4B52_988E_32E4D06D59A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodePropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodePropsGeneral dialog

class CNodePropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CNodePropsGeneral)

// Construction
public:
	CNodePropsGeneral();
	~CNodePropsGeneral();

// Dialog Data
	//{{AFX_DATA(CNodePropsGeneral)
	enum { IDD = IDD_OBJECT_NODE_GENERAL };
	CComboBox	m_wndAuthList;
	DWORD	m_dwObjectId;
	CString	m_strName;
	CString	m_strOID;
	int		m_iAgentPort;
	CString	m_strPrimaryIp;
	CString	m_strSecret;
	CString	m_strCommunity;
	int		m_iAuthType;
	int		m_iSNMPVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNodePropsGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNodePropsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	afx_msg void OnChangeEditSecret();
	afx_msg void OnChangeEditPort();
	afx_msg void OnChangeEditCommunity();
	afx_msg void OnSelchangeComboAuth();
	afx_msg void OnRadioVersion2c();
	afx_msg void OnRadioVersion1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODEPROPSGENERAL_H__AB0C9F79_7DD3_4B52_988E_32E4D06D59A9__INCLUDED_)
