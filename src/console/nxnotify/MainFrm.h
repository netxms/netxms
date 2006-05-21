// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__A80DF5B7_B4C9_4F90_A840_29745629B6AA__INCLUDED_)
#define AFX_MAINFRM_H__A80DF5B7_B4C9_4F90_A840_29745629B6AA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	BOOL m_bExit;
	CImageList m_imageList;
	int m_iSortDir;
	int m_iSortMode;
	int m_iSortImageBase;
	CListCtrl m_wndListCtrl;

   void AddAlarm(NXC_ALARM *pAlarm);
   int FindAlarmRecord(DWORD dwAlarmId);

	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTaskbarOpen();
	afx_msg void OnClose();
	afx_msg void OnFileExit();
	//}}AFX_MSG
   afx_msg void OnTaskbarCallback(WPARAM wParam, LPARAM lParam);
   afx_msg void OnAlarmUpdate(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__A80DF5B7_B4C9_4F90_A840_29745629B6AA__INCLUDED_)
