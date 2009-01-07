#if !defined(AFX_OBJECTPROPCAPS_H__FEF9C406_E725_43C1_B177_1E36583A7B31__INCLUDED_)
#define AFX_OBJECTPROPCAPS_H__FEF9C406_E725_43C1_B177_1E36583A7B31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropCaps.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropCaps dialog

class CObjectPropCaps : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropCaps)

// Construction
public:
	NXC_OBJECT * m_pObject;
	CObjectPropCaps();
	~CObjectPropCaps();

// Dialog Data
	//{{AFX_DATA(CObjectPropCaps)
	enum { IDD = IDD_OBJECT_CAPS };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropCaps)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CObjectPropCaps)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void AddListRecord(const TCHAR *pszName, const TCHAR *pszValue);
	void AddListRecord(const TCHAR *szName, BOOL bValue);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPCAPS_H__FEF9C406_E725_43C1_B177_1E36583A7B31__INCLUDED_)
