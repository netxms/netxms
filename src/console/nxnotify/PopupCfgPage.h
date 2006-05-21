#if !defined(AFX_POPUPCFGPAGE_H__26340498_09FC_4CB8_BC4C_5CFF00D61D88__INCLUDED_)
#define AFX_POPUPCFGPAGE_H__26340498_09FC_4CB8_BC4C_5CFF00D61D88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PopupCfgPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPopupCfgPage dialog

class CPopupCfgPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPopupCfgPage)

// Construction
public:
	int m_nActionDblClk;
	int m_nActionRight;
	int m_nActionLeft;
	ALARM_SOUND_CFG m_soundCfg;
	CPopupCfgPage();
	~CPopupCfgPage();

// Dialog Data
	//{{AFX_DATA(CPopupCfgPage)
	enum { IDD = IDD_POUP_CFG_PAGE };
	CComboBox	m_wndComboRight;
	CComboBox	m_wndComboLeft;
	CComboBox	m_wndComboDblClk;
	int		m_nTimeout;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPopupCfgPage)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPopupCfgPage)
	afx_msg void OnChangeSound();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POPUPCFGPAGE_H__26340498_09FC_4CB8_BC4C_5CFF00D61D88__INCLUDED_)
