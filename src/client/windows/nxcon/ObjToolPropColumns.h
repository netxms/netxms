#if !defined(AFX_OBJTOOLPROPCOLUMNS_H__9C6A9169_AC06_48A9_9D74_D63CFECCC07E__INCLUDED_)
#define AFX_OBJTOOLPROPCOLUMNS_H__9C6A9169_AC06_48A9_9D74_D63CFECCC07E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjToolPropColumns.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns dialog

class CObjToolPropColumns : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjToolPropColumns)

// Construction
public:
	NXC_OBJECT_TOOL_COLUMN *m_pColumnList;
	DWORD m_dwNumColumns;
	int m_iToolType;
	CObjToolPropColumns();
	~CObjToolPropColumns();

// Dialog Data
	//{{AFX_DATA(CObjToolPropColumns)
	enum { IDD = IDD_OBJTOOL_COLUMNS };
	CButton	m_wndButtonUp;
	CButton	m_wndButtonDown;
	CButton	m_wndButtonAdd;
	CButton	m_wndButtonDelete;
	CButton	m_wndButtonModify;
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjToolPropColumns)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjToolPropColumns)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedListColumns(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListColumns(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonMoveup();
	afx_msg void OnButtonMovedown();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonModify();
	afx_msg void OnButtonDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL ModifyColumn(NXC_OBJECT_TOOL_COLUMN *column, BOOL isCreate = FALSE);
	void SwapColumns(DWORD index1, DWORD index2);
	void RecalcIndexes(DWORD dwIndex);
	int AddListEntry(DWORD dwIndex);
	void UpdateListEntry(int iItem, DWORD dwIndex);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJTOOLPROPCOLUMNS_H__9C6A9169_AC06_48A9_9D74_D63CFECCC07E__INCLUDED_)
