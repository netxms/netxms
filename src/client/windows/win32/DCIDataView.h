#if !defined(AFX_DCIDATAVIEW_H__4C89D444_02D7_4D97_ADAC_F09D5564766F__INCLUDED_)
#define AFX_DCIDATAVIEW_H__4C89D444_02D7_4D97_ADAC_F09D5564766F__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCIDataView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCIDataView frame

class CDCIDataView : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CDCIDataView)
protected:
	CDCIDataView();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
   CDCIDataView(DWORD dwNodeId, DWORD dwItemId, TCHAR *pszItemName);
   CDCIDataView(TCHAR *pszParams);
	virtual ~CDCIDataView();

   TCHAR *GetItemName(void) { return m_szItemName; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDCIDataView)
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SetScale(int nScale);
	void DisplayScale(void);
	int m_nScale;
	TCHAR m_szItemName[MAX_OBJECT_NAME + MAX_ITEM_NAME + 4];
	CStatusBarCtrl m_wndStatusBar;
	// Generated message map functions
	//{{AFX_MSG(CDCIDataView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDataScaleGbytes();
	afx_msg void OnDataScaleGiga();
	afx_msg void OnDataScaleKbytes();
	afx_msg void OnDataScaleKilo();
	afx_msg void OnDataScaleMbytes();
	afx_msg void OnDataScaleMega();
	afx_msg void OnDataScaleNormal();
	afx_msg void OnDataCopytoclipboard();
	//}}AFX_MSG
   afx_msg void OnListViewItemChange(LPNMLISTVIEW pNMHDR, LRESULT *pResult);
   afx_msg LRESULT OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo);
	DECLARE_MESSAGE_MAP()
private:
	int m_iStatusBarHeight;
	CListCtrl m_wndListCtrl;
	DWORD m_dwItemId;
	DWORD m_dwNodeId;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCIDATAVIEW_H__4C89D444_02D7_4D97_ADAC_F09D5564766F__INCLUDED_)
