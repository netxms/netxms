#if !defined(AFX_FINISHPAGE_H__35A849F5_1060_4373_AB51_C2D4BC2D2299__INCLUDED_)
#define AFX_FINISHPAGE_H__35A849F5_1060_4373_AB51_C2D4BC2D2299__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FinishPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFinishPage dialog

class CFinishPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CFinishPage)

// Construction
public:
	CFinishPage();
	~CFinishPage();

// Dialog Data
	//{{AFX_DATA(CFinishPage)
	enum { IDD = IDD_FINISH };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinishPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinishPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINISHPAGE_H__35A849F5_1060_4373_AB51_C2D4BC2D2299__INCLUDED_)
