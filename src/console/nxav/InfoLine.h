#if !defined(AFX_INFOLINE_H__6A4E4CEC_1D4E_4D3C_B97B_4AFA962A622C__INCLUDED_)
#define AFX_INFOLINE_H__6A4E4CEC_1D4E_4D3C_B97B_4AFA962A622C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InfoLine.h : header file
//

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
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CInfoLine();

	// Generated message map functions
protected:
	CFont m_fontSmall;
	//{{AFX_MSG(CInfoLine)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INFOLINE_H__6A4E4CEC_1D4E_4D3C_B97B_4AFA962A622C__INCLUDED_)
