#if !defined(AFX_EXTEDITCTRL_H__75FE87B6_7006_457A_B982_D8A0FF4D721C__INCLUDED_)
#define AFX_EXTEDITCTRL_H__75FE87B6_7006_457A_B982_D8A0FF4D721C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExtEditCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExtEditCtrl window

class CExtEditCtrl : public CEdit
{
// Construction
public:
	CExtEditCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExtEditCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetEscapeCommand(int nCmd);
	void SetReturnCommand(int nCmd);
	virtual ~CExtEditCtrl();

	// Generated message map functions
protected:
	int m_nEscapeCommand;
	int m_nReturnCommand;
	//{{AFX_MSG(CExtEditCtrl)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXTEDITCTRL_H__75FE87B6_7006_457A_B982_D8A0FF4D721C__INCLUDED_)
