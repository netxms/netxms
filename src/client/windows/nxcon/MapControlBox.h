#if !defined(AFX_MAPCONTROLBOX_H__2D44F149_7F4A_4B2A_AF6B_2D36B6C3ED61__INCLUDED_)
#define AFX_MAPCONTROLBOX_H__2D44F149_7F4A_4B2A_AF6B_2D36B6C3ED61__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapControlBox.h : header file
//

#include "ToolBox.h"

/////////////////////////////////////////////////////////////////////////////
// CMapControlBox window

class CMapControlBox : public CToolBox
{
// Construction
public:
	CMapControlBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapControlBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMapControlBox();

	// Generated message map functions
protected:
	void AddCommand(TCHAR *pszText, int nImage, int nCommand);
	CFont m_font;
	CImageList m_imageList;
	CListCtrl m_wndListCtrl;
	//{{AFX_MSG(CMapControlBox)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
   afx_msg void OnListViewItemChanging(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPCONTROLBOX_H__2D44F149_7F4A_4B2A_AF6B_2D36B6C3ED61__INCLUDED_)
