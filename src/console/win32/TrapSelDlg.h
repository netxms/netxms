#if !defined(AFX_TRAPSELDLG_H__F35F729F_D569_4553_B08B_72B61985D61D__INCLUDED_)
#define AFX_TRAPSELDLG_H__F35F729F_D569_4553_B08B_72B61985D61D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrapSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrapSelDlg dialog

class CTrapSelDlg : public CDialog
{
// Construction
public:
	DWORD * m_pdwTrapList;
	DWORD m_dwNumTraps;
	NXC_TRAP_CFG_ENTRY * m_pTrapCfg;
	DWORD m_dwTrapCfgSize;
	CTrapSelDlg(CWnd* pParent = NULL);   // standard constructor
	~CTrapSelDlg();

// Dialog Data
	//{{AFX_DATA(CTrapSelDlg)
	enum { IDD = IDD_SELECT_TRAP };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrapSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int CompareItems(LPARAM nItem1, LPARAM nItem2);
	void SortList(void);
	int m_iSortDir;
	int m_iSortMode;
	int m_iLastEventImage;
	CImageList * m_pImageList;

	// Generated message map functions
	//{{AFX_MSG(CTrapSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAPSELDLG_H__F35F729F_D569_4553_B08B_72B61985D61D__INCLUDED_)
