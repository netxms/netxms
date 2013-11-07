#if !defined(AFX_DCIPERFTABPAGE_H__F70AB154_5692_4E2C_8886_64AF29372326__INCLUDED_)
#define AFX_DCIPERFTABPAGE_H__F70AB154_5692_4E2C_8886_64AF29372326__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCIPerfTabPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCIPerfTabPage dialog

class CDCIPerfTabPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDCIPerfTabPage)

// Construction
public:
	CString m_dciDescription;
	CString m_strConfig;
	CDCIPerfTabPage();
	~CDCIPerfTabPage();

// Dialog Data
	//{{AFX_DATA(CDCIPerfTabPage)
	enum { IDD = IDD_DCI_PERFTAB };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDCIPerfTabPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CColourPickerXP m_wndGraphColor;
	COLORREF m_graphColor;
	CString m_graphTitle;
	bool m_showOnPerfTab;
	bool m_showThresholds;
	// Generated message map functions
	//{{AFX_MSG(CDCIPerfTabPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCIPERFTABPAGE_H__F70AB154_5692_4E2C_8886_64AF29372326__INCLUDED_)
