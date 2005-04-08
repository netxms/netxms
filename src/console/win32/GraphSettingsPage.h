#if !defined(AFX_GRAPHSETTINGSPAGE_H__0FC10C59_9D77_42AF_99F4_4AFBE2EA7253__INCLUDED_)
#define AFX_GRAPHSETTINGSPAGE_H__0FC10C59_9D77_42AF_99F4_4AFBE2EA7253__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphSettingsPage.h : header file
//
#include "ColorSelector.h"


/////////////////////////////////////////////////////////////////////////////
// CGraphSettingsPage dialog

class CGraphSettingsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGraphSettingsPage)

// Construction
public:
	COLORREF m_rgbItems[MAX_GRAPH_ITEMS];
	COLORREF m_rgbLabelBkgnd;
	COLORREF m_rgbLabelText;
	COLORREF m_rgbGridLines;
	COLORREF m_rgbAxisLines;
	COLORREF m_rgbText;
	COLORREF m_rgbBackground;

	CGraphSettingsPage();
	~CGraphSettingsPage();

// Dialog Data
	//{{AFX_DATA(CGraphSettingsPage)
	enum { IDD = IDD_GRAPH_PROP_SETTINGS };
	BOOL	m_bAutoscale;
	BOOL	m_bShowGrid;
	BOOL	m_bAutoUpdate;
	DWORD	m_dwRefreshInterval;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGraphSettingsPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CColorSelector m_pwndCSItem[MAX_GRAPH_ITEMS];
	CColorSelector m_wndCSText;
	CColorSelector m_wndCSLabelText;
	CColorSelector m_wndCSLabelBkgnd;
	CColorSelector m_wndCSGridLines;
	CColorSelector m_wndCSAxisLines;
	CColorSelector m_wndCSBackground;

	// Generated message map functions
	//{{AFX_MSG(CGraphSettingsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHSETTINGSPAGE_H__0FC10C59_9D77_42AF_99F4_4AFBE2EA7253__INCLUDED_)
