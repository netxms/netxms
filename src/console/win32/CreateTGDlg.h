#if !defined(AFX_CREATETGDLG_H__5A71CFF5_7762_47E8_AE58_6E724657EC48__INCLUDED_)
#define AFX_CREATETGDLG_H__5A71CFF5_7762_47E8_AE58_6E724657EC48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateTGDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateTGDlg dialog

class CCreateTGDlg : public CCreateObjectDlg
{
// Construction
public:
	CCreateTGDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateTGDlg)
	enum { IDD = IDD_CREATE_TG };
	CString	m_strDescription;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateTGDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateTGDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATETGDLG_H__5A71CFF5_7762_47E8_AE58_6E724657EC48__INCLUDED_)
