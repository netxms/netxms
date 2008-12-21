#if !defined(AFX_CONFIGWIZARD_H__43960977_7BC5_4D05_954D_72B7821105C3__INCLUDED_)
#define AFX_CONFIGWIZARD_H__43960977_7BC5_4D05_954D_72B7821105C3__INCLUDED_

#include "nxconfig.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigWizard.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfigWizard

class CConfigWizard : public CPropertySheet
{
	DECLARE_DYNAMIC(CConfigWizard)

// Construction
public:
	CConfigWizard(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CConfigWizard(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	WIZARD_CFG_INFO m_cfg;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigWizard)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConfigWizard();

	// Generated message map functions
protected:
	void DefaultConfig(void);
	//{{AFX_MSG(CConfigWizard)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGWIZARD_H__43960977_7BC5_4D05_954D_72B7821105C3__INCLUDED_)
