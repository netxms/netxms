#if !defined(AFX_LPPLIST_H__87773022_E8F4_4E14_9260_BA5102560BD7__INCLUDED_)
#define AFX_LPPLIST_H__87773022_E8F4_4E14_9260_BA5102560BD7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LPPList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLPPList frame

class CLPPList : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CLPPList)
protected:
	CLPPList();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLPPList)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLPPList();

	// Generated message map functions
	//{{AFX_MSG(CLPPList)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LPPLIST_H__87773022_E8F4_4E14_9260_BA5102560BD7__INCLUDED_)
