#if !defined(AFX_WAITVIEW_H__75084C02_DB22_4977_BD8F_E5092C21A222__INCLUDED_)
#define AFX_WAITVIEW_H__75084C02_DB22_4977_BD8F_E5092C21A222__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaitView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaitView window

class CWaitView : public CWnd
{
// Construction
public:
	CWaitView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWaitView();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWaitView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITVIEW_H__75084C02_DB22_4977_BD8F_E5092C21A222__INCLUDED_)
