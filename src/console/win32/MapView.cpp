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

#define SCROLL_STEP     40


//
// Map view states
//

#define STATE_NORMAL          0
#define STATE_OBJECT_LCLICK   1
#define STATE_DRAGGING        2
#define STATE_SELECTING       3


//
// Static data
//

static SCALE_INFO m_scaleInfo[] =
{
   { -4, { 10, 10 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, -1, -1, _T("1:4") },
   { -2, { 20, 20 }, { 2, 2 }, { -2, 21 }, { 24, 12 }, 1, 1, _T("1:2") },
   { 0, { 40, 40 }, { 3, 3 }, { -15, 42 }, { 70, 24 }, 0, 0, _T("1:1") },   // Neutral scale (1:1)
   { 2, { 80, 80 }, { 6, 6 }, { -30, 84 }, { 140, 48 }, 0, 2, _T("2:1") }
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
	m_rgbLabelTextColor = RGB(0, 0, 0);
	m_rgbLabelBkColor = RGB(128, 255, 128);
   m_rgbSelBkColor = GetSysColor(COLOR_HIGHLIGHT);
   m_rgbSelTextColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
   m_rgbSelRectColor = RGB(0, 0, 128);
   m_pObjectInfo = NULL;
   m_dwNumObjects = 0;
   m_pDragImageList = NULL;
   m_ptOrg.x = 0;
   m_ptOrg.y = 0;
   m_dwHistoryPos = 0;
   m_dwHistorySize = 0;
   m_hBkImage = NULL;
   m_bIsModified = FALSE;
	m_bCanOpenObjects = TRUE;
	m_bShowConnectorNames = FALSE;
}

CMapView::~CMapView()
{
   safe_free(m_pObjectInfo);
   delete m_pDragImageList;
   if (m_hBkImage != NULL)
      DeleteObject(m_hBkImage);
	delete m_pMap;
}


BEGIN_MESSAGE_MAP(CMapView, CWnd)
	//{{AFX_MSG_MAP(CMapView)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


const TCHAR *CMapView::GetScaleText(void)
{
   return m_scaleInfo[m_nScale].pszText;
}


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
                            VARIABLE_PITCH | FF_DONTCARE, _T("Helvetica"));
   m_fontList[1].CreateFont(-MulDiv(5, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_DONTCARE, _T("Helvetica"));
   m_fontList[2].CreateFont(-MulDiv(11, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_DONTCARE, _T("Helvetica"));

   // Create pens for different link types
   m_penLinkTypes[0].CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
   m_penLinkTypes[1].CreatePen(PS_DOT, 0, RGB(0, 0, 0));
	
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
      dcMap.BitBlt(0, 0, rect.right, rect.bottom, &dc, m_ptOrg.x, m_ptOrg.y, SRCCOPY);

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
      AlphaBlend(dcMap.m_hDC, m_rcSelection.left, m_rcSelection.top,
                 cx, cy, dcTemp.m_hDC, 0, 0, cx, cy, bf);
      
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
      sdc.BitBlt(0, 0, rect.right, rect.bottom, &dc, m_ptOrg.x, m_ptOrg.y, SRCCOPY);
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
   CDC *pdc, dc, dcBmp;           // Window dc and in-memory dc
   CBitmap *pOldBitmap;
   CBrush brush;
   CPen *pOldPen;
   RECT rcClient, rcBitmap;
   DWORD i;
   CImageList *pImageList;
   CFont *pOldFont;
   POINT ptOffset, ptLinkStart, ptLinkEnd;
	CSize te;
	int dx, dy, nLen;
	TCHAR *pszText;
   
   GetClientRect(&rcClient);

   // Create compatible DC and bitmap for painting
   pdc = GetDC();
   dc.CreateCompatibleDC(pdc);
   bitmap.DeleteObject();
   rcBitmap.left = 0;
   rcBitmap.top = 0;
   if (bSelectionOnly)
   {
      rcBitmap.right = prcSel->right - prcSel->left;
      rcBitmap.bottom = prcSel->bottom - prcSel->top;
      ptOffset.x = -prcSel->left;
      ptOffset.y = -prcSel->top;
   }
   else
   {
      rcBitmap.right = max(m_ptMapSize.x, rcClient.right);
      rcBitmap.bottom = max(m_ptMapSize.y, rcClient.bottom);
      ptOffset.x = 0;
      ptOffset.y = 0;
   }
   bitmap.CreateCompatibleBitmap(pdc, rcBitmap.right, rcBitmap.bottom);

   // Initial DC setup
   pOldBitmap = dc.SelectObject(&bitmap);
   dc.SetBkColor(m_rgbBkColor);
   brush.CreateSolidBrush(m_rgbBkColor);
   if (m_scaleInfo[m_nScale].nFontIndex != -1)
      pOldFont = dc.SelectObject(&m_fontList[m_scaleInfo[m_nScale].nFontIndex]);
   else
      pOldFont = NULL;

   // Draw background
   dc.FillRect(&rcBitmap, &brush);
   if ((m_hBkImage != NULL) && (!bSelectionOnly))
   {
      CBitmap bmpBkImage, *pOldBkImage;

      dcBmp.CreateCompatibleDC(pdc);
      bmpBkImage.Attach(m_hBkImage);
      pOldBkImage = dcBmp.SelectObject(&bmpBkImage);
      dc.BitBlt(0, 0, rcBitmap.right, rcBitmap.bottom, &dcBmp, 0, 0, SRCCOPY);
      dcBmp.SelectObject(pOldBkImage);
      bmpBkImage.Detach();
      dcBmp.DeleteDC();
   }
   ReleaseDC(pdc);

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
         // Draw links between objects
         pOldPen = dc.SelectObject(&m_penLinkTypes[0]);
			dc.SetBkColor(m_rgbLabelBkColor);
			dc.SetTextColor(m_rgbLabelTextColor);
         for(i = 0; i < m_pSubmap->GetNumLinks(); i++)
         {
            ptLinkStart = m_pSubmap->GetObjectPosition(m_pSubmap->GetLinkByIndex(i)->dwId1);
            ptLinkEnd = m_pSubmap->GetObjectPosition(m_pSubmap->GetLinkByIndex(i)->dwId2);

            ScalePosMapToScreen(&ptLinkStart);
            ptLinkStart.x += m_scaleInfo[m_nScale].ptObjectSize.x / 2;
            ptLinkStart.y += m_scaleInfo[m_nScale].ptObjectSize.y / 2;

            ScalePosMapToScreen(&ptLinkEnd);
            ptLinkEnd.x += m_scaleInfo[m_nScale].ptObjectSize.x / 2;
            ptLinkEnd.y += m_scaleInfo[m_nScale].ptObjectSize.y / 2;

            dc.SelectObject(&m_penLinkTypes[m_pSubmap->GetLinkByIndex(i)->nType]);
            dc.MoveTo(ptLinkStart);
            dc.LineTo(ptLinkEnd);

				if (m_bShowConnectorNames && (m_scaleInfo[m_nScale].nFontIndex != -1))
				{
					dx = (ptLinkEnd.x - ptLinkStart.x) / 3;
					//if (abs(dx) > 20 + MAP_OBJECT_SIZE_X)
					//	dx = dx < 0 ? (-20 - MAP_OBJECT_SIZE_X): (20 + MAP_OBJECT_SIZE_X);
					dy = (ptLinkEnd.y - ptLinkStart.y) / 3;
					//if (abs(dy) > 20 + MAP_OBJECT_SIZE_Y)
					//	dy = dy < 0 ? (-20 - MAP_OBJECT_SIZE_Y): (20 + MAP_OBJECT_SIZE_Y);

					pszText = m_pSubmap->GetLinkByIndex(i)->szPort1;
					nLen = _tcslen(pszText);
					te = dc.GetTextExtent(pszText, nLen);
					dc.TextOut(ptLinkStart.x + dx - te.cx / 2, ptLinkStart.y + dy - te.cy / 2, pszText, nLen);

					pszText = m_pSubmap->GetLinkByIndex(i)->szPort2;
					nLen = _tcslen(pszText);
					te = dc.GetTextExtent(pszText, nLen);
					dc.TextOut(ptLinkEnd.x - dx - te.cx / 2, ptLinkEnd.y - dy - te.cy / 2, pszText, nLen);
				}
         }
         dc.SelectObject(pOldPen);

         // Draw objects
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
   CBrush brush, *pOldBrush;
   RECT rect;
   DWORD dwState;

   pObject = NXCFindObjectById(g_hSession, m_pSubmap->GetObjectIdFromIndex(dwIndex));
   if (pObject != NULL)
   {
      dwState = m_pSubmap->GetObjectStateFromIndex(dwIndex);

      // Get and scale object's position
      pt = m_pSubmap->GetObjectPositionByIndex(dwIndex);
      ScalePosMapToScreen(&pt);

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
      brush.CreateSolidBrush(g_statusColorTable[pObject->iStatus]);
      pOldBrush = dc.SelectObject(&brush);
      dc.RoundRect(&rect, CPoint(6, 6));
      dc.SelectObject(pOldBrush);
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
   m_dwHistorySize = 0;
   m_dwHistoryPos = 0;
   delete m_pMap;
   m_pMap = pMap;
   OpenSubmap(m_pMap->ObjectId(), FALSE);
   m_bIsModified = FALSE;
}


//
// Repaint entire map
//

void CMapView::Update()
{
   DrawOnBitmap(m_bmpMap, FALSE, NULL);
   UpdateScrollBars(TRUE);
}


//
// (Re)do current submap layout
//

void CMapView::DoSubmapLayout()
{
   NXC_OBJECT *pObject;
   RECT rect;

   // Submap id 0 means that automatic layout turned off
   if (m_pSubmap->Id() != 0)
   {
      pObject = NXCFindObjectById(g_hSession, m_pSubmap->Id());
      ASSERT(pObject != NULL);
      if (pObject != NULL)
      {
         GetClientRect(&rect);
         if (pObject->dwId == 1)    // "Entire Network" object
         {
            DWORD i, j, k, dwNumObjects, dwObjListSize, *pdwObjectList, dwNumLinks = 0;
            DWORD dwNumVpns, dwNumSubnets, *pdwSubnetList = NULL;
            OBJLINK *pLinkList = NULL;
            NXC_OBJECT_INDEX *pIndex;
            NXC_OBJECT *pSibling;
         
            dwObjListSize = pObject->dwNumChilds;
            pdwObjectList = (DWORD *)nx_memdup(pObject->pdwChildList, sizeof(DWORD) * dwObjListSize);

            NXCLockObjectIndex(g_hSession);
            pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(g_hSession, &dwNumObjects);
            for(i = 0; i < dwNumObjects; i++)
            {
               if (pIndex[i].pObject->iClass == OBJECT_NODE)
               {
                  pdwSubnetList = (DWORD *)realloc(pdwSubnetList, sizeof(DWORD) * pIndex[i].pObject->dwNumParents);
                  for(j = 0, dwNumSubnets = 0; j < pIndex[i].pObject->dwNumParents; j++)
                  {
                     pSibling = NXCFindObjectByIdNoLock(g_hSession, pIndex[i].pObject->pdwParentList[j]);
                     if ((pSibling != NULL) &&
                         (pSibling->iClass == OBJECT_SUBNET))
                     {
                        pdwSubnetList[dwNumSubnets] = pIndex[i].pObject->pdwParentList[j];
                        dwNumSubnets++;
                     }
                  }
                  for(j = 0, dwNumVpns = 0; j < pIndex[i].pObject->dwNumChilds; j++)
                  {
                     pSibling = NXCFindObjectByIdNoLock(g_hSession, pIndex[i].pObject->pdwChildList[j]);
                     if ((pSibling != NULL) &&
                         (pSibling->iClass == OBJECT_VPNCONNECTOR))
                     {
                        dwNumVpns++;

                        // Check for duplicated links
                        for(k = 0; k < dwNumLinks; k++)
                           if ((((pLinkList[k].dwId1 == pSibling->vpnc.dwPeerGateway) &&
                                 (pLinkList[k].dwId2 == pIndex[i].dwKey)) ||
                                ((pLinkList[k].dwId1 == pIndex[i].dwKey) &&
                                 (pLinkList[k].dwId2 == pSibling->vpnc.dwPeerGateway))) &&
                                 (pLinkList[k].nType == LINK_TYPE_VPN))
                              break;

                        if (k == dwNumLinks)
                        {
                           // Add new link
                           dwNumLinks++;
                           pLinkList = (OBJLINK *)realloc(pLinkList, sizeof(OBJLINK) * dwNumLinks);
                           pLinkList[k].dwId1 = pIndex[i].dwKey;
                           pLinkList[k].dwId2 = pSibling->vpnc.dwPeerGateway;
                           pLinkList[k].nType = LINK_TYPE_VPN;
                        }
                     }
                  }
                  if ((dwNumSubnets > 1) || (dwNumVpns > 0))
                  {
                     // Node connected to more than one subnet or
                     // have VPN connectors, so add it to the map
                     pdwObjectList = (DWORD *)realloc(pdwObjectList, sizeof(DWORD) * (dwObjListSize + 1));
                     pdwObjectList[dwObjListSize] = pIndex[i].dwKey;
                     dwObjListSize++;

                     j = dwNumLinks;
                     dwNumLinks += dwNumSubnets;
                     pLinkList = (OBJLINK *)realloc(pLinkList, sizeof(OBJLINK) * dwNumLinks);
                     for(k = 0; k < dwNumSubnets; j++, k++)
                     {
                        pLinkList[j].dwId1 = pIndex[i].dwKey;
                        pLinkList[j].dwId2 = pdwSubnetList[k];
                        pLinkList[j].nType = LINK_TYPE_NORMAL;
                     }
                  }
               }
            }
            NXCUnlockObjectIndex(g_hSession);
            safe_free(pdwSubnetList);

            m_pSubmap->DoLayout(dwObjListSize, pdwObjectList,
                                dwNumLinks, pLinkList, rect.right, rect.bottom,
                                SUBMAP_LAYOUT_RADIAL, TRUE);
            safe_free(pdwObjectList);
            safe_free(pLinkList);
         }
         else
         {
            m_pSubmap->DoLayout(pObject->dwNumChilds, pObject->pdwChildList,
                                0, NULL, rect.right, rect.bottom,
                                SUBMAP_LAYOUT_DUMB, FALSE);
         }
         m_bIsModified = TRUE;
      }
      else
      {
         MessageBox(_T("Internal error: cannot find root object for the current submap"),
                    _T("Error"), MB_OK | MB_ICONSTOP);
      }
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

   if (m_pSubmap != NULL)
   {
      pt.x += m_ptOrg.x;
      pt.y += m_ptOrg.y;
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
   }
   return 0;
}


//
// Generic processing for mouse button down message (either left or right)
//

void CMapView::OnMouseButtonDown(UINT nFlags, POINT point)
{
   m_ptMouseOpStart = point;
   m_dwFocusedObject = PointInObject(point);
   if (m_dwFocusedObject != 0)
   {
      m_dwFocusedObjectIndex = m_pSubmap->GetObjectIndex(m_dwFocusedObject);
      if (m_pSubmap->GetObjectStateFromIndex(m_dwFocusedObjectIndex) & MOS_SELECTED)
      {
         if (nFlags & (MK_CONTROL | MK_SHIFT))
         {
            m_pSubmap->SetObjectStateByIndex(m_dwFocusedObjectIndex, 0);
            Update();
         }
         else
         {
            m_nState = STATE_OBJECT_LCLICK;
         }
      }
      else
      {
         if ((nFlags & (MK_CONTROL | MK_SHIFT)) == 0)
            ClearSelection(FALSE);
         m_pSubmap->SetObjectStateByIndex(m_dwFocusedObjectIndex, MOS_SELECTED);
         Update();
         m_nState = STATE_OBJECT_LCLICK;
      }
   }
   else
   {
      if ((nFlags & (MK_CONTROL | MK_SHIFT)) == 0)
         ClearSelection();
      m_rcSelection.left = m_rcSelection.right = point.x;
      m_rcSelection.top = m_rcSelection.bottom = point.y;
      m_nState = STATE_SELECTING;
   }
   SetCapture();
}


//
// Generic processing for mouse button up message (either left or right)
//

void CMapView::OnMouseButtonUp(UINT nFlags, POINT point)
{
   ReleaseCapture();
   switch(m_nState)
   {
      case STATE_DRAGGING:
         m_pDragImageList->DragLeave(this);
         m_pDragImageList->EndDrag();
         delete_and_null(m_pDragImageList);
         point.x -= m_ptMouseOpStart.x;
         point.y -= m_ptMouseOpStart.y;
         ScalePosScreenToMap(&point);
         MoveSelectedObjects(point.x, point.y);
         break;
      case STATE_SELECTING:
         if ((nFlags & (MK_CONTROL | MK_SHIFT)) == 0)
            ClearSelection(FALSE);
         OffsetRect(&m_rcSelection, m_ptOrg.x, m_ptOrg.y);
         SelectObjectsInRect(m_rcSelection);
         Update();
         break;
      default:
         break;
   }
   m_nState = STATE_NORMAL;
}


//
// WM_LBUTTONDBLCLK message handler
//

void CMapView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
   DWORD dwId;

	if (m_bCanOpenObjects)
	{
		dwId = PointInObject(point);
		if (dwId != 0)
			OpenSubmap(dwId);
	}
}


//
// WM_LBUTTONDOWN message handler
//

void CMapView::OnLButtonDown(UINT nFlags, CPoint point) 
{
   OnMouseButtonDown(nFlags, point);
}


//
// WM_LBUTTONUP message handler
//

void CMapView::OnLButtonUp(UINT nFlags, CPoint point) 
{
   OnMouseButtonUp(nFlags, point);
}


//
// WM_RBUTTONDOWN message handler
//

void CMapView::OnRButtonDown(UINT nFlags, CPoint point) 
{
   OnMouseButtonDown(nFlags, point);
}


//
// WM_RBUTTONUP message handler
//

void CMapView::OnRButtonUp(UINT nFlags, CPoint point) 
{
   OnMouseButtonUp(nFlags, point);
   ClientToScreen(&point);
   GetParent()->SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELONG(point.x, point.y));
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

   if (m_pSubmap != NULL)
   {
      for(i = m_pSubmap->GetNumObjects(); i > 0; i--)
         m_pSubmap->SetObjectStateByIndex(i - 1, m_pSubmap->GetObjectStateFromIndex(i - 1) & ~MOS_SELECTED);
      if (bRedraw)
         Update();
   }
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
   m_ptMapSize = m_pSubmap->GetMinSize();
   ScalePosMapToScreen(&m_ptMapSize);
   Update();
   m_bIsModified = TRUE;
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

   m_pDragImageList->BeginDrag(0, CPoint(point.x - rcSel.left + m_ptOrg.x, point.y - rcSel.top + m_ptOrg.y));
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


//
// Open submap with given ID
//

void CMapView::OpenSubmap(DWORD dwId, BOOL bAddToHistory)
{
   nxSubmap *pSubmap;

   pSubmap = m_pMap->GetSubmap(dwId);
   if (pSubmap != NULL)
   {
      // Update history and change current submap
      if ((bAddToHistory) && (m_pSubmap != NULL))
         AddToHistory(m_pSubmap->Id());
      m_pSubmap = pSubmap;

      // Prepare objects on submap
      if ((!m_pSubmap->IsLayoutCompleted()) && m_pSubmap->GetAutoLayoutFlag())
         DoSubmapLayout();
      ClearSelection(FALSE);

      // Change background image
      if (m_hBkImage != NULL)
         DeleteObject(m_hBkImage);
      if (m_pSubmap->GetBkImageFlag())
      {
         m_hBkImage = GetBkImage(m_pMap->MapId(), dwId, m_scaleInfo[m_nScale].nFactor);
      }
      else
      {
         m_hBkImage = NULL;
      }

      // Update map size in pixels
      m_ptMapSize = m_pSubmap->GetMinSize();
      ScalePosMapToScreen(&m_ptMapSize);
      
      // Redraw everything
      Update();

      // Notify parent
      GetParent()->SendMessage(NXCM_SUBMAP_CHANGE, 0, (LPARAM)m_pSubmap);
   }
   else
   {
      MessageBox(_T("Unable to get submap object"), _T("Internal error"), MB_OK | MB_ICONSTOP);
   }
}


//
// Convert map coordinates to screen coordinates according to current scale factor
//

void CMapView::ScalePosMapToScreen(POINT *pt)
{
   if (m_scaleInfo[m_nScale].nFactor > 0)
   {
      pt->x *= m_scaleInfo[m_nScale].nFactor;
      pt->y *= m_scaleInfo[m_nScale].nFactor;
   }
   else if (m_scaleInfo[m_nScale].nFactor < 0)
   {
      pt->x /= -m_scaleInfo[m_nScale].nFactor;
      pt->y /= -m_scaleInfo[m_nScale].nFactor;
   }
}


//
// Convert screen coordinates to map coordinates according to current scale factor
//

void CMapView::ScalePosScreenToMap(POINT *pt)
{
   if (m_scaleInfo[m_nScale].nFactor > 0)
   {
      pt->x /= m_scaleInfo[m_nScale].nFactor;
      pt->y /= m_scaleInfo[m_nScale].nFactor;
   }
   else if (m_scaleInfo[m_nScale].nFactor < 0)
   {
      pt->x *= -m_scaleInfo[m_nScale].nFactor;
      pt->y *= -m_scaleInfo[m_nScale].nFactor;
   }
}


//
// Update scroll bars state and position
//

void CMapView::UpdateScrollBars(BOOL bForceUpdate)
{
   SCROLLINFO si;
   RECT rect;
   BOOL bUpdateWindow = bForceUpdate;

   GetClientRect(&rect);
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
   si.nMin = 0;

   // Set scroll bars ranges
   si.nMax = (rect.right < m_ptMapSize.x) ? m_ptMapSize.x : 0;
   si.nPage = rect.right;
   if (si.nMax == 0)
   {
      // Set X origin to 0 if there are no horizontal scroll bar anymore
      if (m_ptOrg.x > 0)
      {
         m_ptOrg.x = 0;
         bUpdateWindow = TRUE;
      }
   }
   else
   {
      // Adjust X origin so map will fully occupy window
      if (m_ptOrg.x > (int)(si.nMax - si.nPage))
      {
         m_ptOrg.x = si.nMax - si.nPage;
         bUpdateWindow = TRUE;
      }
   }
   si.nPos = m_ptOrg.x;
   SetScrollInfo(SB_HORZ, &si);

   si.nMax = (rect.bottom < m_ptMapSize.y) ? m_ptMapSize.y : 0;
   si.nPage = rect.bottom;
   if (si.nMax == 0)
   {
      // Set Y origin to 0 if there are no vertical scroll bar anymore
      if (m_ptOrg.y > 0)
      {
         m_ptOrg.y = 0;
         bUpdateWindow = TRUE;
      }
   }
   else
   {
      // Adjust Y origin so map will fully occupy window
      if (m_ptOrg.y > (int)(si.nMax - si.nPage))
      {
         m_ptOrg.y = si.nMax - si.nPage;
         bUpdateWindow = TRUE;
      }
   }
   si.nPos = m_ptOrg.y;
   SetScrollInfo(SB_VERT, &si);

   if (bUpdateWindow)
      Invalidate(FALSE);
}


//
// WM_SIZE message handler
//

void CMapView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
   Update();
}


//
// WM_HSCROLL message handler
//

void CMapView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
   int iNewPos;

   // Calculate new position
   iNewPos = CalculateNewScrollPos(SB_HORZ, nSBCode, nPos);

   // Update X origin
   if (iNewPos != -1)
   {
      SetScrollPos(SB_HORZ, iNewPos);
      m_ptOrg.x = iNewPos;
      Invalidate(FALSE);
   }
}


