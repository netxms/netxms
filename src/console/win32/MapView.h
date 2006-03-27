#if !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
#define AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapView.h : header file
//

#include <netxms_maps.h>

#define NEUTRAL_SCALE   2


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
	void Update(void);
	void SetMap(nxMap *pMap);
	virtual ~CMapView();

	// Generated message map functions
protected:
	CFont m_fontList[2];
	int m_nScale;
	void DoSubmapLayout(void);
	COLORREF m_rgbBkColor;
	void DrawObject(CDC &dc, DWORD dwObjectId, CImageList *pImageList);
	void DrawOnBitmap(void);
	CBitmap m_bmpMap;
	nxSubmap *m_pSubmap;
	nxMap *m_pMap;
	//{{AFX_MSG(CMapView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
