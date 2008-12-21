/* 
** NetXMS - Network Management System
** Windows Console
** Copyright (C) 2003, 2004 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: draw.cpp
** Various drawing and image functions
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Draw pie chart
//

void DrawPieChart(CDC &dc, RECT *pRect, int iNumElements, DWORD *pdwValues, COLORREF *pColors)
{
   int i, iTotalSize, iSum, cx, cy, ncx, ncy, dir, inc;
   int *piSize;
   CBrush brush, *pOldBrush; 
   CPen pen, *pOldPen;
   DWORD dwTotal;

   // Calculate total value
   for(i = 0, dwTotal = 0; i < iNumElements; i++)
      dwTotal += pdwValues[i];

   // Calculate size of each sector
   piSize = (int *)malloc(sizeof(int) * iNumElements);
   iTotalSize = ((pRect->bottom - pRect->top) + (pRect->right - pRect->left)) * 2;
   for(i = 0; i < iNumElements; i++)
      piSize[i] = (int)(iTotalSize * ((double)pdwValues[i] / (double)dwTotal));

   // Check if sum of all sizes equals total size of graph
   for(i = 0, iSum = 0; i < iNumElements; i++)
      iSum += piSize[i];
   if (iSum != iTotalSize)
   {
      inc = iTotalSize - iSum;
      
      // Correct largest sector (iSum here is a temporary variable to hold max value)
      for(i = 0, iSum = 0; i < iNumElements; i++)
         if (piSize[i] > iSum)
         {
            iSum = piSize[i];
            cx = i;
         }
      piSize[cx] += inc;
   }

   // Start coordinates
   ncx = cx = pRect->left;
   ncy = cy = pRect->top + (pRect->bottom - pRect->top) / 2;

   // Select pen for line drawing
   pen.CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
   pOldPen = dc.SelectObject(&pen);

   // Draw sectors
   for(i = 0, dir = 0; i < iNumElements; i++, cx = ncx, cy = ncy)
   {
      if (piSize[i] > 0)
      {
         do
         {
            switch(dir)
            {
               case 0:
                  inc = min(piSize[i], pRect->bottom - ncy);
                  ncy += inc;
                  piSize[i] -= inc;
                  if (ncy == pRect->bottom)
                     dir++;
                  break;
               case 1:
                  inc = min(piSize[i], pRect->right - ncx);
                  ncx += inc;
                  piSize[i] -= inc;
                  if (ncx == pRect->right)
                     dir++;
                  break;
               case 2:
                  inc = min(piSize[i], ncy - pRect->top);
                  ncy -= inc;
                  piSize[i] -= inc;
                  if (ncy == pRect->top)
                     dir++;
                  break;
               case 3:
                  inc = min(piSize[i], ncx - pRect->left);
                  ncx -= inc;
                  piSize[i] -= inc;
                  if (ncx == pRect->left)
                     dir = 0;
                  break;
            }
         }
         while(piSize[i] > 0);

         brush.CreateSolidBrush(pColors[i]);
         pOldBrush = dc.SelectObject(&brush);
         dc.Pie(pRect, CPoint(cx, cy), CPoint(ncx, ncy));
         dc.SelectObject(pOldBrush);
         brush.DeleteObject();
      }
   }

   // Cleanup
   dc.SelectObject(pOldPen);
   free(piSize);
}


//
// Draw 3D rectangle
//

void Draw3dRect(HDC hDC, LPRECT pRect, COLORREF rgbTop, COLORREF rgbBottom)
{
   CPen pen;
   HGDIOBJ hOldPen;

   // Draw left and top sides
   pen.CreatePen(PS_SOLID, 1, rgbTop);
   hOldPen = SelectObject(hDC, pen.m_hObject);
   MoveToEx(hDC, pRect->right - 1, pRect->top, NULL);
   LineTo(hDC, pRect->left, pRect->top);
   LineTo(hDC, pRect->left, pRect->bottom);
   SelectObject(hDC, hOldPen);
   pen.DeleteObject();

   // Draw right and bottom sides
   pen.CreatePen(PS_SOLID, 1, rgbBottom);
   hOldPen = SelectObject(hDC, pen.m_hObject);
   MoveToEx(hDC, pRect->left + 1, pRect->bottom, NULL);
   LineTo(hDC, pRect->right, pRect->bottom);
   LineTo(hDC, pRect->right, pRect->top);

   // Cleanup
   SelectObject(hDC, hOldPen);
}
