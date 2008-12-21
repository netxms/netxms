#if !defined(AFX_CONSOLEPROPGENERAL_H__EA50D0C3_5A41_4576_B253_A528A8E195D3__INCLUDED_)
#define AFX_CONSOLEPROPGENERAL_H__EA50D0C3_5A41_4576_B253_A528A8E195D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConsolePropsGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConsolePropsGeneral dialog

class CConsolePropsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CConsolePropsGeneral)

// Construction
public:
	CConsolePropsGeneral();
	~CConsolePropsGeneral();

// Dialog Data
	//{{AFX_DATA(CConsolePropsGeneral)
	enum { IDD = IDD_CP_GENERAL };
	BOOL	m_bExpandCtrlPanel;
	BOOL	m_bShowGrid;
	CString	m_strTimeZone;
	int		m_nTimeZoneType;
	BOOL	m_bSaveGraphSettings;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CConsolePropsGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CConsolePropsGeneral)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONSOLEPROPGENERAL_H__EA50D0C3_5A41_4576_B253_A528A8E195D3__INCLUDED_)
