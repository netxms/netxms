#if !defined(AFX_OBJECTVIEW_H__9122B452_70C9_48BA_802A_34201206A20E__INCLUDED_)
#define AFX_OBJECTVIEW_H__9122B452_70C9_48BA_802A_34201206A20E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectView.h : header file
//

//
// Hash for tree items
//

struct OBJ_TREE_HASH
{
   DWORD dwObjectId;
   HTREEITEM hTreeItem;
};


//
// libnxcl object index structure
//

struct NXC_OBJECT_INDEX
{
   DWORD dwKey;
   NXC_OBJECT *pObject;
};


/////////////////////////////////////////////////////////////////////////////
// CObjectView window

class CObjectView : public CWnd
{
// Construction
public:
	CObjectView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectView)
	//}}AFX_VIRTUAL

// Implementation
public:
	void OnObjectChange(DWORD dwObjectId, NXC_OBJECT *pObject);
	virtual ~CObjectView();

	// Generated message map functions
protected:
	void ChangeMgmtStatus(DWORD dwObjectId, BOOL bStatus);
	CImageList m_imageList;
	DWORD m_dwTreeHashSize;
	OBJ_TREE_HASH * m_pTreeHash;
	CTreeCtrl m_wndTreeCtrl;

   void AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent);
   void CreateTreeItemText(NXC_OBJECT *pObject, TCHAR *pszBuffer);
   void DeleteObjectTreeItem(HTREEITEM hRootItem);
   void UpdateObjectTree(DWORD dwObjectId, NXC_OBJECT *pObject);
   void SortTreeItems(HTREEITEM hItem);
   DWORD FindObjectInTree(DWORD dwObjectId);

	//{{AFX_MSG(CObjectView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnObjectManage();
	afx_msg void OnObjectUnmanage();
	afx_msg void OnObjectLastdcivalues();
	afx_msg void OnObjectWakeup();
	//}}AFX_MSG
   afx_msg void OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	NXC_OBJECT *m_pCurrentObject;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTVIEW_H__9122B452_70C9_48BA_802A_34201206A20E__INCLUDED_)
