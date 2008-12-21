#if !defined(AFX_USEREDITOR_H__2D1C5AB7_A0ED_467D_A1F3_9ECACE44EE95__INCLUDED_)
#define AFX_USEREDITOR_H__2D1C5AB7_A0ED_467D_A1F3_9ECACE44EE95__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UserEditor.h : header file
//

#include "NewUserDlg.h"
#include "UserPropDlg.h"
#include "GroupPropDlg.h"
#include "PasswordChangeDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CUserEditor frame

class CUserEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CUserEditor)
protected:
	CUserEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void CreateUserObject(const TCHAR *pszName, BOOL bIsGroup, BOOL bShowProp);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SortList();
	int m_iSortDir;
	int m_iSortMode;
	int m_iSortImageBase;
	CImageList m_imageList;
	DWORD m_dwCurrentUser;
	CListCtrl m_wndListCtrl;
	virtual ~CUserEditor();

	// Generated message map functions
	//{{AFX_MSG(CUserEditor)
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnUserCreateGroup();
	afx_msg void OnUserCreateUser();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnUserProperties();
	afx_msg void OnUserDelete();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUpdateUserProperties(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUserDelete(CCmdUI* pCmdUI);
	afx_msg void OnUserSetpassword();
	afx_msg void OnUpdateUserSetpassword(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg void OnUserDBChange(int iCode, NXC_USER *pUserInfo);
   afx_msg void OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult);
   afx_msg void OnListViewItemChange(LPNMLISTVIEW pNMHDR, LRESULT *pResult);
   afx_msg void OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	int AddListItem(NXC_USER *pUser);

public:
   int GetSortMode(void) { return m_iSortMode; }
   int GetSortDir(void) { return m_iSortDir; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USEREDITOR_H__2D1C5AB7_A0ED_467D_A1F3_9ECACE44EE95__INCLUDED_)
