#if !defined(AFX_CLUSTERRESDLG_H__A4022A23_74DB_42A7_AB73_09A03179C3E7__INCLUDED_)
#define AFX_CLUSTERRESDLG_H__A4022A23_74DB_42A7_AB73_09A03179C3E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClusterResDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClusterResDlg dialog

class CClusterResDlg : public CDialog
{
// Construction
public:
	DWORD m_dwIpAddr;
	CClusterResDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CClusterResDlg)
	enum { IDD = IDD_CLUSTER_RESOURCE };
	CIPAddressCtrl	m_wndIpAddr;
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClusterResDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CClusterResDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLUSTERRESDLG_H__A4022A23_74DB_42A7_AB73_09A03179C3E7__INCLUDED_)
