#if !defined(AFX_OBJECTSEARCHBOX_H__D1D524BC_CD12_44D3_97CF_A780ADAC0657__INCLUDED_)
#define AFX_OBJECTSEARCHBOX_H__D1D524BC_CD12_44D3_97CF_A780ADAC0657__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectSearchBox.h : header file
//

#include "ToolBox.h"


/////////////////////////////////////////////////////////////////////////////
// CObjectSearchBox window

class CObjectSearchBox : public CToolBox
{
// Construction
public:
	CObjectSearchBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectSearchBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectSearchBox();

	// Generated message map functions
protected:
	CFont m_fontNormal;
	CButton m_wndButton;
	CEdit m_wndEditBox;
	CStatic m_wndStatic;
	//{{AFX_MSG(CObjectSearchBox)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTSEARCHBOX_H__D1D524BC_CD12_44D3_97CF_A780ADAC0657__INCLUDED_)
