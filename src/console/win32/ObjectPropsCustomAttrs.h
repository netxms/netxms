#if !defined(AFX_OBJECTPROPSCUSTOMATTRS_H__1FCD36D6_FB2D_4664_A351_055FC2B17D19__INCLUDED_)
#define AFX_OBJECTPROPSCUSTOMATTRS_H__1FCD36D6_FB2D_4664_A351_055FC2B17D19__INCLUDED_

#include "..\..\..\INCLUDE\nms_util.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsCustomAttrs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsCustomAttrs dialog

class CObjectPropsCustomAttrs : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsCustomAttrs)

// Construction
public:
	NXC_OBJECT * m_pObject;
	CObjectPropsCustomAttrs();
	~CObjectPropsCustomAttrs();

// Dialog Data
	//{{AFX_DATA(CObjectPropsCustomAttrs)
	enum { IDD = IDD_OBJECT_CUSTOM_ATTRS };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsCustomAttrs)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	StringMap m_strMap;
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsCustomAttrs)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonDelete();
	afx_msg void OnItemchangedListAttributes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListAttributes(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	NXC_OBJECT_UPDATE * m_pUpdate;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSCUSTOMATTRS_H__1FCD36D6_FB2D_4664_A351_055FC2B17D19__INCLUDED_)
