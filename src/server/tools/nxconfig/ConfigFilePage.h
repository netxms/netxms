#if !defined(AFX_CONFIGFILEPAGE_H__CDBE6809_8D0A_4D90_AFF8_BE9EBECCDF35__INCLUDED_)
#define AFX_CONFIGFILEPAGE_H__CDBE6809_8D0A_4D90_AFF8_BE9EBECCDF35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigFilePage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfigFilePage dialog

class CConfigFilePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CConfigFilePage)

// Construction
public:
	CConfigFilePage();
	~CConfigFilePage();

// Dialog Data
	//{{AFX_DATA(CConfigFilePage)
	enum { IDD = IDD_CFG_FILE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CConfigFilePage)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CConfigFilePage)
	afx_msg void OnButtonBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGFILEPAGE_H__CDBE6809_8D0A_4D90_AFF8_BE9EBECCDF35__INCLUDED_)