//
// WM_VSCROLL message handler
//

void CMapView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
   int iNewPos;

   // Calculate new position
   iNewPos = CalculateNewScrollPos(SB_VERT, nSBCode, nPos);

   // Update Y origin
   if (iNewPos != -1)
   {
      SetScrollPos(SB_VERT, iNewPos);
      m_ptOrg.y = iNewPos;
      Invalidate(FALSE);
   }
}


//
// Calculate new scroll bar position
//

int CMapView::CalculateNewScrollPos(UINT nScrollBar, UINT nSBCode, UINT nPos)
{
   int iNewPos;
   RECT rect;

   GetClientRect(&rect);
   switch(nSBCode)
   {
      case SB_THUMBPOSITION:
         iNewPos = nPos;
         break;
      case SB_TOP:
         iNewPos = 0;
         break;
      case SB_BOTTOM:
         iNewPos = (nScrollBar == SB_HORZ) ? m_ptMapSize.x - rect.right :
                        m_ptMapSize.y - rect.bottom;
         break;
      case SB_PAGEUP:
         iNewPos = (nScrollBar == SB_HORZ) ? m_ptOrg.x - rect.right : m_ptOrg.y - rect.bottom;
         if (iNewPos < 0)
            iNewPos = 0;
         break;
      case SB_PAGEDOWN:
         iNewPos = (nScrollBar == SB_HORZ) ? 
               min(m_ptOrg.x + rect.right, m_ptMapSize.x - rect.right) :
               min(m_ptOrg.y + rect.bottom, m_ptMapSize.y - rect.bottom);
         break;
      case SB_LINEUP:
         if (nScrollBar == SB_HORZ)
            iNewPos = (m_ptOrg.x > SCROLL_STEP) ? m_ptOrg.x - SCROLL_STEP : 0;
         else
            iNewPos = (m_ptOrg.y > SCROLL_STEP) ? m_ptOrg.y - SCROLL_STEP : 0;
         break;
      case SB_LINEDOWN:
         if (nScrollBar == SB_HORZ)
            iNewPos = (m_ptOrg.x + SCROLL_STEP < m_ptMapSize.x - rect.right) ? 
               (m_ptOrg.x + SCROLL_STEP) : (m_ptMapSize.x - rect.right);
         else
            iNewPos = (m_ptOrg.y + SCROLL_STEP < m_ptMapSize.y - rect.bottom) ? 
               (m_ptOrg.y + SCROLL_STEP) : (m_ptMapSize.y - rect.bottom);
         break;
      default:
         iNewPos = -1;  // Ignore other codes
         break;
   }
   return iNewPos;
}


