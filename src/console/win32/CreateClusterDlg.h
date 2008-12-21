#if !defined(AFX_CREATECLUSTERDLG_H__BF45A4D7_0620_4661_8604_48C1FF41D019__INCLUDED_)
#define AFX_CREATECLUSTERDLG_H__BF45A4D7_0620_4661_8604_48C1FF41D019__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateClusterDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateClusterDlg dialog

class CCreateClusterDlg : public CCreateObjectDlg
{
// Construction
public:
	CCreateClusterDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateClusterDlg)
	enum { IDD = IDD_CREATE_CLUSTER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateClusterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateClusterDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATECLUSTERDLG_H__BF45A4D7_0620_4661_8604_48C1FF41D019__INCLUDED_)
