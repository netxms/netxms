#if !defined(AFX_ADDRCHANGEDLG_H__480F0EB6_3490_4FCB_B9CD_AE8DF9BB9385__INCLUDED_)
#define AFX_ADDRCHANGEDLG_H__480F0EB6_3490_4FCB_B9CD_AE8DF9BB9385__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddrChangeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddrChangeDlg dialog

class CAddrChangeDlg : public CDialog
{
// Construction
public:
	DWORD m_dwIpAddr;
	CAddrChangeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddrChangeDlg)
	enum { IDD = IDD_CHANGE_IP };
	CIPAddressCtrl	m_wndIPAddr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddrChangeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddrChangeDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDRCHANGEDLG_H__480F0EB6_3490_4FCB_B9CD_AE8DF9BB9385__INCLUDED_)
