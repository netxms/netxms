#if !defined(AFX_MAPFRAME_H__8A4682BA_0A30_4BE2_BDBE_16ED918E0D46__INCLUDED_)
#define AFX_MAPFRAME_H__8A4682BA_0A30_4BE2_BDBE_16ED918E0D46__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapFrame.h : header file
//

#include "MapView.h"
#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView

#define OBJECT_HISTORY_SIZE      512


/////////////////////////////////////////////////////////////////////////////
// CMapFrame frame

class CMapFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CMapFrame)
protected:
	CMapFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapFrame)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iStatusImageBase;
	DWORD m_dwHistoryPos;
	NXC_OBJECT *m_pObjectHistory[OBJECT_HISTORY_SIZE];
	NXC_OBJECT *GetSelectedObject(void);
	void AddObjectToView(NXC_OBJECT *pObject);
	CImageList *m_pImageList;
	NXC_OBJECT *m_pRootObject;
	CListCtrl m_wndMapView;
	virtual ~CMapFrame();

	// Generated message map functions
	//{{AFX_MSG(CMapFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnObjectOpen();
	afx_msg void OnObjectOpenparent();
	//}}AFX_MSG
   afx_msg void OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPFRAME_H__8A4682BA_0A30_4BE2_BDBE_16ED918E0D46__INCLUDED_)
