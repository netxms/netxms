// MapView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Static data
//

static SCALE_INFO m_scaleInfo[] =
{
   { -4, { 10, 10 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, -1, -1 },
   { -2, { 20, 20 }, { 2, 2 }, { -2, 21 }, { 24, 9 }, 1, 1 },
   { 0, { 40, 40 }, { 3, 3 }, { -10, 42 }, { 60, 18 }, 0, 0 }   // Neutral scale
};


/////////////////////////////////////////////////////////////////////////////
// CMapView

CMapView::CMapView()
{
   m_nScale = NEUTRAL_SCALE;
   m_pMap = NULL;
   m_pSubmap = NULL;
   m_rgbBkColor = RGB(224, 224, 224);
   m_pObjectInfo = NULL;
   m_dwNumObjects = 0;
}

CMapView::~CMapView()
{
   safe_free(m_pObjectInfo);
}


BEGIN_MESSAGE_MAP(CMapView, CWnd)
	//{{AFX_MSG_MAP(CMapView)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMapView message handlers

BOOL CMapView::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.style |= WS_HSCROLL | WS_VSCROLL;
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CMapView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create fonts
   m_fontList[0].CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");
   m_fontList[1].CreateFont(-MulDiv(5, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");
	
   // Disable scroll bars
   SetScrollRange(SB_HORZ, 0, 0);
   SetScrollRange(SB_VERT, 0, 0);

   return 0;
}


//
// WM_PAINT message handler
//

void CMapView::OnPaint() 
{
	CPaintDC sdc(this);  // original device context for painting
   CDC dc;              // In-memory dc
   CBitmap *pOldBitmap;
   RECT rect;

   GetClientRect(&rect);
   
   // Move drawing from in-memory bitmap to screen
   dc.CreateCompatibleDC(&sdc);
   pOldBitmap = dc.SelectObject(&m_bmpMap);
   sdc.BitBlt(0, 0, rect.right, rect.bottom, &dc, 0, 0, SRCCOPY);

   // Cleanup
   dc.SelectObject(pOldBitmap);
   dc.DeleteDC();
}


//
// Draw entire map on bitmap in memory
//

void CMapView::DrawOnBitmap()
{
   CDC *pdc, dc;           // Window dc and in-memory dc
   CBitmap *pOldBitmap;
   CBrush brush;
   NXC_OBJECT *pObject;
   RECT rect;
   DWORD i;
   CImageList *pImageList;
   CFont *pOldFont;
   
   GetClientRect(&rect);

   // Create compatible DC and bitmap for painting
   pdc = GetDC();
   dc.CreateCompatibleDC(pdc);
   m_bmpMap.DeleteObject();
   m_bmpMap.CreateCompatibleBitmap(pdc, rect.right, rect.bottom);
   ReleaseDC(pdc);

   // Initial DC setup
   pOldBitmap = dc.SelectObject(&m_bmpMap);
   dc.SetBkColor(m_rgbBkColor);
   brush.CreateSolidBrush(m_rgbBkColor);
   dc.FillRect(&rect, &brush);
   if (m_scaleInfo[m_nScale].nFontIndex != -1)
      pOldFont = dc.SelectObject(&m_fontList[m_scaleInfo[m_nScale].nFontIndex]);
   else
      pOldFont = NULL;

   if (m_pSubmap != NULL)
   {
      // Get current object
      pObject = NXCFindObjectById(g_hSession, m_pSubmap->Id());
      if (pObject != NULL)
      {
         if (m_scaleInfo[m_nScale].nImageList == 0)
            pImageList = g_pObjectNormalImageList;
         else if (m_scaleInfo[m_nScale].nImageList == 1)
            pImageList = g_pObjectSmallImageList;
         else
            pImageList = NULL;

         for(i = 0; i < m_pSubmap->GetNumObjects(); i++)
            DrawObject(dc, i, pImageList);
      }
      else
      {
         dc.TextOut(0, 0, _T("NULL object"), 11);
      }
   }
   else
   {
      dc.TextOut(0, 0, _T("NULL submap"), 11);
   }

   // Cleanup
   if (pOldFont != NULL)
      dc.SelectObject(pOldFont);
   dc.SelectObject(pOldBitmap);
   //font.DeleteObject();
   dc.DeleteDC();
}


//
// Draw single object on map
//

void CMapView::DrawObject(CDC &dc, DWORD dwIndex, CImageList *pImageList)
{
   NXC_OBJECT *pObject;
   POINT pt, ptIcon;
   RECT rect;
   DWORD dwState;

   pObject = NXCFindObjectById(g_hSession, m_pSubmap->GetObjectIdFromIndex(dwIndex));
   if (pObject != NULL)
   {
      dwState = m_pSubmap->GetObjectStateFromIndex(dwIndex);

      // Get and scale object's position
      pt = m_pSubmap->GetObjectPositionByIndex(dwIndex);
      if (m_scaleInfo[m_nScale].nFactor > 0)
      {
         pt.x *= m_scaleInfo[m_nScale].nFactor;
         pt.y *= m_scaleInfo[m_nScale].nFactor;
      }
      else if (m_scaleInfo[m_nScale].nFactor < 0)
      {
         pt.x /= -m_scaleInfo[m_nScale].nFactor;
         pt.y /= -m_scaleInfo[m_nScale].nFactor;
      }

      // Draw background and icon
      rect.left = pt.x;
      rect.top = pt.y;
      rect.right = rect.left + m_scaleInfo[m_nScale].ptObjectSize.x;
      rect.bottom = rect.top + m_scaleInfo[m_nScale].ptObjectSize.y;
      SetObjectRect(pObject->dwId, &rect, FALSE);
      dc.FillSolidRect(pt.x, pt.y, m_scaleInfo[m_nScale].ptObjectSize.x, 
                       m_scaleInfo[m_nScale].ptObjectSize.y,
                       g_statusColorTable[pObject->iStatus]);
      ptIcon.x = pt.x + m_scaleInfo[m_nScale].ptIconOffset.x;
      ptIcon.y = pt.y + m_scaleInfo[m_nScale].ptIconOffset.y;
      if (pImageList != NULL)
      {
         pImageList->Draw(&dc, GetObjectImageIndex(pObject), ptIcon,
                          (dwState & MOS_SELECTED) ? ILD_BLEND50 : ILD_TRANSPARENT);
      }

      // Draw object name
      if (m_scaleInfo[m_nScale].nFontIndex != -1)
      {
         rect.left = pt.x + m_scaleInfo[m_nScale].ptTextOffset.x;
         rect.top = pt.y + m_scaleInfo[m_nScale].ptTextOffset.y;
         rect.right = rect.left + m_scaleInfo[m_nScale].ptTextSize.x;
         rect.bottom = rect.top + m_scaleInfo[m_nScale].ptTextSize.y;
         dc.SetBkColor(m_rgbBkColor);
         dc.DrawText(pObject->szName, -1, &rect,
                     DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK | DT_CENTER);
         SetObjectRect(pObject->dwId, &rect, TRUE);
      }
   }
}


//
// Set new map
//

void CMapView::SetMap(nxMap *pMap)
{
   delete m_pMap;
   m_pMap = pMap;
   m_pSubmap = m_pMap->GetSubmap(m_pMap->ObjectId());
   if (!m_pSubmap->IsLayoutCompleted())
      DoSubmapLayout();
   Update();
}


//
// Repaint entire map
//

void CMapView::Update()
{
   DrawOnBitmap();
   Invalidate(FALSE);
}


//
// (Re)do current submap layout
//

void CMapView::DoSubmapLayout()
{
   NXC_OBJECT *pObject;
   RECT rect;

   pObject = NXCFindObjectById(g_hSession, m_pSubmap->Id());
   if (pObject != NULL)
   {
      GetClientRect(&rect);
      m_pSubmap->DoLayout(pObject->dwNumChilds, pObject->pdwChildList,
                          0, NULL, rect.right, rect.bottom);
   }
   else
   {
      MessageBox(_T("Internal error: cannot find root object for the current submap"),
                 _T("Error"), MB_OK | MB_ICONSTOP);
   }
}


//
// Save information about rectangle occupied by object
//

void CMapView::SetObjectRect(DWORD dwObjectId, RECT *pRect, BOOL bTextRect)
{
   DWORD i;

   for(i = 0; i < m_dwNumObjects; i++)
      if (m_pObjectInfo[i].dwObjectId == dwObjectId)
      {
         memcpy(bTextRect ? &m_pObjectInfo[i].rcText : &m_pObjectInfo[i].rcObject,
                pRect, sizeof(RECT));
         break;
      }
   if (i == m_dwNumObjects)
   {
      m_dwNumObjects++;
      m_pObjectInfo = (OBJINFO *)realloc(m_pObjectInfo, sizeof(OBJINFO) * m_dwNumObjects);
      memset(&m_pObjectInfo[i], 0, sizeof(OBJINFO));
      m_pObjectInfo[i].dwObjectId = dwObjectId;
      memcpy(bTextRect ? &m_pObjectInfo[i].rcText : &m_pObjectInfo[i].rcObject,
             pRect, sizeof(RECT));
   }
}


//
// Check if given point is withing object
// Will return object id on success or 0 otherwise
//

DWORD CMapView::PointInObject(POINT pt)
{
   DWORD i, j, dwId;

   for(i = m_pSubmap->GetNumObjects(); i > 0; i--)
   {
      dwId = m_pSubmap->GetObjectIdFromIndex(i - 1);
      for(j = 0; j < m_dwNumObjects; j++)
      {
         if (m_pObjectInfo[j].dwObjectId == dwId)
         {
            if (PtInRect(&m_pObjectInfo[j].rcObject, pt) ||
                PtInRect(&m_pObjectInfo[j].rcText, pt))
            {
               return dwId;
            }
         }
      }
   }
   return 0;
}


//
// WM_LBUTTONDOWN message handler
//

void CMapView::OnLButtonDown(UINT nFlags, CPoint point) 
{
   DWORD dwObjectId;

   dwObjectId = PointInObject(point);
   if (dwObjectId != 0)
   {
      m_pSubmap->SetObjectState(dwObjectId, MOS_SELECTED);
      Update();
   }
}


//
// WM_LBUTTONUP message handler
//

void CMapView::OnLButtonUp(UINT nFlags, CPoint point) 
{
}


//
// WM_MOUSEMOVE message handler
//

void CMapView::OnMouseMove(UINT nFlags, CPoint point) 
{
}
