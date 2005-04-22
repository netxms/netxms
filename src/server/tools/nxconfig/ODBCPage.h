#if !defined(AFX_ODBCPAGE_H__56EDDF17_CD0E_4BB3_9A97_0CC4FDA4FB26__INCLUDED_)
#define AFX_ODBCPAGE_H__56EDDF17_CD0E_4BB3_9A97_0CC4FDA4FB26__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ODBCPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CODBCPage dialog

class CODBCPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CODBCPage)

// Construction
public:
	CODBCPage();
	~CODBCPage();

// Dialog Data
	//{{AFX_DATA(CODBCPage)
	enum { IDD = IDD_ODBC };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CODBCPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CODBCPage)
	afx_msg void OnButtonOdbc();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ODBCPAGE_H__56EDDF17_CD0E_4BB3_9A97_0CC4FDA4FB26__INCLUDED_)
