#if !defined(AFX_ADDRENTRYDLG_H__F1FB4BD3_43B2_4A92_B509_8CE1F2B661B0__INCLUDED_)
#define AFX_ADDRENTRYDLG_H__F1FB4BD3_43B2_4A92_B509_8CE1F2B661B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddrEntryDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddrEntryDlg dialog

class CAddrEntryDlg : public CDialog
{
// Construction
public:
	DWORD m_dwAddr2;
	DWORD m_dwAddr1;
	CAddrEntryDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddrEntryDlg)
	enum { IDD = IDD_ADDR_ENTRY };
	CIPAddressCtrl	m_wndAddr2;
	CIPAddressCtrl	m_wndAddr1;
	int		m_nType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddrEntryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddrEntryDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDRENTRYDLG_H__F1FB4BD3_43B2_4A92_B509_8CE1F2B661B0__INCLUDED_)
