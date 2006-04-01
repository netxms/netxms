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
   { 0, { 40, 40 }, { 3, 3 }, { -10, 42 }, { 60, 18 }, 0, 0 }   // Neutral scale (1:1)
};


/////////////////////////////////////////////////////////////////////////////
// CMapView

CMapView::CMapView()
{
   m_nState = STATE_NORMAL;
   m_nScale = NEUTRAL_SCALE;
   m_pMap = NULL;
   m_pSubmap = NULL;
   m_rgbBkColor = RGB(224, 224, 224);
   m_rgbTextColor = RGB(0, 0, 0);
   m_rgbSelBkColor = GetSysColor(COLOR_HIGHLIGHT);
   m_rgbSelTextColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
   m_rgbSelRectColor = RGB(0, 0, 128);
   m_pObjectInfo = NULL;
   m_dwNumObjects = 0;
   m_pDragImageList = NULL;
}

CMapView::~CMapView()
{
   safe_free(m_pObjectInfo);
   delete m_pDragImageList;
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

   // Create in-memory DC containig map image
   dc.CreateCompatibleDC(&sdc);
   pOldBitmap = dc.SelectObject(&m_bmpMap);

   // Draw map with selection rectangle
   if (m_nState == STATE_SELECTING)
   {
      CPen pen, *pOldPen;
      CBrush brush, *pOldBrush;
      CDC dcTemp, dcMap;
      CBitmap bitmap, bmpMap, *pOldTempBitmap, *pOldMapBitmap;
      int cx, cy;
      BLENDFUNCTION bf;

      // Create map copy
      dcMap.CreateCompatibleDC(&sdc);
      bmpMap.CreateCompatibleBitmap(&sdc, rect.right, rect.bottom);
      pOldMapBitmap = dcMap.SelectObject(&bmpMap);
      dcMap.BitBlt(0, 0, rect.right, rect.bottom, &dc, 0, 0, SRCCOPY);

      // Calculate selection width and height
      cx = m_rcSelection.right - m_rcSelection.left - 1;
      cy = m_rcSelection.bottom - m_rcSelection.top - 1;

      // Create temporary bitmap
      dcTemp.CreateCompatibleDC(&sdc);
      bitmap.CreateCompatibleBitmap(&sdc, cx, cy);
      pOldTempBitmap = dcTemp.SelectObject(&bitmap);

      // Fill temporary bitmap with selection color
      brush.DeleteObject();
      brush.CreateSolidBrush(m_rgbSelRectColor);
      pen.CreatePen(PS_SOLID, 1, m_rgbSelRectColor);
      pOldBrush = dcTemp.SelectObject(&brush);
      pOldPen = dcTemp.SelectObject(&pen);
      dcTemp.Rectangle(0, 0, cx + 1, cy + 1);
      dcTemp.SelectObject(pOldPen);
      dcTemp.SelectObject(pOldBrush);

      // Copy temporary bitmap to main bitmap with transparency
      bf.AlphaFormat = 0;
      bf.BlendFlags = 0;
      bf.BlendOp = AC_SRC_OVER;
      bf.SourceConstantAlpha = 16;
      AlphaBlend(dcMap.m_hDC, m_rcSelection.left, m_rcSelection.top, cx, cy,
                 dcTemp.m_hDC, 0, 0, cx, cy, bf);
      
      // Draw solid rectangle around selection area
      brush.DeleteObject();
      brush.CreateStockObject(NULL_BRUSH);
      pOldBrush = dcMap.SelectObject(&brush);
      pOldPen = dcMap.SelectObject(&pen);
      dcMap.Rectangle(&m_rcSelection);
      dcMap.SelectObject(pOldPen);
      dcMap.SelectObject(pOldBrush);

      dcTemp.SelectObject(pOldTempBitmap);
      dcTemp.DeleteDC();

      // Move drawing from in-memory bitmap to screen
      sdc.BitBlt(0, 0, rect.right, rect.bottom, &dcMap, 0, 0, SRCCOPY);

      dcMap.SelectObject(pOldMapBitmap);
      dcMap.DeleteDC();
   }
   else
   {
      // Move drawing from in-memory bitmap to screen
      sdc.BitBlt(0, 0, rect.right, rect.bottom, &dc, 0, 0, SRCCOPY);
   }

   // Cleanup
   dc.SelectObject(pOldBitmap);
   dc.DeleteDC();
}


//
// Draw entire map on bitmap in memory
//

