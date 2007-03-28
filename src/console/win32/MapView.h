#if !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
#define AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapView.h : header file
//

#include <netxms_maps.h>

#define MAX_ZOOM           3
#define NEUTRAL_SCALE      2
#define MAX_HISTORY_SIZE   256


//
// Map object states
//

#define MOS_SELECTED    0x01


//
// Scale-dependent elements
//

struct SCALE_INFO
{
   int nFactor;
   POINT ptObjectSize;
   POINT ptIconOffset;
   POINT ptTextOffset;
   POINT ptTextSize;
   int nImageList;
   int nFontIndex;
   TCHAR *pszText;
};


//
// Additional information about objects on map
//

struct OBJINFO
{
   DWORD dwObjectId;
   RECT rcObject;
   RECT rcText;
};


/////////////////////////////////////////////////////////////////////////////
// CMapView window

class CMapView : public CWnd
{
// Construction
public:
	CMapView();

// Attributes
public:
	COLORREF m_rgbSelBkColor;
	COLORREF m_rgbSelTextColor;
	COLORREF m_rgbSelRectColor;
	COLORREF m_rgbBkColor;
	COLORREF m_rgbTextColor;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
   const TCHAR *GetScaleText(void);
	void DoSubmapLayout(void);
	void OnObjectChange(DWORD dwObjectId, NXC_OBJECT *pObject);
	BOOL CanGoForward(void);
	BOOL CanGoBack(void);
	void OpenRoot(void);
	void GoForward(void);
	void GoBack(void);
	BOOL CanGoToParent(void);
	void GoToParentObject(void);
	BOOL CanZoomOut(void);
	void ZoomOut(void);
	BOOL CanZoomIn(void);
	void ZoomIn(void);
	void OpenSubmap(DWORD dwId, BOOL bAddToHistory = TRUE);
	void GetMinimalSelectionRect(RECT *pRect);
	int GetSelectionCount(void);
	void ClearSelection(BOOL  bRedraw = TRUE);
	DWORD PointInObject(POINT pt);
	void Update(void);
	void SetMap(nxMap *pMap);
	virtual ~CMapView();

	// Generated message map functions
protected:
	CPen m_penLinkTypes[2];
	HBITMAP GetBkImage(DWORD dwMapId, DWORD dwSubmapId, int nScaleFactor);
	HBITMAP m_hBkImage;
	void OnMouseButtonDown(UINT nFlags, POINT point);
	void OnMouseButtonUp(UINT nFlags, POINT point);
	void AddToHistory(DWORD dwId);
	DWORD m_dwHistory[MAX_HISTORY_SIZE];
	DWORD m_dwHistoryPos;
	DWORD m_dwHistorySize;
	POINT m_ptMapSize;
	int CalculateNewScrollPos(UINT nScrollBar, UINT nSBCode, UINT nPos);
   POINT m_ptOrg;
	void UpdateScrollBars(BOOL bForceUpdate);
	void ScalePosMapToScreen(POINT *pt);
	void ScalePosScreenToMap(POINT *pt);
	POINT m_ptDragOffset;
	void SelectObjectsInRect(RECT &rcSelection);
	void StartObjectDragging(POINT point);
	CImageList *m_pDragImageList;
	void MoveSelectedObjects(int nOffsetX, int nOffsetY);
	DWORD m_dwFocusedObjectIndex;
	DWORD m_dwFocusedObject;
	POINT m_ptMouseOpStart;
   RECT m_rcSelection;
	int m_nState;
	void SetObjectRect(DWORD dwObjectId, RECT *pRect, BOOL bTextRect);
   OBJINFO *m_pObjectInfo;
   DWORD m_dwNumObjects;
	CFont m_fontList[3];
	int m_nScale;
	void DrawObject(CDC &dc, DWORD dwIndex, CImageList *pImageList,
                   POINT ptOffset, BOOL bUpdateInfo);
	void DrawOnBitmap(CBitmap &bitmap, BOOL bSelectionOnly, RECT *prcSel);
	CBitmap m_bmpMap;
	nxSubmap *m_pSubmap;
	nxMap *m_pMap;
	//{{AFX_MSG(CMapView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL m_bShowConnectorNames;
	BOOL m_bCanOpenObjects;
	BOOL m_bIsModified;
   int GetScaleFactor(void);
	void SetBkImage(HBITMAP hBitmap);
	DWORD * GetSelectedObjects(DWORD *pdwNumObjects);
   nxMap *GetMap(void) { return m_pMap; }
   nxSubmap *GetSubmap(void) { return m_pSubmap; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
