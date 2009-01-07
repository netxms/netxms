#if !defined(AFX_LASTVALUESPROPDLG_H__526A3103_305E_43AC_9992_C117E1DFAB27__INCLUDED_)
#define AFX_LASTVALUESPROPDLG_H__526A3103_305E_43AC_9992_C117E1DFAB27__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LastValuesPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLastValuesPropDlg dialog

class CLastValuesPropDlg : public CDialog
{
// Construction
public:
	CLastValuesPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLastValuesPropDlg)
	enum { IDD = IDD_LASTVAL_PROP };
	BOOL	m_bShowGrid;
	BOOL	m_bRefresh;
	DWORD	m_dwSeconds;
	BOOL	m_bHideEmpty;
	BOOL	m_bUseMultipliers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLastValuesPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLastValuesPropDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LASTVALUESPROPDLG_H__526A3103_305E_43AC_9992_C117E1DFAB27__INCLUDED_)
