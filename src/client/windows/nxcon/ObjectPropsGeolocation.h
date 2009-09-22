#if !defined(AFX_OBJECTPROPSGEOLOCATION_H__720D8C5E_2F21_4BDE_9A18_582649F660B3__INCLUDED_)
#define AFX_OBJECTPROPSGEOLOCATION_H__720D8C5E_2F21_4BDE_9A18_582649F660B3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsGeolocation.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsGeolocation dialog

class CObjectPropsGeolocation : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsGeolocation)

// Construction
public:
	CObjectPropsGeolocation();
	~CObjectPropsGeolocation();

// Dialog Data
	//{{AFX_DATA(CObjectPropsGeolocation)
	enum { IDD = IDD_OBJECT_GEOLOCATION };
	int		m_locationType;
	CString	m_strLatitude;
	CString	m_strLongitude;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsGeolocation)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsGeolocation)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioGps();
	afx_msg void OnRadioManual();
	afx_msg void OnRadioUndefined();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSGEOLOCATION_H__720D8C5E_2F21_4BDE_9A18_582649F660B3__INCLUDED_)