//
// Zoom in
//

void CMapView::ZoomIn()
{
   if (m_nScale < MAX_ZOOM)
   {
      ScalePosScreenToMap(&m_ptOrg);
      m_nScale++;
      ScalePosMapToScreen(&m_ptOrg);
      if (m_hBkImage != NULL)
      {
         DeleteObject(m_hBkImage);
         m_hBkImage = GetBkImage(m_pMap->MapId(), m_pSubmap->Id(), m_scaleInfo[m_nScale].nFactor);
      }
      m_ptMapSize = m_pSubmap->GetMinSize();
      ScalePosMapToScreen(&m_ptMapSize);
      Update();
   }
}


//
// Check if zoom in can be applied
//

BOOL CMapView::CanZoomIn()
{
   return m_nScale < MAX_ZOOM;
}


//
// Zoom out
//

void CMapView::ZoomOut()
{
   if (m_nScale > 0)
   {
      ScalePosScreenToMap(&m_ptOrg);
      m_nScale--;
      ScalePosMapToScreen(&m_ptOrg);
      if (m_hBkImage != NULL)
      {
         DeleteObject(m_hBkImage);
         m_hBkImage = GetBkImage(m_pMap->MapId(), m_pSubmap->Id(), m_scaleInfo[m_nScale].nFactor);
      }
      m_ptMapSize = m_pSubmap->GetMinSize();
      ScalePosMapToScreen(&m_ptMapSize);
      Update();
   }
}


