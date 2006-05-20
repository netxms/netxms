#if !defined(AFX_ALARMPOPUP_H__8C16E949_E8E8_434B_9FE1_0A3974DE539B__INCLUDED_)
#define AFX_ALARMPOPUP_H__8C16E949_E8E8_434B_9FE1_0A3974DE539B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlarmPopup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAlarmPopup window

class CAlarmPopup : public CTaskBarPopupWnd
{
// Construction
public:
	CAlarmPopup();

// Attributes
public:
   NXC_ALARM *m_pAlarm;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmPopup)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAlarmPopup();

	// Generated message map functions
protected:
	CFont m_fontNormal;
	CFont m_fontBold;
	virtual void DrawContent(CDC &dc);

	//{{AFX_MSG(CAlarmPopup)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMPOPUP_H__8C16E949_E8E8_434B_9FE1_0A3974DE539B__INCLUDED_)
