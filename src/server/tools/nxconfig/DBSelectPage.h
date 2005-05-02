#if !defined(AFX_DBSELECTPAGE_H__8FB40671_C871_41DD_AFE9_CA605ECD3D1B__INCLUDED_)
#define AFX_DBSELECTPAGE_H__8FB40671_C871_41DD_AFE9_CA605ECD3D1B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DBSelectPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDBSelectPage dialog

class CDBSelectPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDBSelectPage)

// Construction
public:
	CDBSelectPage();
	~CDBSelectPage();

// Dialog Data
	//{{AFX_DATA(CDBSelectPage)
	enum { IDD = IDD_SELECT_DB };
	CComboBox	m_wndEngineList;
	CComboBox	m_wndDrvList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDBSelectPage)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnDBCreationSelect(void);
	void OnEngineSelect(void);
	void OnDriverSelect(void);
	// Generated message map functions
	//{{AFX_MSG(CDBSelectPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboDbengine();
	afx_msg void OnRadioNewdb();
	afx_msg void OnRadioExistingdb();
	afx_msg void OnCheckInitdb();
	afx_msg void OnSelchangeComboDbdrv();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBSELECTPAGE_H__8FB40671_C871_41DD_AFE9_CA605ECD3D1B__INCLUDED_)
