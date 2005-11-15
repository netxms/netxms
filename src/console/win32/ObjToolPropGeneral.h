#if !defined(AFX_OBJTOOLPROPGENERAL_H__C868C56C_F8F6_4260_8096_45EE7ACC3021__INCLUDED_)
#define AFX_OBJTOOLPROPGENERAL_H__C868C56C_F8F6_4260_8096_45EE7ACC3021__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjToolPropGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropGeneral dialog

class CObjToolPropGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjToolPropGeneral)

// Construction
public:
	int m_iToolType;
	CObjToolPropGeneral();
	~CObjToolPropGeneral();

// Dialog Data
	//{{AFX_DATA(CObjToolPropGeneral)
	enum { IDD = IDD_OBJTOOL_GENERAL };
	CListCtrl	m_wndListCtrl;
	CString	m_strDescription;
	CString	m_strName;
	CString	m_strRegEx;
	CString	m_strEnum;
	CString	m_strData;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjToolPropGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjToolPropGeneral)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJTOOLPROPGENERAL_H__C868C56C_F8F6_4260_8096_45EE7ACC3021__INCLUDED_)
