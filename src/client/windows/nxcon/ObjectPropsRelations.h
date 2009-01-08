#if !defined(AFX_OBJECTPROPSRELATIONS_H__035844B5_8692_4905_816F_3970493B9FCB__INCLUDED_)
#define AFX_OBJECTPROPSRELATIONS_H__035844B5_8692_4905_816F_3970493B9FCB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropsRelations.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsRelations dialog

class CObjectPropsRelations : public CPropertyPage
{
	DECLARE_DYNCREATE(CObjectPropsRelations)

// Construction
public:
	NXC_OBJECT *m_pObject;
	CObjectPropsRelations();
	~CObjectPropsRelations();

// Dialog Data
	//{{AFX_DATA(CObjectPropsRelations)
	enum { IDD = IDD_OBJECT_RELATIONS };
	CListCtrl	m_wndListParents;
	CListCtrl	m_wndListChilds;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropsRelations)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList *m_pImageList;
	// Generated message map functions
	//{{AFX_MSG(CObjectPropsRelations)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPSRELATIONS_H__035844B5_8692_4905_816F_3970493B9FCB__INCLUDED_)
