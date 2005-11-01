#if !defined(AFX_OBJECTPROPSSTATUS_H__F62963D6_0352_4013_99D8_A5F2575A18BD__INCLUDED_)
#define AFX_OBJECTPROPSSTATUS_H__F62963D6_0352_4013_99D8_A5F2575A18BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsStatus.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsStatus dialog

class CObjectPropsStatus : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsStatus)

// Construction
public:
	CObjectPropsStatus();
	~CObjectPropsStatus();

// Dialog Data
	//{{AFX_DATA(CObjectPropsStatus)
	enum { IDD = IDD_OBJECT_STATUS };
	CComboBox	m_wndComboS4;
	CComboBox	m_wndComboS3;
	CComboBox	m_wndComboS2;
	CComboBox	m_wndComboS1;
	CComboBox	m_wndComboFixed;
	int		m_iRelChange;
	double	m_dSingleThreshold;
	double	m_dThreshold1;
	double	m_dThreshold2;
	double	m_dThreshold3;
	double	m_dThreshold4;
	int		m_iCalcAlg;
	int		m_iPropAlg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsStatus)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsStatus)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSSTATUS_H__F62963D6_0352_4013_99D8_A5F2575A18BD__INCLUDED_)
