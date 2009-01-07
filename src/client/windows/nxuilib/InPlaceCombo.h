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

#if !defined(AFX_INPLACECOMBO_H__2E04D8D9_827F_4FBD_9E87_30AF8C31639D__INCLUDED_)
#define AFX_INPLACECOMBO_H__2E04D8D9_827F_4FBD_9E87_30AF8C31639D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class NXUILIB_EXPORTABLE CInPlaceCombo : public CComboBox
{
public:
		
// Implementation
	
	// Returns the instance of the class
	static CInPlaceCombo* GetInstance(); 

	// Deletes the instance of the class
	static void DeleteInstance(); 

	// Creates the Windows combo control and attaches it to the object, if needed and shows the combo ctrl
	BOOL ShowComboCtrl(DWORD dwStyle, const CRect& rCellRect, CWnd* pParentWnd, UINT uiResourceID,
					   int iRowIndex, int iColumnIndex, CStringList* pDropDownList, CString strCurSelecetion = _T(""), int iCurSel = -1);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInPlaceCombo)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

protected:
	int m_nOffset;

	// Generated message map functions
	//{{AFX_MSG(CInPlaceCombo)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnCloseup();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

// Implementation
	// Constructor
	CInPlaceCombo();

	// Hide the copy constructor and operator =
	CInPlaceCombo (CInPlaceCombo&) {}

	//CInPlaceCombo operator = (CInPlaceCombo) {}

	// Destructor
	virtual ~CInPlaceCombo();

// Attributes

	// Index of the item in the list control
	int m_iRowIndex;

	// Index of the subitem in the list control
	int m_iColumnIndex;

	// To indicate whether ESC key was pressed
	BOOL m_bESC;
	
	// Singleton instance
	static CInPlaceCombo* m_pInPlaceCombo;

	// Previous selected string value in the combo control
	CString m_strWindowText;

	// List of items to be shown in the drop down
	CStringList m_DropDownList;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPLACECOMBO_H__2E04D8D9_827F_4FBD_9E87_30AF8C31639D__INCLUDED_)