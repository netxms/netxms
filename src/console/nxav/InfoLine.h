#if !defined(AFX_INFOLINE_H__6A4E4CEC_1D4E_4D3C_B97B_4AFA962A622C__INCLUDED_)
#define AFX_INFOLINE_H__6A4E4CEC_1D4E_4D3C_B97B_4AFA962A622C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InfoLine.h : header file
//
#include "FlatButton.h"

/////////////////////////////////////////////////////////////////////////////
// CInfoLine window

class CInfoLine : public CWnd
{
// Construction
public:
	CInfoLine();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInfoLine)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CInfoLine();

	// Generated message map functions
protected:
	UINT m_nTimer;
	CStatic m_wndTimer;
	CFlatButton m_wndButtonSettings;
	CFlatButton m_wndButtonClose;
	CFont m_fontSmall;
	//{{AFX_MSG(CInfoLine)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INFOLINE_H__6A4E4CEC_1D4E_4D3C_B97B_4AFA962A622C__INCLUDED_)
