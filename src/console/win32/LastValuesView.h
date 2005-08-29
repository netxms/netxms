#if !defined(AFX_LASTVALUESVIEW_H__A36FB8F1_1BFA_4D01_8AA3_2A9D962B3140__INCLUDED_)
#define AFX_LASTVALUESVIEW_H__A36FB8F1_1BFA_4D01_8AA3_2A9D962B3140__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LastValuesView.h : header file
//
#include "ValueList.h"

//
// Flags
//

#define LVF_SHOW_GRID      0x01
#define LVF_AUTOREFRESH    0x02


/////////////////////////////////////////////////////////////////////////////
// CLastValuesView frame

class CLastValuesView : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CLastValuesView)
protected:
	CLastValuesView();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	int CompareListItems(LPARAM lParam1, LPARAM lParam2);
	CLastValuesView(DWORD dwNodeId);
	CLastValuesView(TCHAR *pszParams);

   DWORD GetObjectId(void) { return m_dwNodeId; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLastValuesView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iSortImageBase;
	DWORD GetDCIIndex(DWORD dwId);
	int m_iSortMode;
	int m_iSortDir;
	UINT m_nTimer;
	DWORD m_dwSeconds;
	DWORD m_dwFlags;
	DWORD FindItem(DWORD dwId);
	void UpdateItem(int iItem, NXC_DCI_VALUE *pValue);
	CImageList m_imageList;
	CValueList m_wndListCtrl;
   //CListCtrl m_wndListCtrl;
	DWORD m_dwNodeId;
   NXC_DCI_VALUE *m_pItemList;
   DWORD m_dwNumItems;
	virtual ~CLastValuesView();

	// Generated message map functions
	//{{AFX_MSG(CLastValuesView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnItemGraph();
	afx_msg void OnItemShowdata();
	afx_msg void OnUpdateItemGraph(CCmdUI* pCmdUI);
	afx_msg void OnUpdateItemShowdata(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLastvaluesProperties();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnUpdateItemExportdata(CCmdUI* pCmdUI);
	afx_msg void OnItemExportdata();
	//}}AFX_MSG
   afx_msg LRESULT OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo);
   afx_msg void OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LASTVALUESVIEW_H__A36FB8F1_1BFA_4D01_8AA3_2A9D962B3140__INCLUDED_)
