#if !defined(AFX_POPUPINFOWINDOW_H__6F41AA45_9B9E_4861_B3C6_1C7560751C02__INCLUDED_)
#define AFX_POPUPINFOWINDOW_H__6F41AA45_9B9E_4861_B3C6_1C7560751C02__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PopupInfoWindow.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPopupInfoWindow window

class CPopupInfoWindow : public CWnd
{
// Construction
public:
	CPopupInfoWindow();

// Attributes
public:

// Operations
public:

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPopupInfoWindow)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ShowAt(POINT pt, const TCHAR *text);
	virtual ~CPopupInfoWindow();

	// Generated message map functions
protected:
	CFont m_font;
	//{{AFX_MSG(CPopupInfoWindow)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POPUPINFOWINDOW_H__6F41AA45_9B9E_4861_B3C6_1C7560751C02__INCLUDED_)