//
// Check if zoom out can be applied
//

BOOL CMapView::CanZoomOut()
{
   return m_nScale > 0;
}


//
// Go to parent object from the current
//

void CMapView::GoToParentObject()
{
   NXC_OBJECT *pObject;
   DWORD i;

   if (m_pSubmap != NULL)
   {
      pObject = NXCFindObjectById(g_hSession, m_pSubmap->Id());
      if (pObject != NULL)
      {
         for(i = 0; i < pObject->dwNumParents; i++)
         {
            if (m_pMap->IsSubmapExist(pObject->pdwParentList[i]))
            {
               OpenSubmap(pObject->pdwParentList[i]);
               break;
            }
         }
      }
      else
      {
         MessageBox(_T("Unable to find object for current submap"), _T("Internal error"), MB_OK | MB_ICONSTOP);
      }
   }
}


//
// Return TRUE if it's possible to go to parent object
//

BOOL CMapView::CanGoToParent()
{
   if ((m_pMap == NULL) || (m_pSubmap == NULL))
      return FALSE;
   return m_pMap->ObjectId() != m_pSubmap->Id();
}


//
// Add submap id to history
//

void CMapView::AddToHistory(DWORD dwId)
{
   if (m_dwHistoryPos == MAX_HISTORY_SIZE - 1)
   {
      memmove(m_dwHistory, &m_dwHistory[1], sizeof(DWORD) * (MAX_HISTORY_SIZE - 1));
      m_dwHistoryPos--;
   }

   m_dwHistory[m_dwHistoryPos] = dwId;
   m_dwHistoryPos++;
   m_dwHistorySize = m_dwHistoryPos;
}


