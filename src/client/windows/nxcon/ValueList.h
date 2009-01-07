#if !defined(AFX_VALUELIST_H__C697E2D6_D72B_4A45_8779_C5FAC83F6209__INCLUDED_)
#define AFX_VALUELIST_H__C697E2D6_D72B_4A45_8779_C5FAC83F6209__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ValueList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CValueList window

class CValueList : public CListCtrl
{
// Construction
public:
	CValueList();

// Attributes
public:

// Operations
public:
   virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CValueList)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CValueList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CValueList)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	CFont m_fontBold;
	CFont m_fontNormal;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VALUELIST_H__C697E2D6_D72B_4A45_8779_C5FAC83F6209__INCLUDED_)
