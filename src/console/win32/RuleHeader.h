#if !defined(AFX_RULEHEADER_H__4B560BCA_AFF4_4030_BE9F_B0A965B94791__INCLUDED_)
#define AFX_RULEHEADER_H__4B560BCA_AFF4_4030_BE9F_B0A965B94791__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleHeader.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleHeader window

class CRuleHeader : public CHeaderCtrl
{
// Construction
public:
	CRuleHeader();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleHeader)
	//}}AFX_VIRTUAL

   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

// Implementation
public:
	virtual ~CRuleHeader();

	// Generated message map functions
protected:
	//{{AFX_MSG(CRuleHeader)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULEHEADER_H__4B560BCA_AFF4_4030_BE9F_B0A965B94791__INCLUDED_)
