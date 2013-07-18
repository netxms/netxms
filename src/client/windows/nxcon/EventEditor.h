#if !defined(AFX_EVENTEDITOR_H__A45B4A38_99BC_4277_B600_742C39D58310__INCLUDED_)
#define AFX_EVENTEDITOR_H__A45B4A38_99BC_4277_B600_742C39D58310__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EventEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventEditor frame

class CEventEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CEventEditor)
protected:
	CEventEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	int CompareListItems(DWORD dwId1, DWORD dwId2);
   DWORD DeleteEvents(DWORD dwNumEvents, DWORD *pdwEventList);
	void OnEventDBUpdate(DWORD code, NXC_EVENT_TEMPLATE *etmpl);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iSortMode;
	int m_iSortDir;
	int m_iSortImageBase;
	void UpdateItem(int iItem, NXC_EVENT_TEMPLATE *pData);
	CImageList *m_pImageList;
	BOOL EditEvent(int iItem);
	UINT32 m_dwNumTemplates;
	NXC_EVENT_TEMPLATE ** m_ppEventTemplates;
	CListCtrl m_wndListCtrl;
	virtual ~CEventEditor();

	// Generated message map functions
	//{{AFX_MSG(CEventEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnClose();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUpdateEventEdit(CCmdUI* pCmdUI);
	afx_msg void OnEventEdit();
	afx_msg void OnUpdateEventDelete(CCmdUI* pCmdUI);
	afx_msg void OnEventNew();
	afx_msg void OnEventDelete();
	afx_msg void OnUpdateEventList();
	//}}AFX_MSG
	afx_msg void OnListViewDoubleClick(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTEDITOR_H__A45B4A38_99BC_4277_B600_742C39D58310__INCLUDED_)
