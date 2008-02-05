#if !defined(AFX_DISCOVERYPROPCOMMUNITIES_H__038E8674_007F_4086_8236_59AB775A436C__INCLUDED_)
#define AFX_DISCOVERYPROPCOMMUNITIES_H__038E8674_007F_4086_8236_59AB775A436C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiscoveryPropCommunities.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropCommunities dialog

class CDiscoveryPropCommunities : public CPropertyPage
{
	DECLARE_DYNCREATE(CDiscoveryPropCommunities)

// Construction
public:
	TCHAR **m_ppStringList;
	DWORD m_dwNumStrings;
	CDiscoveryPropCommunities();
	~CDiscoveryPropCommunities();

// Dialog Data
	//{{AFX_DATA(CDiscoveryPropCommunities)
	enum { IDD = IDD_DISCOVERY_COMMUNITIES };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDiscoveryPropCommunities)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDiscoveryPropCommunities)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISCOVERYPROPCOMMUNITIES_H__038E8674_007F_4086_8236_59AB775A436C__INCLUDED_)
