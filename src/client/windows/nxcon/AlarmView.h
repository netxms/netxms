#if !defined(AFX_ALARMVIEW_H__FD1B4F5C_6CA1_4084_9F05_2384212DB9C8__INCLUDED_)
#define AFX_ALARMVIEW_H__FD1B4F5C_6CA1_4084_9F05_2384212DB9C8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlarmView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAlarmView window

class CAlarmView : public CWnd
{
// Construction
public:
	CAlarmView();

// Attributes
public:

// Operations
public:
   NXC_ALARM *FindAlarmInList(DWORD dwAlarmId);
   void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmView)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAlarmView();

	// Generated message map functions
protected:
	DWORD m_dwNumAlarms;
	NXC_ALARM * m_pAlarmList;
   void AddAlarmToList(NXC_ALARM *pAlarm);
   void DeleteAlarmFromList(DWORD dwAlarmId);
   void AddAlarm(NXC_ALARM *pAlarm);
   void UpdateListItem(int nItem, NXC_ALARM *pAlarm);
   int FindAlarmListItem(DWORD dwAlarmId);
	BOOL MatchAlarm(NXC_ALARM *pAlarm);
	NXC_OBJECT *m_pObject;
	int m_iSortMode;
	int m_iSortDir;
	int m_iStateImageBase;
	int m_iSortImageBase;
	CImageList * m_pImageList;
	CListCtrl m_wndListCtrl;
	//{{AFX_MSG(CAlarmView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnAlarmAcknowledge();
	afx_msg void OnUpdateAlarmAcknowledge(CCmdUI* pCmdUI);
	afx_msg void OnAlarmDelete();
	afx_msg void OnUpdateAlarmDelete(CCmdUI* pCmdUI);
	afx_msg void OnAlarmDetails();
	afx_msg void OnUpdateAlarmDetails(CCmdUI* pCmdUI);
	afx_msg void OnAlarmLastdcivalues();
	afx_msg void OnUpdateAlarmLastdcivalues(CCmdUI* pCmdUI);
	afx_msg void OnAlarmTerminate();
	afx_msg void OnUpdateAlarmTerminate(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg LRESULT OnSetObject(WPARAM wParam, LPARAM lParam);
   afx_msg void OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnObjectTool(UINT nID);
   afx_msg void OnUpdateObjectTool(CCmdUI *pCmdUI);
	DECLARE_MESSAGE_MAP()

public:
   int SortMode(void) { return m_iSortMode; }
   int SortDir(void) { return m_iSortDir; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMVIEW_H__FD1B4F5C_6CA1_4084_9F05_2384212DB9C8__INCLUDED_)
