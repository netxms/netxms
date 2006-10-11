#if !defined(AFX_OBJTOOLPROPOPTIONS_H__2F1F4B87_3FCD_45AE_8C43_AC5FBB04AB3E__INCLUDED_)
#define AFX_OBJTOOLPROPOPTIONS_H__2F1F4B87_3FCD_45AE_8C43_AC5FBB04AB3E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjToolPropOptions.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropOptions dialog

class CObjToolPropOptions : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjToolPropOptions)

// Construction
public:
	int m_iToolType;
	CObjToolPropOptions();
	~CObjToolPropOptions();

// Dialog Data
	//{{AFX_DATA(CObjToolPropOptions)
	enum { IDD = IDD_OBJTOOL_OPTIONS };
	CString	m_strMatchingOID;
	BOOL	m_bNeedAgent;
	BOOL	m_bMatchOID;
	BOOL	m_bNeedSNMP;
	int		m_nIndexType;
	BOOL	m_bConfirmation;
	CString	m_strConfirmationText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjToolPropOptions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjToolPropOptions)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckMatchOid();
	afx_msg void OnCheckConfirm();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJTOOLPROPOPTIONS_H__2F1F4B87_3FCD_45AE_8C43_AC5FBB04AB3E__INCLUDED_)
