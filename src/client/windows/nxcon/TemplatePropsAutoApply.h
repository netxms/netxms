#if !defined(AFX_TEMPLATEPROPSAUTOAPPLY_H__72BD1DA0_4722_4C20_90EA_CCC666F6ACB7__INCLUDED_)
#define AFX_TEMPLATEPROPSAUTOAPPLY_H__72BD1DA0_4722_4C20_90EA_CCC666F6ACB7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TemplatePropsAutoApply.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTemplatePropsAutoApply dialog

class CTemplatePropsAutoApply : public CPropertyPage
{
	DECLARE_DYNCREATE(CTemplatePropsAutoApply)

// Construction
public:
	CTemplatePropsAutoApply();
	~CTemplatePropsAutoApply();

// Dialog Data
	//{{AFX_DATA(CTemplatePropsAutoApply)
	enum { IDD = IDD_OBJECT_TEMPLATE_AUTOAPPLY };
	BOOL	m_bEnableAutoApply;
	//}}AFX_DATA

	CString m_strFilterScript;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTemplatePropsAutoApply)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTemplatePropsAutoApply)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckEnable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CScintillaCtrl m_wndEditor;

private:
	NXC_OBJECT_UPDATE *m_pUpdate;
	BOOL m_bInitialEnableFlag;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEMPLATEPROPSAUTOAPPLY_H__72BD1DA0_4722_4C20_90EA_CCC666F6ACB7__INCLUDED_)
