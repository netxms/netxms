#if !defined(AFX_NEWOBJECTTOOLDLG_H__8DA7443E_36B5_4364_92E9_9AAC3476BCF2__INCLUDED_)
#define AFX_NEWOBJECTTOOLDLG_H__8DA7443E_36B5_4364_92E9_9AAC3476BCF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewObjectToolDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewObjectToolDlg dialog

class CNewObjectToolDlg : public CDialog
{
// Construction
public:
	CNewObjectToolDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewObjectToolDlg)
	enum { IDD = IDD_NEW_OBJECT_TOOL };
	CString	m_strName;
	int		m_iToolType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewObjectToolDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewObjectToolDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWOBJECTTOOLDLG_H__8DA7443E_36B5_4364_92E9_9AAC3476BCF2__INCLUDED_)
