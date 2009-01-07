#if !defined(AFX_EVENTSELDLG_H__5D11D29D_CE20_4CFD_8159_679FFF5F060C__INCLUDED_)
#define AFX_EVENTSELDLG_H__5D11D29D_CE20_4CFD_8159_679FFF5F060C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EventSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventSelDlg dialog

class CEventSelDlg : public CDialog
{
// Construction
public:
	BOOL m_bSingleSelection;
	DWORD *m_pdwEventList;
	DWORD m_dwNumEvents;
	CEventSelDlg(CWnd* pParent = NULL);   // standard constructor
   virtual ~CEventSelDlg();

// Dialog Data
	//{{AFX_DATA(CEventSelDlg)
	enum { IDD = IDD_SELECT_EVENT };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SortList(void);
	CImageList *m_pImageList;

	// Generated message map functions
	//{{AFX_MSG(CEventSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkListEvents(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickListEvents(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	int m_iLastEventImage;
	int m_iSortDir;
	int m_iSortMode;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTSELDLG_H__5D11D29D_CE20_4CFD_8159_679FFF5F060C__INCLUDED_)
