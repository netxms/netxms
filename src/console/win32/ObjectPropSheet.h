#if !defined(AFX_OBJECTPROPSHEET_H__10D443C2_A463_468B_9974_FF168FB42918__INCLUDED_)
#define AFX_OBJECTPROPSHEET_H__10D443C2_A463_468B_9974_FF168FB42918__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropSheet

class CObjectPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CObjectPropSheet)

// Construction
public:
	CObjectPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CObjectPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectPropSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CObjectPropSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSHEET_H__10D443C2_A463_468B_9974_FF168FB42918__INCLUDED_)
