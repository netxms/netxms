#if !defined(AFX_GRAPHPROPDLG_H__2C17056D_6D61_4B55_9D1A_C7BE1338047F__INCLUDED_)
#define AFX_GRAPHPROPDLG_H__2C17056D_6D61_4B55_9D1A_C7BE1338047F__INCLUDED_

#include "ColorSelector.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGraphPropDlg dialog

class CGraphPropDlg : public CDialog
{
// Construction
public:
	COLORREF m_rgbItems[MAX_GRAPH_ITEMS];
	COLORREF m_rgbLabelBkgnd;
	COLORREF m_rgbLabelText;
	COLORREF m_rgbGridLines;
	COLORREF m_rgbAxisLines;
	COLORREF m_rgbText;
	COLORREF m_rgbBackground;
	CGraphPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGraphPropDlg)
	enum { IDD = IDD_GRAPH_PROPERTIES };
	DWORD	m_dwRefreshInterval;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphPropDlg)
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
	//{{AFX_MSG(CGraphPropDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHPROPDLG_H__2C17056D_6D61_4B55_9D1A_C7BE1338047F__INCLUDED_)
