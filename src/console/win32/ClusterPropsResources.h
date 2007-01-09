#if !defined(AFX_CLUSTERPROPSRESOURCES_H__FAC7A63F_E6D4_4F31_8A9D_360F04FCFCAF__INCLUDED_)
#define AFX_CLUSTERPROPSRESOURCES_H__FAC7A63F_E6D4_4F31_8A9D_360F04FCFCAF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClusterPropsResources.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClusterPropsResources dialog

class CClusterPropsResources : public CPropertyPage
{
	DECLARE_DYNCREATE(CClusterPropsResources)

// Construction
public:
	NXC_OBJECT * m_pObject;
	CClusterPropsResources();
	~CClusterPropsResources();

// Dialog Data
	//{{AFX_DATA(CClusterPropsResources)
	enum { IDD = IDD_OBJECT_CLUSTER_RESOURCES };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CClusterPropsResources)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	DWORD m_dwNumResources;
	CLUSTER_RESOURCE * m_pResourceList;
	// Generated message map functions
	//{{AFX_MSG(CClusterPropsResources)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLUSTERPROPSRESOURCES_H__FAC7A63F_E6D4_4F31_8A9D_360F04FCFCAF__INCLUDED_)
