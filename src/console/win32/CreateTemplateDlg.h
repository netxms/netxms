#if !defined(AFX_CREATETEMPLATEDLG_H__79782431_AEF6_4657_914D_3BA095729AE5__INCLUDED_)
#define AFX_CREATETEMPLATEDLG_H__79782431_AEF6_4657_914D_3BA095729AE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateTemplateDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateTemplateDlg dialog

class CCreateTemplateDlg : public CCreateObjectDlg
{
// Construction
public:
	CCreateTemplateDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateTemplateDlg)
	enum { IDD = IDD_CREATE_TEMPLATE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateTemplateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateTemplateDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATETEMPLATEDLG_H__79782431_AEF6_4657_914D_3BA095729AE5__INCLUDED_)
