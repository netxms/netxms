#if !defined(AFX_SUMMARYVIEW_H__54818CA0_DFF4_4351_8B01_224C56A92FD9__INCLUDED_)
#define AFX_SUMMARYVIEW_H__54818CA0_DFF4_4351_8B01_224C56A92FD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SummaryView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSummaryView window

class CSummaryView : public CWnd
{
// Construction
public:
	CSummaryView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSummaryView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSummaryView();

	// Generated message map functions
protected:
	DWORD m_dwNodeStats[8];
	DWORD m_dwTotalNodes;
	CFont m_fontNormal;
	CFont m_fontTitle;
	void PaintNodeSummary(CDC &dc);
	//{{AFX_MSG(CSummaryView)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUMMARYVIEW_H__54818CA0_DFF4_4351_8B01_224C56A92FD9__INCLUDED_)
