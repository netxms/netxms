#if !defined(AFX_CREATECONDDLG_H__4AB1FDF5_2681_4339_92D3_02CBD447AB41__INCLUDED_)
#define AFX_CREATECONDDLG_H__4AB1FDF5_2681_4339_92D3_02CBD447AB41__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateCondDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateCondDlg dialog

class CCreateCondDlg : public CCreateObjectDlg
{
// Construction
public:
	CCreateCondDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateCondDlg)
	enum { IDD = IDD_CREATE_CONDITION };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateCondDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateCondDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATECONDDLG_H__4AB1FDF5_2681_4339_92D3_02CBD447AB41__INCLUDED_)
