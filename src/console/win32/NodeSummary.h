#if !defined(AFX_NODESUMMARY_H__D6E22F80_07FD_4BFD_BC0C_B8972C9EEDCE__INCLUDED_)
#define AFX_NODESUMMARY_H__D6E22F80_07FD_4BFD_BC0C_B8972C9EEDCE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodeSummary.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodeSummary window

class CNodeSummary : public CWnd
{
// Construction
public:
	CNodeSummary();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodeSummary)
	public:
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext);
	virtual ~CNodeSummary();

	// Generated message map functions
protected:
	void UpdateStatus(void);
	//{{AFX_MSG(CNodeSummary)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	DWORD m_dwTotalNodes;
	DWORD m_dwNodeStats[OBJECT_STATUS_COUNT];
	CFont m_fontNormal;
	CFont m_fontTitle;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODESUMMARY_H__D6E22F80_07FD_4BFD_BC0C_B8972C9EEDCE__INCLUDED_)
