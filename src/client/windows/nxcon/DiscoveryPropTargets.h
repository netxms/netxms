#if !defined(AFX_DISCOVERYPROPTARGETS_H__6EB9558D_165F_4C26_A840_400670DCBE33__INCLUDED_)
#define AFX_DISCOVERYPROPTARGETS_H__6EB9558D_165F_4C26_A840_400670DCBE33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DiscoveryPropTargets.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropTargets dialog

class CDiscoveryPropTargets : public CPropertyPage
{
	DECLARE_DYNCREATE(CDiscoveryPropTargets)

// Construction
public:
	NXC_ADDR_ENTRY * m_pAddrList;
	DWORD m_dwAddrCount;
	CDiscoveryPropTargets();
	~CDiscoveryPropTargets();

// Dialog Data
	//{{AFX_DATA(CDiscoveryPropTargets)
	enum { IDD = IDD_DISCOVERY_TARGETS };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDiscoveryPropTargets)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void AddRecordToList(int nItem, NXC_ADDR_ENTRY *pAddr);
   // Generated message map functions
	//{{AFX_MSG(CDiscoveryPropTargets)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISCOVERYPROPTARGETS_H__6EB9558D_165F_4C26_A840_400670DCBE33__INCLUDED_)
