#if !defined(AFX_OBJECTPROPSGENERAL_H__B1A6809B_1070_43F9_A561_BBE7EF483DCA__INCLUDED_)
#define AFX_OBJECTPROPSGENERAL_H__B1A6809B_1070_43F9_A561_BBE7EF483DCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsGeneral dialog

class CObjectPropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsGeneral)

// Construction
public:
	CObjectPropsGeneral();
	~CObjectPropsGeneral();

// Dialog Data
	//{{AFX_DATA(CObjectPropsGeneral)
	enum { IDD = IDD_OBJECT_GENERAL };
	DWORD	m_dwObjectId;
	CString	m_strName;
	CString	m_strClass;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSGENERAL_H__B1A6809B_1070_43F9_A561_BBE7EF483DCA__INCLUDED_)
