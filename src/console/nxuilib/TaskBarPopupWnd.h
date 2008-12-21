#if !defined(AFX_TASKBARPOPUPWND_H__8565EB6C_EFD4_4C45_8DE1_1DCB8722AF25__INCLUDED_)
#define AFX_TASKBARPOPUPWND_H__8565EB6C_EFD4_4C45_8DE1_1DCB8722AF25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TaskBarPopupWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTaskBarPopupWnd window

class NXUILIB_EXPORTABLE CTaskBarPopupWnd : public CWnd
{
// Construction
public:
	CTaskBarPopupWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTaskBarPopupWnd)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetAttr(DWORD dwStayTime, DWORD dwAnimInt = 40, int nStep = 15);
	void Update(void);
	virtual BOOL Create(int nWidth, int nHeight, CWnd *pParentWnd);
	virtual ~CTaskBarPopupWnd();

	// Generated message map functions
protected:
	DWORD m_dwAnimationInterval;
	DWORD m_dwStayTime;
	int m_nStep;
	void DrawOnBitmap(void);
	virtual void DrawContent(CDC &dc);
	RECT m_rcCurrSize;
   RECT m_rcClient;
   int m_nWidth;
   int m_nHeight;
	int m_nIncY;
	int m_nIncX;
	//{{AFX_MSG(CTaskBarPopupWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CBitmap m_bmpContent;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TASKBARPOPUPWND_H__8565EB6C_EFD4_4C45_8DE1_1DCB8722AF25__INCLUDED_)
