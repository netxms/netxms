#if !defined(AFX_INPUTBOX_H__D9193821_D8B5_4E37_9BFE_283F5B7DF649__INCLUDED_)
#define AFX_INPUTBOX_H__D9193821_D8B5_4E37_9BFE_283F5B7DF649__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InputBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInputBox dialog

class CInputBox : public CDialog
{
// Construction
public:
	CString m_strTitle;
	CInputBox(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInputBox)
	enum { IDD = IDD_INPUT_BOX };
	CString	m_strText;
	CString	m_strHeader;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInputBox)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInputBox)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPUTBOX_H__D9193821_D8B5_4E37_9BFE_283F5B7DF649__INCLUDED_)
