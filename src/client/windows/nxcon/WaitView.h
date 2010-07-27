#if !defined(AFX_WAITVIEW_H__75084C02_DB22_4977_BD8F_E5092C21A222__INCLUDED_)
#define AFX_WAITVIEW_H__75084C02_DB22_4977_BD8F_E5092C21A222__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaitView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaitView window

class CWaitView : public CWnd
{
// Construction
public:
	CWaitView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void Stop(void);
	void Start(void);
	virtual ~CWaitView();

	// Generated message map functions
protected:
	int m_nThreshold;
	int m_nMaxPos;
	int m_nCurrPos;
	int m_nTextHeight;
	CFont m_font;
	TCHAR m_szText[MAX_DB_STRING];
	CProgressCtrl m_wndProgressBar;
	CStatic m_wndText;
	UINT_PTR m_nTimer;
	//{{AFX_MSG(CWaitView)
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   void SetText(TCHAR *pszText) { nx_strncpy(m_szText, pszText, MAX_DB_STRING); }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITVIEW_H__75084C02_DB22_4977_BD8F_E5092C21A222__INCLUDED_)