//
// Go back through submaps (to the previous one)
//

void CMapView::GoBack()
{
   if (m_dwHistoryPos > 0)
   {
      m_dwHistory[m_dwHistoryPos] = m_pSubmap->Id();
      m_dwHistoryPos--;
      OpenSubmap(m_dwHistory[m_dwHistoryPos], FALSE);
   }
}


//
// Check if we can go back now
//

BOOL CMapView::CanGoBack()
{
   return m_dwHistoryPos > 0;
}

//
// Go forward through visited submaps history
//

void CMapView::GoForward()
{
   if (m_dwHistoryPos < m_dwHistorySize)
   {
      m_dwHistoryPos++;
      OpenSubmap(m_dwHistory[m_dwHistoryPos], FALSE);
   }
}


//
// Check if we can go forward now
//

BOOL CMapView::CanGoForward()
{
   return m_dwHistoryPos < m_dwHistorySize;
}


//
// Open root submap
//

void CMapView::OpenRoot()
{
   if (m_pSubmap->Id() != m_pMap->ObjectId())
      OpenSubmap(m_pMap->ObjectId());
}


//
// Handler for object changes
//

void CMapView::OnObjectChange(DWORD dwObjectId, NXC_OBJECT *pObject)
{
   DWORD i, dwNumSubnets;
   NXC_OBJECT *pParent;
   BOOL bUpdate = FALSE;

   // Check if updated object should be presented on current submap
   for(i = 0; i < pObject->dwNumParents; i++)
      if (pObject->pdwParentList[i] == m_pSubmap->Id())
      {
         bUpdate = TRUE;
         break;
      }

   if ((!bUpdate) && (m_pSubmap->Id() == 1))
   {
      for(i = 0, dwNumSubnets = 0; i < pObject->dwNumParents; i++)
      {
         pParent = NXCFindObjectById(g_hSession, pObject->pdwParentList[i]);
         if ((pParent != NULL) &&
             (pParent->iClass == OBJECT_SUBNET))
         {
            dwNumSubnets++;
         }
      }
      if (dwNumSubnets > 1)
         bUpdate = TRUE;
   }

   if (bUpdate)
   {
      // Check if object already presented on current submap
      if (m_pSubmap->GetObjectIndex(dwObjectId) == INVALID_INDEX)
      {
         if (m_pSubmap->GetAutoLayoutFlag())
         {
            DoSubmapLayout();
         }
         else
         {
				m_pSubmap->SetObjectPosition(dwObjectId, 0, 0);		// Will add object implicitely
         }
      }
      Update();
   }
}


