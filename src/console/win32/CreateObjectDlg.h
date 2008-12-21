#if !defined(AFX_CREATEOBJECTDLG_H__78C8C102_6D9F_43A8_900F_4844E62CCF4F__INCLUDED_)
#define AFX_CREATEOBJECTDLG_H__78C8C102_6D9F_43A8_900F_4844E62CCF4F__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateObjectDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCreateObjectDlg dialog

class CCreateObjectDlg : public CDialog
{
// Construction
public:
	int m_iObjectClass;
	NXC_OBJECT *m_pParentObject;
	CCreateObjectDlg(int iId, CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CCreateObjectDlg)
	enum { IDD = IDD_DUMMY };
	CStatic	m_wndStaticName;
	CStatic	m_wndStaticId;
	CStatic	m_wndParentIcon;
	CString	m_strObjectName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateObjectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateParentInfo(void);

	// Generated message map functions
	//{{AFX_MSG(CCreateObjectDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectParent();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATEOBJECTDLG_H__78C8C102_6D9F_43A8_900F_4844E62CCF4F__INCLUDED_)
