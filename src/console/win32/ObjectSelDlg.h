#if !defined(AFX_OBJECTSELDLG_H__79C83A28_C41C_49C1_A2B3_8EC4388579A5__INCLUDED_)
#define AFX_OBJECTSELDLG_H__79C83A28_C41C_49C1_A2B3_8EC4388579A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectSelDlg.h : header file
//

//
// Allowed classes
//

#define SCL_NODE              0x0001
#define SCL_INTERFACE         0x0002
#define SCL_CONTAINER         0x0004
#define SCL_SUBNET            0x0008
#define SCL_NETWORK           0x0010
#define SCL_SERVICEROOT       0x0020
#define SCL_ZONE              0x0040
#define SCL_NETWORKSERVICE    0x0080
#define SCL_TEMPLATE          0x0100
#define SCL_TEMPLATEGROUP     0x0200
#define SCL_TEMPLATEROOT      0x0400
#define SCL_VPNCONNECTOR      0x0800


/////////////////////////////////////////////////////////////////////////////
// CObjectSelDlg dialog

class CObjectSelDlg : public CDialog
{
// Construction
public:
	BOOL m_bShowLoopback;
	BOOL m_bAllowEmptySelection;
	BOOL m_bSelectAddress;
	DWORD m_dwParentObject;
	BOOL m_bSingleSelection;
	DWORD m_dwAllowedClasses;
	DWORD m_dwNumObjects;
	DWORD *m_pdwObjectList;
	CObjectSelDlg(CWnd* pParent = NULL);   // standard constructor
   virtual ~CObjectSelDlg();

// Dialog Data
	//{{AFX_DATA(CObjectSelDlg)
	enum { IDD = IDD_SELECT_OBJECT };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjectSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkListObjects(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonNone();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CImageList m_imageList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTSELDLG_H__79C83A28_C41C_49C1_A2B3_8EC4388579A5__INCLUDED_)
