/* 
** NetXMS - Network Management System
** Windows Console
** Copyright (C) 2004 Victor Kirhenshtein
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
** Various drawing functions
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Draw pie chart
//

void DrawPieChart(CDC &dc, RECT *pRect, int iNumElements, DWORD *pdwValues, COLORREF *pColors)
{
   int i, iTotalSize, cx, cy, ncx, ncy;
   int *piSize;
   CBrush brush, *pOldBrush; 
   DWORD dwTotal;

   // Calculate total value
   for(i = 0, dwTotal = 0; i < iNumElements; i++)
      dwTotal += pdwValues[i];

   // Calculate size of each sector
   piSize = (int *)malloc(sizeof(int) * iNumElements);
   iTotalSize = ((pRect->bottom - pRect->top) + (pRect->right - pRect->left)) * 2;
   for(i = 0, dwTotal = 0; i < iNumElements; i++)
      piSize[i] = (int)(iTotalSize * ((double)pdwValues[i] / (double)dwTotal));

   cx = pRect->left;
   cy = pRect->top;

   for(i = 0; i < iNumElements; i++)
   {

      brush.CreateSolidBrush(pColors[i]);
      pOldBrush = dc.SelectObject(&brush);
      dc.Pie(pRect, CPoint(cx, cy), CPoint(ncx, ncy));
      dc.SelectObject(pOldBrush);
      brush.DeleteObject();
   }

   free(piSize);
}
