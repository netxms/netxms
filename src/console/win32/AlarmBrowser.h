#if !defined(AFX_ALARMBROWSER_H__0378A627_F763_4D57_87DC_76A9EF2560D3__INCLUDED_)
#define AFX_ALARMBROWSER_H__0378A627_F763_4D57_87DC_76A9EF2560D3__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlarmBrowser.h : header file
//

#include "AdvSplitter.h"


/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser frame

class CAlarmBrowser : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CAlarmBrowser)
protected:
	CAlarmBrowser();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	CAlarmBrowser(TCHAR *pszParam);  // public constructor used by desktop restore

	void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);
	NXC_ALARM *FindAlarmInList(DWORD dwAlarmId);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmBrowser)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iStateImageBase;
	void UpdateListItem(int nItem, NXC_ALARM *pAlarm);
	void RefreshAlarmList(void);
	BOOL IsNodeExist(DWORD dwNodeId);
	void AddNodeToTree(DWORD dwNodeId);
	HTREEITEM m_hTreeRoot;
	CImageList *m_pObjectImageList;
	DWORD m_dwCurrNode;
	CTreeCtrl m_wndTreeCtrl;
	CAdvSplitter m_wndSplitter;
	BOOL m_bShowNodes;
	BOOL m_bRestoredDesktop;
	void AddAlarmToList(NXC_ALARM *pAlarm);
	void DeleteAlarmFromList(DWORD dwAlarmId);
	NXC_ALARM *m_pAlarmList;
	DWORD m_dwNumAlarms;
	int m_iSortImageBase;
	int m_iSortDir;
	int m_iSortMode;
	int m_iNumAlarms[5];
	void UpdateStatusBar(void);
	int m_iStatusBarHeight;
	CStatusBarCtrl m_wndStatusBar;
	int FindAlarmRecord(DWORD dwAlarmId);
	void AddAlarm(NXC_ALARM *pAlarm);
	BOOL m_bShowAllAlarms;
	CImageList *m_pImageList;
	CListCtrl m_wndListCtrl;
	virtual ~CAlarmBrowser();

	// Generated message map functions
	//{{AFX_MSG(CAlarmBrowser)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnAlarmAcknowlege();
	afx_msg void OnUpdateAlarmAcknowlege(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnAlarmShownodes();
	afx_msg void OnUpdateAlarmShownodes(CCmdUI* pCmdUI);
	afx_msg void OnAlarmSoundconfiguration();
	afx_msg void OnAlarmTerminate();
	afx_msg void OnUpdateAlarmTerminate(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg LRESULT OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo);
   afx_msg void OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult);
   afx_msg void OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()

public:
   int SortMode(void) { return m_iSortMode; }
   int SortDir(void) { return m_iSortDir; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMBROWSER_H__0378A627_F763_4D57_87DC_76A9EF2560D3__INCLUDED_)
