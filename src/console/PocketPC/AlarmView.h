#if !defined(AFX_ALARMVIEW_H__97CA7D6F_BEE9_4F1D_A0D6_B039D034CBED__INCLUDED_)
#define AFX_ALARMVIEW_H__97CA7D6F_BEE9_4F1D_A0D6_B039D034CBED__INCLUDED_

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

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmView)
	//}}AFX_VIRTUAL

// Implementation
public:
	int m_iNumAlarms[5];
	virtual ~CAlarmView();

   void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);

	// Generated message map functions
protected:
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMVIEW_H__97CA7D6F_BEE9_4F1D_A0D6_B039D034CBED__INCLUDED_)
