#if !defined(AFX_VPNCPROPSGENERAL_H__3150567E_5792_4B14_82A9_23031DCE196A__INCLUDED_)
#define AFX_VPNCPROPSGENERAL_H__3150567E_5792_4B14_82A9_23031DCE196A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VPNCPropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVPNCPropsGeneral dialog

class CVPNCPropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CVPNCPropsGeneral)

// Construction
public:
	DWORD m_dwPeerGateway;
	NXC_OBJECT_UPDATE *m_pUpdate;
	NXC_OBJECT *m_pObject;
	CVPNCPropsGeneral();
	~CVPNCPropsGeneral();

// Dialog Data
	//{{AFX_DATA(CVPNCPropsGeneral)
	enum { IDD = IDD_OBJECT_VPNC_GENERAL };
	CListCtrl	m_wndListRemote;
	CListCtrl	m_wndListLocal;
	CString	m_strName;
	DWORD	m_dwObjectId;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CVPNCPropsGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void RemoveNetworks(DWORD *pdwNumNets, IP_NETWORK **ppNetList, CListCtrl *pListCtrl);
	void AddSubnetToList(CListCtrl *pListCtrl, IP_NETWORK *pSubnet);
	IP_NETWORK *m_pRemoteNetList;
	IP_NETWORK *m_pLocalNetList;
	DWORD m_dwNumRemoteNets;
	DWORD m_dwNumLocalNets;
	void AddNetwork(DWORD *pdwNumNets, IP_NETWORK **ppNetList, CListCtrl *pListCtrl);
	// Generated message map functions
	//{{AFX_MSG(CVPNCPropsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	afx_msg void OnSelectGateway();
	afx_msg void OnAddLocalNet();
	afx_msg void OnAddRemoteNet();
	afx_msg void OnRemoveLocalNet();
	afx_msg void OnRemoveRemoteNet();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VPNCPROPSGENERAL_H__3150567E_5792_4B14_82A9_23031DCE196A__INCLUDED_)