void CMapView::DrawOnBitmap(CBitmap &bitmap, BOOL bSelectionOnly, RECT *prcSel)
{
   CDC *pdc, dc;           // Window dc and in-memory dc
   CBitmap *pOldBitmap;
   CBrush brush;
   RECT rect;
   DWORD i;
   CImageList *pImageList;
   CFont *pOldFont;
   POINT ptOffset;
   
   GetClientRect(&rect);

   // Create compatible DC and bitmap for painting
   pdc = GetDC();
   dc.CreateCompatibleDC(pdc);
   bitmap.DeleteObject();
   if (bSelectionOnly)
   {
      bitmap.CreateCompatibleBitmap(pdc, prcSel->right - prcSel->left,
                                    prcSel->bottom - prcSel->top);
      ptOffset.x = -prcSel->left;
      ptOffset.y = -prcSel->top;
   }
   else
   {
      bitmap.CreateCompatibleBitmap(pdc, rect.right, rect.bottom);
      ptOffset.x = 0;
      ptOffset.y = 0;
   }
   ReleaseDC(pdc);

   // Initial DC setup
   pOldBitmap = dc.SelectObject(&bitmap);
   dc.SetBkColor(m_rgbBkColor);
   brush.CreateSolidBrush(m_rgbBkColor);
   dc.FillRect(&rect, &brush);
   if (m_scaleInfo[m_nScale].nFontIndex != -1)
      pOldFont = dc.SelectObject(&m_fontList[m_scaleInfo[m_nScale].nFontIndex]);
   else
      pOldFont = NULL;

   if (m_pSubmap != NULL)
   {
      if (m_scaleInfo[m_nScale].nImageList == 0)
         pImageList = g_pObjectNormalImageList;
      else if (m_scaleInfo[m_nScale].nImageList == 1)
         pImageList = g_pObjectSmallImageList;
      else
         pImageList = NULL;

      if (bSelectionOnly)
      {
         for(i = 0; i < m_pSubmap->GetNumObjects(); i++)
            if (m_pSubmap->GetObjectStateFromIndex(i) & MOS_SELECTED)
               DrawObject(dc, i, pImageList, ptOffset, FALSE);
      }
      else
      {
         for(i = 0; i < m_pSubmap->GetNumObjects(); i++)
            DrawObject(dc, i, pImageList, ptOffset, TRUE);
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
   dc.DeleteDC();
}


//
// Draw single object on map
//

void CMapView::DrawObject(CDC &dc, DWORD dwIndex, CImageList *pImageList,
                          POINT ptOffset, BOOL bUpdateInfo)
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

      // Apply offset
      pt.x += ptOffset.x;
      pt.y += ptOffset.y;

      // Draw background and icon
      rect.left = pt.x;
      rect.top = pt.y;
      rect.right = rect.left + m_scaleInfo[m_nScale].ptObjectSize.x;
      rect.bottom = rect.top + m_scaleInfo[m_nScale].ptObjectSize.y;
      if (bUpdateInfo)
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
         dc.SetBkColor((dwState & MOS_SELECTED) ? m_rgbSelBkColor : m_rgbBkColor);
         dc.SetTextColor((dwState & MOS_SELECTED) ? m_rgbSelTextColor : m_rgbTextColor);
         dc.DrawText(pObject->szName, -1, &rect,
                     DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK | DT_CENTER);
         if (bUpdateInfo)
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
   DrawOnBitmap(m_bmpMap, FALSE, NULL);
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
   m_ptMouseOpStart = point;
   m_dwFocusedObject = PointInObject(point);
   if (m_dwFocusedObject != 0)
   {
      m_dwFocusedObjectIndex = m_pSubmap->GetObjectIndex(m_dwFocusedObject);
      if (!(m_pSubmap->GetObjectStateFromIndex(m_dwFocusedObjectIndex) & MOS_SELECTED))
      {
         ClearSelection(FALSE);
         m_pSubmap->SetObjectStateByIndex(m_dwFocusedObjectIndex, MOS_SELECTED);
         Update();
      }
      m_nState = STATE_OBJECT_LCLICK;
   }
   else
   {
      ClearSelection();
      m_rcSelection.left = m_rcSelection.right = point.x;
      m_rcSelection.top = m_rcSelection.bottom = point.y;
      m_nState = STATE_SELECTING;
   }
   SetCapture();
}


//
// WM_LBUTTONUP message handler
//

void CMapView::OnLButtonUp(UINT nFlags, CPoint point) 
{
   ReleaseCapture();
   switch(m_nState)
   {
      case STATE_OBJECT_LCLICK:
         ClearSelection(FALSE);
         m_pSubmap->SetObjectStateByIndex(m_dwFocusedObjectIndex, MOS_SELECTED);
         Update();
         break;
      case STATE_DRAGGING:
         m_pDragImageList->DragLeave(this);
         m_pDragImageList->EndDrag();
         delete_and_null(m_pDragImageList);
         MoveSelectedObjects(point.x - m_ptMouseOpStart.x, point.y - m_ptMouseOpStart.y);
         break;
      case STATE_SELECTING:
         ClearSelection(FALSE);
         SelectObjectsInRect(m_rcSelection);
         Update();
         break;
      default:
         break;
   }
   m_nState = STATE_NORMAL;
}


//
// WM_MOUSEMOVE message handler
//

void CMapView::OnMouseMove(UINT nFlags, CPoint point) 
{
   RECT rcOldSel, rcUpdate, rcClient;

   switch(m_nState)
   {
      case STATE_OBJECT_LCLICK:
         m_nState = STATE_DRAGGING;
         StartObjectDragging(point);
         //break;
      case STATE_DRAGGING:
         m_pDragImageList->DragMove(point);
         break;
      case STATE_SELECTING:
         CopyRect(&rcOldSel, &m_rcSelection);

         // Calculate normalized selection rectangle
         if (point.x < m_ptMouseOpStart.x)
         {
            m_rcSelection.left = point.x;
            m_rcSelection.right = m_ptMouseOpStart.x;
         }
         else
         {
            m_rcSelection.left = m_ptMouseOpStart.x;
            m_rcSelection.right = point.x;
         }
         if (point.y < m_ptMouseOpStart.y)
         {
            m_rcSelection.top = point.y;
            m_rcSelection.bottom = m_ptMouseOpStart.y;
         }
         else
         {
            m_rcSelection.top = m_ptMouseOpStart.y;
            m_rcSelection.bottom = point.y;
         }

         // Validate selection rectangle
         GetClientRect(&rcClient);
         if (m_rcSelection.left < 0)
            m_rcSelection.left = 0;
         if (m_rcSelection.top < 0)
            m_rcSelection.top = 0;
         if (m_rcSelection.right > rcClient.right)
            m_rcSelection.right = rcClient.right;
         if (m_rcSelection.bottom > rcClient.bottom)
            m_rcSelection.bottom = rcClient.bottom;
         
         // Update changed region
         UnionRect(&rcUpdate, &rcOldSel, &m_rcSelection);
         InvalidateRect(&rcUpdate, FALSE);
         break;
      default:
         break;
   }
}


//
// Clear current object(s) selection
//

void CMapView::ClearSelection(BOOL bRedraw)
{
   DWORD i;

   for(i = m_pSubmap->GetNumObjects(); i > 0; i--)
      m_pSubmap->SetObjectStateByIndex(i - 1, m_pSubmap->GetObjectStateFromIndex(i - 1) & ~MOS_SELECTED);
   if (bRedraw)
      Update();
}


//
// Get number of currently selected objects
//

int CMapView::GetSelectionCount()
{
   int nCount;
   DWORD i;

   for(i = 0, nCount = 0; i < m_pSubmap->GetNumObjects(); i++)
      if (m_pSubmap->GetObjectStateFromIndex(i) & MOS_SELECTED)
         nCount++;
   return nCount;
}


//
// Move selected objects by given offset
//

void CMapView::MoveSelectedObjects(int nOffsetX, int nOffsetY)
{
   DWORD i;
   POINT pt;

   for(i = 0; i < m_pSubmap->GetNumObjects(); i++)
      if (m_pSubmap->GetObjectStateFromIndex(i) & MOS_SELECTED)
      {
         pt = m_pSubmap->GetObjectPositionByIndex(i);
         m_pSubmap->SetObjectPositionByIndex(i, pt.x + nOffsetX, pt.y + nOffsetY);
      }
   Update();
}


//
// Get minimal rectangle which covers all selected objects
//

void CMapView::GetMinimalSelectionRect(RECT *pRect)
{
   DWORD i, j, dwId;
   RECT rect, rcPrev;

   SetRectEmpty(pRect);
   for(i = 0; i < m_pSubmap->GetNumObjects(); i++)
      if (m_pSubmap->GetObjectStateFromIndex(i) & MOS_SELECTED)
      {
         dwId = m_pSubmap->GetObjectIdFromIndex(i);
         for(j = 0; j < m_dwNumObjects; j++)
         {
            if (m_pObjectInfo[j].dwObjectId == dwId)
            {
               UnionRect(&rect, &m_pObjectInfo[j].rcObject, &m_pObjectInfo[j].rcText);
               CopyRect(&rcPrev, pRect);
               UnionRect(pRect, &rcPrev, &rect);
               break;
            }
         }
      }
}


//
// Create image list for object dragging
//

void CMapView::StartObjectDragging(POINT point)
{
   RECT rcSel;
   CBitmap bitmap;

   GetMinimalSelectionRect(&rcSel);
   DrawOnBitmap(bitmap, TRUE, &rcSel);
   m_pDragImageList = new CImageList;
   m_pDragImageList->Create(rcSel.right - rcSel.left, rcSel.bottom - rcSel.top,
                            ILC_COLOR32 | ILC_MASK, 1, 1);
   m_pDragImageList->Add(&bitmap, m_rgbBkColor);

   m_pDragImageList->BeginDrag(0, CPoint(point.x - rcSel.left, point.y - rcSel.top));
   m_pDragImageList->DragEnter(this, point);
}


//
// Select all objects within given rectangle
//

void CMapView::SelectObjectsInRect(RECT &rcSelection)
{
   DWORD i;
   RECT rect;
   
   for(i = 0; i < m_dwNumObjects; i++)
   {
      if (IntersectRect(&rect, &m_pObjectInfo[i].rcObject, &rcSelection))
      {
         m_pSubmap->SetObjectState(m_pObjectInfo[i].dwObjectId, MOS_SELECTED);
      }
   }
}
