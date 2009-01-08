#if !defined(AFX_NETSRVPROPSGENERAL_H__4D3EE6BE_7BC7_49C8_8E6F_9133C27C8BD2__INCLUDED_)
#define AFX_NETSRVPROPSGENERAL_H__4D3EE6BE_7BC7_49C8_8E6F_9133C27C8BD2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NetSrvPropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNetSrvPropsGeneral dialog

class CNetSrvPropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CNetSrvPropsGeneral)

// Construction
public:
	NXC_OBJECT *m_pObject;
	DWORD m_dwIpAddr;
	DWORD m_dwPollerNode;
	int m_iServiceType;
	CNetSrvPropsGeneral();
	~CNetSrvPropsGeneral();

// Dialog Data
	//{{AFX_DATA(CNetSrvPropsGeneral)
	enum { IDD = IDD_OBJECT_NETSRV_GENERAL };
	CComboBox	m_wndTypeList;
	DWORD	m_dwObjectId;
	CString	m_strName;
	long	m_iPort;
	CString	m_strRequest;
	CString	m_strResponse;
	long	m_iProto;
	int		m_iRequiredPolls;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNetSrvPropsGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SetProtocolCtrls(void);
	// Generated message map functions
	//{{AFX_MSG(CNetSrvPropsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	afx_msg void OnSelchangeComboType();
	afx_msg void OnChangeEditRequest();
	afx_msg void OnChangeEditResponse();
	afx_msg void OnRadioTcp();
	afx_msg void OnRadioUdp();
	afx_msg void OnRadioIcmp();
	afx_msg void OnRadioOther();
	afx_msg void OnSelectIp();
	afx_msg void OnSelectPoller();
	afx_msg void OnChangeEditPort();
	afx_msg void OnChangeEditProto();
	afx_msg void OnChangeEditPolls();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE *m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETSRVPROPSGENERAL_H__4D3EE6BE_7BC7_49C8_8E6F_9133C27C8BD2__INCLUDED_)
