#if !defined(AFX_OBJECTPROPDLG_H__4C4ADEE9_38D7_484D_925C_55B00F1EDDE6__INCLUDED_)
#define AFX_OBJECTPROPDLG_H__4C4ADEE9_38D7_484D_925C_55B00F1EDDE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropDlg dialog

class CObjectPropDlg : public CDialog
{
// Construction
public:
	CObjectPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CObjectPropDlg)
	enum { IDD = IDD_OBJECT_PROPERTIES };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjectPropDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPDLG_H__4C4ADEE9_38D7_484D_925C_55B00F1EDDE6__INCLUDED_)
