#if !defined(AFX_DCIPROPDLG_H__45FF1964_0A50_43D7_9F4A_BDFB03DD7A4C__INCLUDED_)
#define AFX_DCIPROPDLG_H__45FF1964_0A50_43D7_9F4A_BDFB03DD7A4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCIPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCIPropDlg dialog

class CDCIPropDlg : public CDialog
{
// Construction
public:
	CDCIPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDCIPropDlg)
	enum { IDD = IDD_DCI_PROPERTIES };
	CComboBox	m_wndOriginList;
	CComboBox	m_wndTypeList;
	CButton	m_wndSelectButton;
	int		m_iPollingInterval;
	int		m_iRetentionTime;
	CString	m_strName;
	int		m_iDataType;
	int		m_iOrigin;
	int		m_iStatus;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDCIPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDCIPropDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCIPROPDLG_H__45FF1964_0A50_43D7_9F4A_BDFB03DD7A4C__INCLUDED_)
