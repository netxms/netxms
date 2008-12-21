#if !defined(AFX_OBJECTPROPSPRESENTATION_H__B69D7656_DCC0_40AA_A72E_8FBCA93817B7__INCLUDED_)
#define AFX_OBJECTPROPSPRESENTATION_H__B69D7656_DCC0_40AA_A72E_8FBCA93817B7__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsPresentation.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsPresentation dialog

class CObjectPropsPresentation : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsPresentation)

// Construction
public:
	DWORD m_dwImageId;
	CObjectPropsPresentation();
	~CObjectPropsPresentation();

// Dialog Data
	//{{AFX_DATA(CObjectPropsPresentation)
	enum { IDD = IDD_OBJECT_PRESENTATION };
	CStatic	m_wndIcon;
	CButton	m_wndSelButton;
	BOOL	m_bUseDefaultImage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsPresentation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsPresentation)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSPRESENTATION_H__B69D7656_DCC0_40AA_A72E_8FBCA93817B7__INCLUDED_)
