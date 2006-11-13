#if !defined(AFX_DISCOVERYPROPADDRLIST_H__CF47913D_FC51_477F_9FC6_FEC0A8FAF568__INCLUDED_)
#define AFX_DISCOVERYPROPADDRLIST_H__CF47913D_FC51_477F_9FC6_FEC0A8FAF568__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiscoveryPropAddrList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropAddrList dialog

class CDiscoveryPropAddrList : public CPropertyPage
{
	DECLARE_DYNCREATE(CDiscoveryPropAddrList)

// Construction
public:
	CDiscoveryPropAddrList();
	~CDiscoveryPropAddrList();

// Dialog Data
	//{{AFX_DATA(CDiscoveryPropAddrList)
	enum { IDD = IDD_DISCOVERY_RANGES };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDiscoveryPropAddrList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDiscoveryPropAddrList)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISCOVERYPROPADDRLIST_H__CF47913D_FC51_477F_9FC6_FEC0A8FAF568__INCLUDED_)
