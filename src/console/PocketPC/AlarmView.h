#if !defined(AFX_ALARMVIEW_H__97CA7D6F_BEE9_4F1D_A0D6_B039D034CBED__INCLUDED_)
#define AFX_ALARMVIEW_H__97CA7D6F_BEE9_4F1D_A0D6_B039D034CBED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlarmView.h : header file
//

#define SORT_BY_SEVERITY      0
#define SORT_BY_SOURCE        1
#define SORT_BY_MESSAGE       2
#define SORT_BY_TIMESTAMP     3


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

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmView)
	//}}AFX_VIRTUAL

// Implementation
public:
	int m_iNumAlarms[5];
	virtual ~CAlarmView();

   void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);
   NXC_ALARM *FindAlarmInList(DWORD dwAlarmId);

	// Generated message map functions
protected:
	int m_cx;
	int GetTextWidth(WCHAR *pszText);
	void UpdateAlarm(int nItem, NXC_ALARM *pAlarm);
   void AddAlarmToList(NXC_ALARM *pAlarm);
   void DeleteAlarmFromList(DWORD dwAlarmId);
	int FindAlarmRecord(DWORD dwAlarmId);
   NXC_ALARM *m_pAlarmList;
   DWORD m_dwNumAlarms;
	int m_iSortMode;
   int m_iSortDir;
	CFont m_fontLarge;
	CFont m_fontSmall;
	void DrawListItem(CDC &dc, RECT &rcItem, int iItem, UINT nData);
	void AddAlarm(NXC_ALARM *pAlarm);
	CImageList *m_pImageList;
	CListCtrl m_wndListCtrl;
	//{{AFX_MSG(CAlarmView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnAlarmSortbyAscending();
	afx_msg void OnAlarmSortbyDescending();
	afx_msg void OnAlarmSortbySeverity();
	afx_msg void OnAlarmSortbySource();
	afx_msg void OnAlarmSortbyMessagetext();
	afx_msg void OnAlarmSortbyTimestamp();
	afx_msg void OnAlarmAcknowledge();
	afx_msg void OnAlarmDelete();
	afx_msg void OnAlarmTerminate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	COLORREF m_rgbText;
	COLORREF m_rgbLine2;
	COLORREF m_rgbLine1;
   int SortMode(void) { return m_iSortMode; }
   int SortDir(void) { return m_iSortDir; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMVIEW_H__97CA7D6F_BEE9_4F1D_A0D6_B039D034CBED__INCLUDED_)
