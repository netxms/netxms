#if !defined(AFX_CONDPROPSSCRIPT_H__4864D04D_C38D_4318_9BFD_61DE1CA19124__INCLUDED_)
#define AFX_CONDPROPSSCRIPT_H__4864D04D_C38D_4318_9BFD_61DE1CA19124__INCLUDED_

#include "..\NXUILIB\ScintillaCtrl.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CondPropsScript.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCondPropsScript dialog

class CCondPropsScript : public CPropertyPage
{
	DECLARE_DYNCREATE(CCondPropsScript)

// Construction
public:
	CString m_strScript;
	CCondPropsScript();
	virtual ~CCondPropsScript();

// Dialog Data
	//{{AFX_DATA(CCondPropsScript)
	enum { IDD = IDD_OBJECT_COND_SCRIPT };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCondPropsScript)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CScintillaCtrl m_wndEditor;
	// Generated message map functions
	//{{AFX_MSG(CCondPropsScript)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONDPROPSSCRIPT_H__4864D04D_C38D_4318_9BFD_61DE1CA19124__INCLUDED_)