//
// Get list of selected objects
//

DWORD *CMapView::GetSelectedObjects(DWORD *pdwNumObjects)
{
   DWORD i, dwCount, *pdwList;

   pdwList = (DWORD *)malloc(sizeof(DWORD) * m_pSubmap->GetNumObjects());
   for(i = 0, dwCount = 0; i < m_pSubmap->GetNumObjects(); i++)
      if (m_pSubmap->GetObjectStateFromIndex(i) & MOS_SELECTED)
      {
         pdwList[dwCount] = m_pSubmap->GetObjectIdFromIndex(i);
         dwCount++;
      }
   *pdwNumObjects = dwCount;
   if (dwCount == 0)
   {
      free(pdwList);
      pdwList = NULL;
   }
   return pdwList;
}


//
// Set new background image for current submap
// To clear background image, hBitmap must be set to NULL
//

void CMapView::SetBkImage(HBITMAP hBitmap)
{
   TCHAR szFileName[MAX_PATH], szServerId[32];
   BYTE bsId[8];
   
   if (m_pSubmap != NULL)
   {
      // Delete cached file, if any
      NXCGetServerID(g_hSession, bsId);
      BinToStr(bsId, 8, szServerId);
      _sntprintf(szFileName, MAX_PATH, _T("%s") WORKDIR_BKIMAGECACHE _T("\\%s.%08X.%08X"),
                 g_szWorkDir, szServerId, m_pMap->MapId(), m_pSubmap->Id());
      DeleteFile(szFileName);

      // Replace in-memory image
      if (m_hBkImage != NULL)
         DeleteObject(m_hBkImage);
      m_hBkImage = hBitmap;
      m_pSubmap->SetBkImageFlag(hBitmap != NULL);
      Update();
      m_bIsModified = TRUE;
   }
   else
   {
      DeleteObject(hBitmap);
   }
}


