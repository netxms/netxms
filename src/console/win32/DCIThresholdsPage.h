#if !defined(AFX_DCITHRESHOLDSPAGE_H__15B4CFCB_DE84_406D_A98F_1ABE35E9965D__INCLUDED_)
#define AFX_DCITHRESHOLDSPAGE_H__15B4CFCB_DE84_406D_A98F_1ABE35E9965D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCIThresholdsPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCIThresholdsPage dialog

class CDCIThresholdsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDCIThresholdsPage)

// Construction
public:
   NXC_DCI *m_pItem;
	CDCIThresholdsPage();
	~CDCIThresholdsPage();

// Dialog Data
	//{{AFX_DATA(CDCIThresholdsPage)
	enum { IDD = IDD_DCI_THRESHOLDS };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDCIThresholdsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDCIThresholdsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCITHRESHOLDSPAGE_H__15B4CFCB_DE84_406D_A98F_1ABE35E9965D__INCLUDED_)
