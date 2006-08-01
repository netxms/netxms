#if !defined(AFX_SRVDEPSPAGE_H__1538FBD8_C1C1_495A_B35F_0371FA38F876__INCLUDED_)
#define AFX_SRVDEPSPAGE_H__1538FBD8_C1C1_495A_B35F_0371FA38F876__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SrvDepsPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSrvDepsPage dialog

class CSrvDepsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSrvDepsPage)

// Construction
public:
	CSrvDepsPage();
	~CSrvDepsPage();

// Dialog Data
	//{{AFX_DATA(CSrvDepsPage)
	enum { IDD = IDD_SRV_DEPS };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSrvDepsPage)
	public:
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	DWORD m_dwNumServices;
	TCHAR **m_ppszServiceNames;
	// Generated message map functions
	//{{AFX_MSG(CSrvDepsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRVDEPSPAGE_H__1538FBD8_C1C1_495A_B35F_0371FA38F876__INCLUDED_)
