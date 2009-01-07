#if !defined(AFX_CONDPROPSGENERAL_H__96AA21CD_9FBA_435A_8098_F8FD6CF27EDE__INCLUDED_)
#define AFX_CONDPROPSGENERAL_H__96AA21CD_9FBA_435A_8098_F8FD6CF27EDE__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CondPropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCondPropsGeneral dialog

class CCondPropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CCondPropsGeneral)

// Construction
public:
	NXC_OBJECT *m_pObject;
	CCondPropsGeneral();
	~CCondPropsGeneral();

// Dialog Data
	//{{AFX_DATA(CCondPropsGeneral)
	enum { IDD = IDD_OBJECT_COND_GENERAL };
	CComboBox	m_wndComboInactive;
	CComboBox	m_wndComboActive;
	DWORD	m_dwObjectId;
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCondPropsGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCondPropsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	afx_msg void OnSelchangeComboActive();
	afx_msg void OnSelchangeComboInactive();
	afx_msg void OnBrowseObject();
	afx_msg void OnBrowseActivationEvent();
	afx_msg void OnBrowseDeactivationEvent();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE *m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONDPROPSGENERAL_H__96AA21CD_9FBA_435A_8098_F8FD6CF27EDE__INCLUDED_)
