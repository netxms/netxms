#if !defined(AFX_OBJECTINFOBOX_H__F4B7071C_AA6F_4CDF_A528_0E3AE5B66E02__INCLUDED_)
#define AFX_OBJECTINFOBOX_H__F4B7071C_AA6F_4CDF_A528_0E3AE5B66E02__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectInfoBox.h : header file
//

#include "ToolBox.h"

/////////////////////////////////////////////////////////////////////////////
// CObjectInfoBox window

class CObjectInfoBox : public CToolBox
{
// Construction
public:
	CObjectInfoBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectInfoBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetCurrentObject(NXC_OBJECT *pObject);
	virtual ~CObjectInfoBox();

	// Generated message map functions
protected:
	CFont m_fontNormal;
	CFont m_fontBold;
	//{{AFX_MSG(CObjectInfoBox)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	NXC_OBJECT *m_pCurrObject;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTINFOBOX_H__F4B7071C_AA6F_4CDF_A528_0E3AE5B66E02__INCLUDED_)
