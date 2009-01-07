/*******************************************************************************
	Author						: Aravindan Premkumar
	Unregistered Copyright 2003	: Aravindan Premkumar
	All Rights Reserved
	
	This piece of code does not have any registered copyright and is free to be 
	used as necessary. The user is free to modify as per the requirements. As a
	fellow developer, all that I expect and request for is to be given the 
	credit for intially developing this reusable code by not removing my name as 
	the author.
*******************************************************************************/

#if !defined(AFX_INPLACEEDIT_H__175AEDFF_731E_4721_8399_DE406A465861__INCLUDED_)
#define AFX_INPLACEEDIT_H__175AEDFF_731E_4721_8399_DE406A465861__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class NXUILIB_EXPORTABLE CInPlaceEdit : public CEdit
{

public:
	
// Implementation
	
	// Returns the instance of the class
	static CInPlaceEdit* GetInstance(); 

	// Deletes the instance of the class
	static void DeleteInstance(); 

	// Creates the Windows edit control and attaches it to the object
	// Shows the edit ctrl
	BOOL ShowEditCtrl(DWORD dwStyle, const RECT& rCellRect, CWnd* pParentWnd, 
					  UINT uiResourceID, int iRowIndex, int iColumnIndex,
					  CString& strValidChars, CString& rstrCurSelection);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInPlaceEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL


// Attributes

protected:	
	// Generated message map functions
	//{{AFX_MSG(CInPlaceEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaste(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:

// Implementation

	// Constructor
	CInPlaceEdit();

	// Hide the copy constructor and operator =
	CInPlaceEdit (CInPlaceEdit&) {}

	operator = (CInPlaceEdit) {}
	
	// Destructor
	virtual ~CInPlaceEdit();

// Attributes

	// Index of the item in the list control
	int m_iRowIndex;

	// Index of the subitem in the list control
	int m_iColumnIndex;

	// To indicate whether ESC key was pressed
	BOOL m_bESC;
	
	// Valid characters
	CString m_strValidChars;

	// Singleton instance
	static CInPlaceEdit* m_pInPlaceEdit;

	// Previous string value in the edit control
	CString m_strWindowText;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPLACEEDIT_H__175AEDFF_731E_4721_8399_DE406A465861__INCLUDED_)
