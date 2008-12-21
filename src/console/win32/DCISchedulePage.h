#if !defined(AFX_DCISCHEDULEPAGE_H__100EDD63_08B7_42C9_A1EF_0B70C1DC4303__INCLUDED_)
#define AFX_DCISCHEDULEPAGE_H__100EDD63_08B7_42C9_A1EF_0B70C1DC4303__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCISchedulePage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCISchedulePage dialog

class CDCISchedulePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDCISchedulePage)

// Construction
public:
	TCHAR **m_ppScheduleList;
	DWORD m_dwNumSchedules;
	CDCISchedulePage();
	~CDCISchedulePage();

// Dialog Data
	//{{AFX_DATA(CDCISchedulePage)
	enum { IDD = IDD_DCI_SCHEDULE };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDCISchedulePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDCISchedulePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonModify();
	afx_msg void OnButtonDelete();
	afx_msg void OnItemchangedListSchedules(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCISCHEDULEPAGE_H__100EDD63_08B7_42C9_A1EF_0B70C1DC4303__INCLUDED_)
