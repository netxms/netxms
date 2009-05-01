#if !defined(AFX_CONTAINERPROPSAUTOBIND_H__CF6160D0_1820_40BB_B8F1_8C776644936C__INCLUDED_)
#define AFX_CONTAINERPROPSAUTOBIND_H__CF6160D0_1820_40BB_B8F1_8C776644936C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ContainerPropsAutoBind.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CContainerPropsAutoBind dialog

class CContainerPropsAutoBind : public CPropertyPage
{
	DECLARE_DYNCREATE(CContainerPropsAutoBind)

// Construction
public:
	CContainerPropsAutoBind();
	~CContainerPropsAutoBind();

// Dialog Data
	//{{AFX_DATA(CContainerPropsAutoBind)
	enum { IDD = IDD_OBJECT_CONTAINER_AUTOBIND };
	BOOL	m_bEnableAutoBind;
	//}}AFX_DATA

	CString m_strFilterScript;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CContainerPropsAutoBind)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CContainerPropsAutoBind)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckEnable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CScintillaCtrl m_wndEditor;

private:
	NXC_OBJECT_UPDATE *m_pUpdate;
	BOOL m_bInitialEnableFlag;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONTAINERPROPSAUTOBIND_H__CF6160D0_1820_40BB_B8F1_8C776644936C__INCLUDED_)
