#if !defined(AFX_DATAQUERYDLG_H__53A57AD5_A364_4E31_AE9D_01DE857A02B7__INCLUDED_)
#define AFX_DATAQUERYDLG_H__53A57AD5_A364_4E31_AE9D_01DE857A02B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataQueryDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDataQueryDlg dialog

class CDataQueryDlg : public CDialog
{
// Construction
public:
	DWORD m_dwObjectId;
	int m_iOrigin;
	CDataQueryDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDataQueryDlg)
	enum { IDD = IDD_DATA_QUERY };
	CEdit	m_wndEditValue;
	CString	m_strNode;
	CString	m_strParameter;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataQueryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDataQueryDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonRestart();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAQUERYDLG_H__53A57AD5_A364_4E31_AE9D_01DE857A02B7__INCLUDED_)
