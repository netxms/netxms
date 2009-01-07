#if !defined(AFX_REMOVETEMPLATEDLG_H__3417AEE3_BCA8_43D9_82C9_F63AF5643EA5__INCLUDED_)
#define AFX_REMOVETEMPLATEDLG_H__3417AEE3_BCA8_43D9_82C9_F63AF5643EA5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RemoveTemplateDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRemoveTemplateDlg dialog

class CRemoveTemplateDlg : public CDialog
{
// Construction
public:
	CRemoveTemplateDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRemoveTemplateDlg)
	enum { IDD = IDD_REMOVE_TEMPLATE };
	int		m_iRemoveDCI;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRemoveTemplateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRemoveTemplateDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REMOVETEMPLATEDLG_H__3417AEE3_BCA8_43D9_82C9_F63AF5643EA5__INCLUDED_)
