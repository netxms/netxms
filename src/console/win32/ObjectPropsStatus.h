#if !defined(AFX_OBJECTPROPSSTATUS_H__F62963D6_0352_4013_99D8_A5F2575A18BD__INCLUDED_)
#define AFX_OBJECTPROPSSTATUS_H__F62963D6_0352_4013_99D8_A5F2575A18BD__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
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
	int		m_iFixedStatus;
	int		m_iStatusTranslation1;
	int		m_iStatusTranslation2;
	int		m_iStatusTranslation3;
	int		m_iStatusTranslation4;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsStatus)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnCalcAlgChange(void);
	void OnPropAlgChange(void);
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsStatus)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bIsModified;
	NXC_OBJECT_UPDATE *m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSSTATUS_H__F62963D6_0352_4013_99D8_A5F2575A18BD__INCLUDED_)