//
// Get background image for submap (from local cache or server)
//

HBITMAP CMapView::GetBkImage(DWORD dwMapId, DWORD dwSubmapId, int nScaleFactor)
{
   TCHAR szFileName[MAX_PATH], szServerId[32];
   BYTE bsId[8];
   DWORD dwResult;

   NXCGetServerID(g_hSession, bsId);
   BinToStr(bsId, 8, szServerId);
   _sntprintf(szFileName, MAX_PATH, _T("%s") WORKDIR_BKIMAGECACHE _T("\\%s.%08X.%08X"),
              g_szWorkDir, szServerId, dwMapId, dwSubmapId);
   
   // Download file if needed
   if (_taccess(szFileName, 4) != 0)
   {
      dwResult = DoRequestArg4(NXCDownloadSubmapBkImage, g_hSession, (void *)dwMapId,
                               (void *)dwSubmapId, szFileName,
                               _T("Downloading background image from server..."));
      if (dwResult != RCC_SUCCESS)
      {
         theApp.ErrorBox(dwResult, _T("Cannot download background image: %s"));
      }
   }

   return LoadPicture(szFileName, nScaleFactor);
}


//
// Returns current scale factor
//

int CMapView::GetScaleFactor(void)
{
   return m_scaleInfo[m_nScale].nFactor;
}
