#if !defined(AFX_OBJECTPROPSSECURITY_H__9ED593B4_1EB8_4B50_A1A0_3F086BAE4031__INCLUDED_)
#define AFX_OBJECTPROPSSECURITY_H__9ED593B4_1EB8_4B50_A1A0_3F086BAE4031__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsSecurity.h : header file
//

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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_ACL_ENTRY * m_pAccessList;
	DWORD m_dwAclSize;
	BOOL m_bIsModified;
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSSECURITY_H__9ED593B4_1EB8_4B50_A1A0_3F086BAE4031__INCLUDED_)
