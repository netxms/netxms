#if !defined(AFX_TOOLBOX_H__1CB3ED8D_E617_4236_9715_9707A1146FE6__INCLUDED_)
#define AFX_TOOLBOX_H__1CB3ED8D_E617_4236_9715_9707A1146FE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ToolBox.h : header file
//

#define MAX_TOOLBOX_TITLE  64

/////////////////////////////////////////////////////////////////////////////
// CToolBox window

class CToolBox : public CWnd
{
// Construction
public:
	CToolBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToolBox)
	public:
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CToolBox();

	virtual BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

   // Generated message map functions
protected:
	CFont m_fontTitle;
	char m_szTitle[MAX_TOOLBOX_TITLE];
	//{{AFX_MSG(CToolBox)
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLBOX_H__1CB3ED8D_E617_4236_9715_9707A1146FE6__INCLUDED_)
