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

#if !defined(AFX_COMBOLISTCTRL_H__9089600F_374F_4BFC_9482_DEAC0E7133E8__INCLUDED_)
#define AFX_COMBOLISTCTRL_H__9089600F_374F_4BFC_9482_DEAC0E7133E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>

class CInPlaceCombo;
class CInPlaceEdit;

// User define message 
// This message is posted to the parent
// The message can be handled to make the necessary validations, if any
#define CLN_VALIDATE		WM_USER + 0x7FFD

// User define message 
// This message is posted to the parent
// The message should be handled to specify the items to the added to the combo
#define CLN_SETITEMS		WM_USER + 0x7FFC

class NXUILIB_EXPORTABLE CComboListCtrl : public CListCtrl
{
public:
	
// Implementation

	// Constructor
	CComboListCtrl();

	// Destructor
	virtual ~CComboListCtrl();

	// Sets/Resets the column which support the in place combo box
	void SetComboColumn(int iColumnIndex, bool bSet = true);
	
	// Sets/Resets the column which support the in place edit control
	void SetReadOnlyColumn(int iColumnIndex, bool bSet = true);

	// Sets/Resets the column which support the in place color picker
	void SetColorPickerColumn(int iColumnIndex, bool bSet = true);
	
	// Sets the valid characters for the edit ctrl
	void SetValidEditCtrlCharacters(CString& rstrValidCharacters);

	// Sets the vertical scroll
	void EnableVScroll(bool bEnable = true);

	// Sets the horizontal scroll
	void EnableHScroll(bool bEnable = true);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CComboListCtrl)
	//}}AFX_VIRTUAL

protected:

// Methods
	// Generated message map functions
	//{{AFX_MSG(CComboListCtrl)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT OnSelEndOK(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
			
// Implementation

	// Returns the row & column index of the column on which mouse click event has occured
	bool HitTestEx(CPoint& rHitPoint, int* pRowIndex, int* pColumnIndex) const;

	// Creates and displays the in place combo box
	CInPlaceCombo* ShowInPlaceList(int iRowIndex, int iColumnIndex, CStringList& rComboItemsList, 
								   CString strCurSelecetion = _T(""), int iSel = -1);

	// Creates and displays the in place edit control
	CInPlaceEdit* ShowInPlaceEdit(int iRowIndex, int iColumnIndex, CString& rstrCurSelection);

	// Creates and displays the in place color picker
	void ShowInPlaceColorPicker(int iRowIndex, int iColumnIndex, CString& rstrCurSelection);

	// Calculates the cell rect
	void CalculateCellRect(int iColumnIndex, int iRowIndex, CRect& robCellRect);

	// Checks whether column supports in place combo box
	bool IsCombo(int iColumnIndex);

	// Checks whether column is read only
	bool IsReadOnly(int iColumnIndex);

	// Checks whether column supports in place color picker
	bool IsColorPicker(int iColumnIndex);

	// Scrolls the list ctrl to bring the in place ctrl to the view
	void ScrollToView(int iColumnIndex, /*int iOffSet, */CRect& obCellRect);

// Attributes
	
	// List of columns that support the in place combo box
	CList<int, int> m_ComboSupportColumnsList;

	// List of columns that are read only
	CList<int, int> m_ReadOnlyColumnsList;

	// List of columns that support the in place color picker
	CList<int, int> m_ColorPickerColumnsList;

	// Valid characters
	CString m_strValidEditCtrlChars;

	// The window style of the in place edit ctrl
	DWORD m_dwEditCtrlStyle;

	// The window style of the in place combo ctrl
	DWORD m_dwDropDownCtrlStyle;

	// Cell currently edited with color picker
	int m_nCPRow;
	int m_nCPColumn;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMBOLISTCTRL_H__9089600F_374F_4BFC_9482_DEAC0E7133E8__INCLUDED_)
