#if !defined(AFX_DCITRANSFORMPAGE_H__E637F1DF_67A3_4F66_9E73_30B9904758DA__INCLUDED_)
#define AFX_DCITRANSFORMPAGE_H__E637F1DF_67A3_4F66_9E73_30B9904758DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCITransformPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCITransformPage dialog

class CDCITransformPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDCITransformPage)

// Construction
public:
	CDCITransformPage();
	~CDCITransformPage();

// Dialog Data
	//{{AFX_DATA(CDCITransformPage)
	enum { IDD = IDD_DCI_TRANSFORM };
	CComboBox	m_wndDeltaList;
	int		m_iDeltaProc;
	CString	m_strFormula;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDCITransformPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableWarning(void);
	// Generated message map functions
	//{{AFX_MSG(CDCITransformPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboDelta();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCITRANSFORMPAGE_H__E637F1DF_67A3_4F66_9E73_30B9904758DA__INCLUDED_)
