#if !defined(AFX_DATACOLLECTIONEDITOR_H__87785A1B_9421_4A4E_810C_97D47C7FC4EE__INCLUDED_)
#define AFX_DATACOLLECTIONEDITOR_H__87785A1B_9421_4A4E_810C_97D47C7FC4EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataCollectionEditor.h : header file
//

#include "DCIPropDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CDataCollectionEditor frame

class CDataCollectionEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CDataCollectionEditor)
protected:
	CDataCollectionEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	CDataCollectionEditor(NXC_DCI_LIST *pList);           // public constructor
	virtual ~CDataCollectionEditor();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataCollectionEditor)
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateListItem(int iItem, NXC_DCI *pItem);
	BOOL EditItem(DWORD dwIndex);
	void AddListItem(DWORD dwIndex, NXC_DCI *pItem);
	NXC_DCI_LIST * m_pItemList;
	CListCtrl m_wndListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CDataCollectionEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnItemNew();
	afx_msg void OnItemEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATACOLLECTIONEDITOR_H__87785A1B_9421_4A4E_810C_97D47C7FC4EE__INCLUDED_)
