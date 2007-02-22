#if !defined(AFX_GRAPHSETTINGSPAGE_H__0FC10C59_9D77_42AF_99F4_4AFBE2EA7253__INCLUDED_)
#define AFX_GRAPHSETTINGSPAGE_H__0FC10C59_9D77_42AF_99F4_4AFBE2EA7253__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphSettingsPage.h : header file
//
#include "GraphFrame.h"
#include "ColorSelector.h"


#define MAX_TIME_UNITS     3


/////////////////////////////////////////////////////////////////////////////
// CGraphSettingsPage dialog

class CGraphSettingsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CGraphSettingsPage)

// Construction
public:
	int m_iTimeUnit;
	COLORREF m_rgbSelection;
	COLORREF m_rgbGridLines;
	COLORREF m_rgbAxisLines;
	COLORREF m_rgbText;
	COLORREF m_rgbBackground;
	COLORREF m_rgbRuler;

	CGraphSettingsPage();
	~CGraphSettingsPage();

// Dialog Data
	//{{AFX_DATA(CGraphSettingsPage)
	enum { IDD = IDD_GRAPH_PROP_SETTINGS };
	CComboBox	m_wndTimeUnits;
	BOOL	m_bAutoscale;
	BOOL	m_bShowGrid;
	BOOL	m_bAutoUpdate;
	DWORD	m_dwRefreshInterval;
	DWORD	m_dwNumUnits;
	int		m_iTimeFrame;
	CTime	m_dateFrom;
	CTime	m_dateTo;
	CTime	m_timeFrom;
	CTime	m_timeTo;
	BOOL	m_bRuler;
	BOOL	m_bShowLegend;
	BOOL	m_bEnableZoom;
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
	CColourPickerXP m_wndCSText;
	CColourPickerXP m_wndCSRuler;
	CColourPickerXP m_wndCSSelection;
	CColourPickerXP m_wndCSGridLines;
	CColourPickerXP m_wndCSAxisLines;
	CColourPickerXP m_wndCSBackground;

	// Generated message map functions
	//{{AFX_MSG(CGraphSettingsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioFromNow();
	afx_msg void OnRadioFixed();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHSETTINGSPAGE_H__0FC10C59_9D77_42AF_99F4_4AFBE2EA7253__INCLUDED_)
