#if !defined(AFX_EDITEVENTDLG_H__C48F412D_D85B_497B_A97E_5DBD90789A54__INCLUDED_)
#define AFX_EDITEVENTDLG_H__C48F412D_D85B_497B_A97E_5DBD90789A54__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditEventDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditEventDlg dialog

class CEditEventDlg : public CDialog
{
// Construction
public:
	DWORD m_dwSeverity;
	CEditEventDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditEventDlg)
	enum { IDD = IDD_EDIT_EVENT };
	CComboBox	m_wndComboBox;
	BOOL	m_bWriteLog;
	CString	m_strName;
	CString	m_strMessage;
	DWORD	m_dwEventId;
	CString	m_strDescription;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditEventDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditEventDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITEVENTDLG_H__C48F412D_D85B_497B_A97E_5DBD90789A54__INCLUDED_)
