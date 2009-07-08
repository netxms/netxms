#if !defined(AFX_NODEPROPSCONN_H__C7E2E204_6281_44E8_BF1A_4ACC2E5B3E07__INCLUDED_)
#define AFX_NODEPROPSCONN_H__C7E2E204_6281_44E8_BF1A_4ACC2E5B3E07__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodePropsConn.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodePropsConn dialog

class CNodePropsConn : public CPropertyPage
{
	DECLARE_DYNCREATE(CNodePropsConn)

// Construction
public:
	int m_iSnmpPriv;
	int m_iSnmpAuth;
	int m_iSNMPVersion;
	int m_iAuthType;
	DWORD m_dwSNMPProxy;
	DWORD m_dwProxyNode;
	CNodePropsConn();
	~CNodePropsConn();

// Dialog Data
	//{{AFX_DATA(CNodePropsConn)
	enum { IDD = IDD_OBJECT_NODE_CONN };
	CComboBox	m_wndSnmpPriv;
	CComboBox	m_wndSnmpAuth;
	CComboBox	m_wndSnmpVersion;
	CComboBox	m_wndAuthList;
	CString	m_strSnmpAuthPassword;
	CString	m_strSnmpAuthName;
	CString	m_strSnmpPrivPassword;
	int		m_iAgentPort;
	CString	m_strSecret;
	BOOL	m_bForceEncryption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNodePropsConn)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnSnmpVersionChange();
	// Generated message map functions
	//{{AFX_MSG(CNodePropsConn)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectProxy();
	afx_msg void OnSelectSnmpproxy();
	afx_msg void OnButtonGenerate();
	afx_msg void OnSelchangeComboAuth();
	afx_msg void OnChangeEditPort();
	afx_msg void OnChangeEditSecret();
	afx_msg void OnChangeEditPrivPassword();
	afx_msg void OnChangeEditCommunity();
	afx_msg void OnChangeEditAuthPassword();
	afx_msg void OnSelchangeComboSnmpVersion();
	afx_msg void OnSelchangeComboUsmAuth();
	afx_msg void OnSelchangeComboUsmPriv();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODEPROPSCONN_H__C7E2E204_6281_44E8_BF1A_4ACC2E5B3E07__INCLUDED_)
