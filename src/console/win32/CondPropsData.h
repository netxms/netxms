#if !defined(AFX_CONDPROPSDATA_H__813A6116_1480_40B1_A036_E1E3B7580683__INCLUDED_)
#define AFX_CONDPROPSDATA_H__813A6116_1480_40B1_A036_E1E3B7580683__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CondPropsData.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCondPropsData dialog

class CCondPropsData : public CPropertyPage
{
	DECLARE_DYNCREATE(CCondPropsData)

// Construction
public:
	NXC_OBJECT *m_pObject;
	CCondPropsData();
	~CCondPropsData();

// Dialog Data
	//{{AFX_DATA(CCondPropsData)
	enum { IDD = IDD_OBJECT_COND_DATA };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCondPropsData)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCondPropsData)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void AddListItem(int nPos, INPUT_DCI *pItem, TCHAR *pszName);
	INPUT_DCI *m_pDCIList;
	DWORD m_dwNumDCI;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONDPROPSDATA_H__813A6116_1480_40B1_A036_E1E3B7580683__INCLUDED_)
