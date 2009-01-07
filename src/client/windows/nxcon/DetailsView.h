#if !defined(AFX_DETAILSVIEW_H__360CE5D7_5598_4282_9789_8D5BCDB2A06E__INCLUDED_)
#define AFX_DETAILSVIEW_H__360CE5D7_5598_4282_9789_8D5BCDB2A06E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DetailsView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDetailsView frame

class CDetailsView : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CDetailsView)
protected:
	CDetailsView();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	CDetailsView(DWORD dwId, int cx, int cy, Table *pData, CImageList *pImageList);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDetailsView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Update(void);
   int m_cx;
   int m_cy;
   Table *m_pData;
   DWORD m_dwViewId;
   CImageList *m_pImageList;
	CListCtrl m_wndListCtrl;
	virtual ~CDetailsView();

	// Generated message map functions
	//{{AFX_MSG(CDetailsView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DETAILSVIEW_H__360CE5D7_5598_4282_9789_8D5BCDB2A06E__INCLUDED_)
