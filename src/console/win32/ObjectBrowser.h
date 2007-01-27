#if !defined(AFX_OBJECTBROWSER_H__C5713E3C_F480_4834_80D6_CFFF28A95911__INCLUDED_)
#define AFX_OBJECTBROWSER_H__C5713E3C_F480_4834_80D6_CFFF28A95911__INCLUDED_

#include "AdvSplitter.h"	// Added by ClassView
#include "ObjectView.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectBrowser.h : header file
//

#define FOLLOW_OBJECT_UPDATES    ((DWORD)0x00000001)


#define OBJECT_FIND_FIRST		0x00
#define OBJECT_FIND_NEXT		0x01
#define OBJECT_FIND_PREV		0x02


//
// Hash for tree items
//

struct OBJ_TREE_HASH
{
   DWORD dwObjectId;
   DWORD dwNumEntries;
   HTREEITEM *phTreeItemList;
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
   CObjectBrowser(TCHAR *pszParams);

   void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectBrowser)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void FindNextObjectEntry(void);
	void OpenObject(DWORD dwObjectId);
   BOOL CurrObjectIsNode(BOOL bIncludeTemplates);
   BOOL CurrObjectIsInterface(void);
	DWORD GetSelectedObject(void);
	DWORD m_dwFlags;
	OBJ_TREE_HASH *m_pTreeHash;
	DWORD m_dwTreeHashSize;
	NXC_OBJECT * m_pCurrentObject;
   void UpdateObjectTree(DWORD dwObjectId, NXC_OBJECT *pObject);
	DWORD FindObjectInTree(DWORD dwObjectId);
	void CreateTreeItemText(NXC_OBJECT *pObject, TCHAR *pszBuffer);
	void AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent);
   void AddObjectEntryToHash(NXC_OBJECT *pObject, HTREEITEM hItem);
   void SortTreeItems(HTREEITEM hItem);
	DWORD AdjustIndex(DWORD dwIndex, DWORD dwObjectId);
	void DeleteObjectTreeItem(HTREEITEM hItem);
	CObjectView m_wndObjectView;
	int m_nStatusImageBase;
	int m_nLastObjectImage;
	CImageList * m_pImageList;
	CTreeCtrl m_wndTreeCtrl;
	CAdvSplitter m_wndSplitter;
	virtual ~CObjectBrowser();

	// Generated message map functions
	//{{AFX_MSG(CObjectBrowser)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnObjectProperties();
	afx_msg void OnUpdateObjectProperties(CCmdUI* pCmdUI);
	afx_msg void OnObjectAgentcfg();
	afx_msg void OnUpdateObjectAgentcfg(CCmdUI* pCmdUI);
	afx_msg void OnObjectApply();
	afx_msg void OnUpdateObjectApply(CCmdUI* pCmdUI);
	afx_msg void OnObjectBind();
	afx_msg void OnUpdateObjectBind(CCmdUI* pCmdUI);
	afx_msg void OnObjectChangeipaddress();
	afx_msg void OnUpdateObjectChangeipaddress(CCmdUI* pCmdUI);
	afx_msg void OnObjectComments();
	afx_msg void OnUpdateObjectComments(CCmdUI* pCmdUI);
	afx_msg void OnObjectCreateCondition();
	afx_msg void OnObjectCreateContainer();
	afx_msg void OnObjectCreateNode();
	afx_msg void OnObjectCreateService();
	afx_msg void OnObjectCreateTemplate();
	afx_msg void OnObjectCreateTemplategroup();
	afx_msg void OnObjectCreateVpnconnector();
	afx_msg void OnObjectDatacollection();
	afx_msg void OnUpdateObjectDatacollection(CCmdUI* pCmdUI);
	afx_msg void OnObjectDelete();
	afx_msg void OnUpdateObjectDelete(CCmdUI* pCmdUI);
	afx_msg void OnObjectLastdcivalues();
	afx_msg void OnUpdateObjectLastdcivalues(CCmdUI* pCmdUI);
	afx_msg void OnObjectManage();
	afx_msg void OnUpdateObjectManage(CCmdUI* pCmdUI);
	afx_msg void OnObjectMove();
	afx_msg void OnUpdateObjectMove(CCmdUI* pCmdUI);
	afx_msg void OnObjectPollConfiguration();
	afx_msg void OnUpdateObjectPollConfiguration(CCmdUI* pCmdUI);
	afx_msg void OnObjectPollStatus();
	afx_msg void OnUpdateObjectPollStatus(CCmdUI* pCmdUI);
	afx_msg void OnObjectUnbind();
	afx_msg void OnUpdateObjectUnbind(CCmdUI* pCmdUI);
	afx_msg void OnObjectUnmanage();
	afx_msg void OnUpdateObjectUnmanage(CCmdUI* pCmdUI);
	afx_msg void OnObjectCreateCluster();
	afx_msg void OnObjectFind();
	//}}AFX_MSG
   afx_msg void OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult);
   afx_msg void OnTreeViewGetDispInfo(LPNMTVDISPINFO lpdi, LRESULT *pResult);
   afx_msg void OnTreeViewItemExpanding(LPNMTREEVIEW lpnmt, LRESULT *pResult);
   afx_msg void OnObjectChange(WPARAM wParam, LPARAM lParam);
   afx_msg void OnObjectTool(UINT nID);
   afx_msg void OnFindObject(WPARAM wParam, TCHAR *pszSearchStr);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTBROWSER_H__C5713E3C_F480_4834_80D6_CFFF28A95911__INCLUDED_)
