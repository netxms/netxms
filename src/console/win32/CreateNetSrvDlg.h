#if !defined(AFX_CREATENETSRVDLG_H__DC95E746_5880_45FB_A142_DE5C6FEAC721__INCLUDED_)
#define AFX_CREATENETSRVDLG_H__DC95E746_5880_45FB_A142_DE5C6FEAC721__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateNetSrvDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateNetSrvDlg dialog

class CCreateNetSrvDlg : public CCreateObjectDlg
{
// Construction
public:
	int m_iServiceType;
	CCreateNetSrvDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateNetSrvDlg)
	enum { IDD = IDD_CREATE_NETSRV };
	CComboBox	m_wndTypeList;
	long	m_iPort;
	int		m_iProtocolType;
	long	m_iProtocolNumber;
	CString	m_strRequest;
	CString	m_strResponce;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateNetSrvDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void GetServiceType(void);
	void SetProtocolCtrls(void);

	// Generated message map functions
	//{{AFX_MSG(CCreateNetSrvDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeComboTypes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATENETSRVDLG_H__DC95E746_5880_45FB_A142_DE5C6FEAC721__INCLUDED_)
