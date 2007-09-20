#if !defined(AFX_IFPROPSGENERAL_H__96D35421_564A_4807_973A_91834C3DCBBA__INCLUDED_)
#define AFX_IFPROPSGENERAL_H__96D35421_564A_4807_973A_91834C3DCBBA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IfPropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIfPropsGeneral dialog

class CIfPropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CIfPropsGeneral)

// Construction
public:
	NXC_OBJECT * m_pObject;
	CIfPropsGeneral();
	~CIfPropsGeneral();

// Dialog Data
	//{{AFX_DATA(CIfPropsGeneral)
	enum { IDD = IDD_OBJECT_IF_GENERAL };
	CString	m_strName;
	int		m_nRequiredPolls;
	DWORD	m_dwObjectId;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIfPropsGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CIfPropsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditName();
	afx_msg void OnChangeEditPolls();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE *m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IFPROPSGENERAL_H__96D35421_564A_4807_973A_91834C3DCBBA__INCLUDED_)
