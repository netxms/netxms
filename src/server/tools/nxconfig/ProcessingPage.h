#if !defined(AFX_PROCESSINGPAGE_H__9EE04A92_A6EA_47B9_B2DD_5B95B6A0C47B__INCLUDED_)
#define AFX_PROCESSINGPAGE_H__9EE04A92_A6EA_47B9_B2DD_5B95B6A0C47B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProcessingPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProcessingPage dialog

class CProcessingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CProcessingPage)

// Construction
public:
	CProcessingPage();
	~CProcessingPage();

// Dialog Data
	//{{AFX_DATA(CProcessingPage)
	enum { IDD = IDD_PROCESSING };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CProcessingPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iLastItem;
	CImageList m_imageList;
	// Generated message map functions
	//{{AFX_MSG(CProcessingPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
   afx_msg LRESULT OnStartStage(WPARAM wParam, LPARAM lParam);
   afx_msg LRESULT OnStageCompleted(WPARAM wParam, LPARAM lParam);
   afx_msg LRESULT OnJobFinished(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROCESSINGPAGE_H__9EE04A92_A6EA_47B9_B2DD_5B95B6A0C47B__INCLUDED_)
