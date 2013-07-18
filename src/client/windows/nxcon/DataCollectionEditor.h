#if !defined(AFX_DATACOLLECTIONEDITOR_H__87785A1B_9421_4A4E_810C_97D47C7FC4EE__INCLUDED_)
#define AFX_DATACOLLECTIONEDITOR_H__87785A1B_9421_4A4E_810C_97D47C7FC4EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataCollectionEditor.h : header file
//

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

   UINT32 MoveItemsToTemplate(UINT32 dwTemplate, UINT32 dwNumItems, UINT32 *pdwItemList);
	void RefreshItemList(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataCollectionEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CopyOrMoveItems(BOOL bMove);
	int m_iSortImageBase;
	BOOL m_bIsTemplate;
	void ChangeItemsStatus(int iStatus);
	void SelectListItem(int iItem);
	void UpdateListItem(int iItem, NXC_DCI *pItem);
	BOOL EditItem(NXC_DCI *pItem);
	int AddListItem(NXC_DCI *pItem);
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
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnUpdateItemEdit(CCmdUI* pCmdUI);
	afx_msg void OnItemDelete();
	afx_msg void OnUpdateItemDelete(CCmdUI* pCmdUI);
	afx_msg void OnItemShowdata();
	afx_msg void OnUpdateItemShowdata(CCmdUI* pCmdUI);
	afx_msg void OnItemGraph();
	afx_msg void OnUpdateItemGraph(CCmdUI* pCmdUI);
	afx_msg void OnItemCopy();
	afx_msg void OnFileExport();
	afx_msg void OnUpdateFileExport(CCmdUI* pCmdUI);
	afx_msg void OnViewRefresh();
	afx_msg void OnItemDuplicate();
	afx_msg void OnUpdateItemCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateItemDuplicate(CCmdUI* pCmdUI);
	afx_msg void OnItemDisable();
	afx_msg void OnItemActivate();
	afx_msg void OnUpdateItemActivate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateItemDisable(CCmdUI* pCmdUI);
	afx_msg void OnItemExportdata();
	afx_msg void OnUpdateItemExportdata(CCmdUI* pCmdUI);
	afx_msg void OnUpdateItemMovetotemplate(CCmdUI* pCmdUI);
	afx_msg void OnItemMovetotemplate();
	afx_msg void OnItemMove();
	afx_msg void OnUpdateItemMove(CCmdUI* pCmdUI);
	afx_msg void OnItemCleardata();
	afx_msg void OnUpdateItemCleardata(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg void OnListViewDblClk(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	int m_iSortDir;
	int m_iSortMode;
	CImageList m_imageList;

public:
   int SortDir(void) { return m_iSortDir; }
   int SortMode(void) { return m_iSortMode; }
   NXC_DCI *GetItem(DWORD dwItemId)
   {
      DWORD dwIndex;

      dwIndex = NXCItemIndex(m_pItemList, dwItemId);
      return (dwIndex != INVALID_INDEX) ? &m_pItemList->pItems[dwIndex] : NULL;
   }
	NXC_DCI_LIST *GetDCIList(void) { return m_pItemList; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATACOLLECTIONEDITOR_H__87785A1B_9421_4A4E_810C_97D47C7FC4EE__INCLUDED_)
