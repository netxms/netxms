#if !defined(AFX_CLUSTERPROPSGENERAL_H__8B55C895_8DD1_4AE2_878E_EF4EB0CA2AF8__INCLUDED_)
#define AFX_CLUSTERPROPSGENERAL_H__8B55C895_8DD1_4AE2_878E_EF4EB0CA2AF8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClusterPropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClusterPropsGeneral dialog

class CClusterPropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CClusterPropsGeneral)

// Construction
public:
	NXC_OBJECT * m_pObject;
	CClusterPropsGeneral();
	~CClusterPropsGeneral();

// Dialog Data
	//{{AFX_DATA(CClusterPropsGeneral)
	enum { IDD = IDD_OBJECT_CLUSTER_GENERAL };
	CListCtrl	m_wndListCtrl;
	DWORD	m_dwObjectId;
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CClusterPropsGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	IP_NETWORK * m_pNetList;
	// Generated message map functions
	//{{AFX_MSG(CClusterPropsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLUSTERPROPSGENERAL_H__8B55C895_8DD1_4AE2_878E_EF4EB0CA2AF8__INCLUDED_)
