#if !defined(AFX_OBJECTPROPSSECURITY_H__9ED593B4_1EB8_4B50_A1A0_3F086BAE4031__INCLUDED_)
#define AFX_OBJECTPROPSSECURITY_H__9ED593B4_1EB8_4B50_A1A0_3F086BAE4031__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsSecurity.h : header file
//

#include "UserSelectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CObjectPropsSecurity dialog

class CObjectPropsSecurity : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsSecurity)

// Construction
public:
	NXC_OBJECT * m_pObject;
	CObjectPropsSecurity();
	~CObjectPropsSecurity();

// Dialog Data
	//{{AFX_DATA(CObjectPropsSecurity)
	enum { IDD = IDD_OBJECT_SECURITY };
	CButton	m_wndCheckTermAlarms;
	CButton	m_wndCheckPushData;
	CButton	m_wndCheckSend;
	CButton	m_wndCheckControl;
	CButton	m_wndCheckAckAlarms;
	CButton	m_wndCheckViewAlarms;
	CButton	m_wndCheckAccess;
	CButton	m_wndCheckCreate;
	CButton	m_wndCheckRead;
	CButton	m_wndCheckModify;
	CButton	m_wndCheckDelete;
	CListCtrl	m_wndUserList;
	BOOL	m_bInheritRights;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsSecurity)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsSecurity)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddUser();
	afx_msg void OnItemchangedListUsers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCheckInheritRights();
	afx_msg void OnDeleteUser();
	afx_msg void OnCheckRead();
	afx_msg void OnCheckModify();
	afx_msg void OnCheckDelete();
	afx_msg void OnCheckCreate();
	afx_msg void OnCheckAccess();
	afx_msg void OnCheckViewAlarms();
	afx_msg void OnCheckAckAlarms();
	afx_msg void OnCheckSend();
	afx_msg void OnCheckControl();
	afx_msg void OnCheckTermAlarms();
	afx_msg void OnCheckPushData();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	DWORD m_dwCurrAclEntry;
	NXC_ACL_ENTRY * m_pAccessList;
	DWORD m_dwAclSize;
	BOOL m_bIsModified;
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSSECURITY_H__9ED593B4_1EB8_4B50_A1A0_3F086BAE4031__INCLUDED_)
