#if !defined(AFX_OBJECTBROWSER_H__FD184738_838F_452D_B786_447053807679__INCLUDED_)
#define AFX_OBJECTBROWSER_H__FD184738_838F_452D_B786_447053807679__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectBrowser.h : header file
//

#include "ObjectPreview.h"


#define SHOW_OBJECT_PREVIEW      ((DWORD)0x00000001)
#define FOLLOW_OBJECT_UPDATES    ((DWORD)0x00000002)
#define VIEW_OBJECTS_AS_TREE     ((DWORD)0x00000004)


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
// CObjectBrowser frame

class CObjectBrowser : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CObjectBrowser)
protected:
	CObjectBrowser();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectBrowser)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void AddObjectToList(NXC_OBJECT *pObject);
	CListCtrl m_wndListCtrl;
	void DeleteObjectTreeItem(HTREEITEM hItem);
	DWORD m_dwFlags;
	void CreateTreeItemText(NXC_OBJECT *pObject, char *pszBuffer);
	DWORD FindObjectInTree(DWORD dwObjectId);
	DWORD m_dwTreeHashSize;
	OBJ_TREE_HASH * m_pTreeHash;
	CTreeCtrl m_wndTreeCtrl;
   CObjectPreview m_wndPreviewPane;
	virtual ~CObjectBrowser();

	// Generated message map functions
	//{{AFX_MSG(CObjectBrowser)
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnRclickTreeView(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnObjectViewShowpreviewpane();
   afx_msg void OnTreeViewSelChange(LPNMTREEVIEW lpnmt);
   afx_msg void OnObjectChange(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void UpdateObjectTree(DWORD dwObjectId, NXC_OBJECT *pObject);
	CImageList *m_pImageList;
	void AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hRoot);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTBROWSER_H__FD184738_838F_452D_B786_447053807679__INCLUDED_)
