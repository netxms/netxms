#if !defined(AFX_AGENTCONFIGMGR_H__AE44746B_D3EB_407A_9116_3DCBF67F101B__INCLUDED_)
#define AFX_AGENTCONFIGMGR_H__AE44746B_D3EB_407A_9116_3DCBF67F101B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AgentConfigMgr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAgentConfigMgr frame

class CAgentConfigMgr : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CAgentConfigMgr)
protected:
	CAgentConfigMgr();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAgentConfigMgr)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SwapItems(int nItem1, int nItem2);
	void EditConfig(NXC_AGENT_CONFIG *pConfig);
	int m_iSortMode;
	int m_iSortDir;
	CImageList m_imageList;
	CListCtrl m_wndListCtrl;
	virtual ~CAgentConfigMgr();

	// Generated message map functions
	//{{AFX_MSG(CAgentConfigMgr)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewRefresh();
	afx_msg void OnUpdateConfigDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateConfigMoveup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateConfigMovedown(CCmdUI* pCmdUI);
	afx_msg void OnUpdateConfigEdit(CCmdUI* pCmdUI);
	afx_msg void OnConfigNew();
	afx_msg void OnConfigEdit();
	afx_msg void OnConfigDelete();
	afx_msg void OnConfigMoveup();
	afx_msg void OnConfigMovedown();
	//}}AFX_MSG
   afx_msg void OnListViewDblClk(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()

public:
	void GetItemText(int item, int col, TCHAR *buffer);
   int GetSortMode(void) { return m_iSortMode; }
   int GetSortDir(void) { return m_iSortDir; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AGENTCONFIGMGR_H__AE44746B_D3EB_407A_9116_3DCBF67F101B__INCLUDED_)
