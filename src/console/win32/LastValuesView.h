#if !defined(AFX_LASTVALUESVIEW_H__A36FB8F1_1BFA_4D01_8AA3_2A9D962B3140__INCLUDED_)
#define AFX_LASTVALUESVIEW_H__A36FB8F1_1BFA_4D01_8AA3_2A9D962B3140__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LastValuesView.h : header file
//
#include "ValueList.h"


/////////////////////////////////////////////////////////////////////////////
// CLastValuesView frame

class CLastValuesView : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CLastValuesView)
protected:
	CLastValuesView();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	CLastValuesView(DWORD dwNodeId);
	CLastValuesView(TCHAR *pszParams);

   DWORD GetObjectId(void) { return m_dwNodeId; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLastValuesView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateItem(int iItem, NXC_DCI_VALUE *pValue);
	CImageList m_imageList;
	CValueList m_wndListCtrl;
   //CListCtrl m_wndListCtrl;
	DWORD m_dwNodeId;
	virtual ~CLastValuesView();

	// Generated message map functions
	//{{AFX_MSG(CLastValuesView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
   afx_msg LRESULT OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LASTVALUESVIEW_H__A36FB8F1_1BFA_4D01_8AA3_2A9D962B3140__INCLUDED_)
