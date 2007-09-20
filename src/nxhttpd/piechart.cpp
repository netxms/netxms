/* $Id: piechart.cpp,v 1.5 2007-09-20 13:04:00 victor Exp $ */
/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
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
** File: piechart.cpp
**
**/

#include "nxhttpd.h"
#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <math.h>


//
// Constructor
//

PieChart::PieChart()
{
	m_rawData = NULL;
	m_rawDataSize = 0;
	memset(m_labels, 0, sizeof(TCHAR *) * MAX_PIE_ELEMENTS);
	memset(m_values, 0, sizeof(double) * MAX_PIE_ELEMENTS);
	m_valueCount = 0;
	m_noDataLabel = "No data to display (total is zero).";
}


//
// Destructor
//

PieChart::~PieChart()
{
	int i;

	if (m_rawData != NULL)
		gdFree(m_rawData);
	for(i = 0; i < MAX_PIE_ELEMENTS; i++)
		safe_free(m_labels[i]);
}


//
// Set value
//

BOOL PieChart::SetValue(const TCHAR *label, double value)
{
	if (m_valueCount < MAX_PIE_ELEMENTS)
	{
		m_labels[m_valueCount] = _tcsdup(label);
		m_values[m_valueCount] = value;
		m_valueCount++;
		return TRUE;
	}
	return FALSE;
}


//
// Set value
//

void PieChart::SetNoDataLabel(const TCHAR *label)
{
	if (label != NULL)
	{
		m_noDataLabel = label;
	}
}


//
// Build chart
//

BOOL PieChart::Build(void)
{
	BOOL ret = FALSE;
	gdImagePtr img;
	double arc_rad[MAX_PIE_ELEMENTS + 1];
	double arc_dec[MAX_PIE_ELEMENTS + 1];
	double perc[MAX_PIE_ELEMENTS];
	double total = 0;
	int i, posX, posY;

	// colors
	int black;
	int background;

	img = gdImageCreate(PIE_WIDTH, PIE_HEIGHT);

	int centX = gdImageSX(img) / 2;
	int centY = gdImageSY(img) / 2;

	int d = 160;

	background = gdImageColorAllocate(img, 255, 255, 255);
	black = gdImageColorAllocate(img, 0, 0, 0);

	gdImageFill(img, 0, 0, background);
	gdImageColorTransparent(img, background);

	// count total
	for(i = 1; i <= m_valueCount; i++)
	{
		total += m_values[i - 1];
		arc_rad[i] = total * 2 * PI;
		arc_dec[i] = total * 360;
	}

	if (total > 0)
	{
		m_color[0] = gdImageColorAllocate(img,   0, 127,   0);
		m_color[1] = gdImageColorAllocate(img,   0, 255, 255);
		m_color[2] = gdImageColorAllocate(img, 255, 255,   0);
		m_color[3] = gdImageColorAllocate(img, 255, 128,   0);
		m_color[4] = gdImageColorAllocate(img, 200,   0,   0);
		m_color[5] = gdImageColorAllocate(img,  61,  12, 187);
		m_color[6] = gdImageColorAllocate(img, 192, 192, 192);
		m_color[7] = gdImageColorAllocate(img,  92,   0,   0);
		m_color[8] = gdImageColorAllocate(img, 255, 128, 255);
		m_colorCount = 9;

		arc_rad[0] = 0;
		arc_dec[0] = 0;

		// draw boundaries
		for(i = 1; i <= m_valueCount; i++)
		{
			perc[i - 1] = m_values[i - 1] / total;

			arc_rad[i] /= total;
			arc_dec[i] /= total;

			posX = (int)ROUND(centX + (d / 2) * sin(arc_rad[i]));
			posY = (int)ROUND(centY + (d / 2) * cos(arc_rad[i]));

			gdImageLine(img, centX, centY, posX, posY, black);
			gdImageArc(img, centX, centY, d, d, (int)(arc_dec[i - 1]), (int)(arc_dec[i]), black);
		}
		
		int currentColor = 0;
		//int hfw = gdImageFontWidth(1);
		//int vfw = gdImageFontHeight(1);
		int fontX = gdFontTiny->w;
		int fontY = gdFontTiny->h;

		// draw labels and fill
		for(i = 0; i < m_valueCount; i++)
		{
			double arc_rad_label = arc_rad[i] + 0.5 * perc[i] * 2 * PI;

			// fill
			posX = (int)(centX + (0.8 * (d / 2) * sin(arc_rad_label)));
			posY = (int)(centY + (0.8 * (d / 2) * cos(arc_rad_label)));

			if (perc[i] > 0)
			{
				gdImageFillToBorder(img, posX, posY, black, m_color[currentColor]);

				// label
				TCHAR s[128];

				_sntprintf(s, 128, _T("%s (%.2lf%%)"), m_labels[i], perc[i] * 100);
				s[127] = 0;
				
				posX = (int)(centX + (1.1 * (d / 2) * sin(arc_rad_label)));
				posY = (int)(centY + (1.1 * (d / 2) * cos(arc_rad_label)));
				if ((arc_rad_label > 0.5 * PI) && (arc_rad_label < 1.5 * PI))
				{
					posY = posY - fontY;
				}
				if (arc_rad_label > PI)
				{
					posX = posX - fontX * _tcslen(s);
				}
				gdImageString(img, gdFontTiny, posX, posY, (unsigned char *)s, black);
			}

			currentColor++;
			if (currentColor == m_colorCount)
			{
				currentColor = 0;
			}
		}
	}
	else
	{
		gdImageString(img, gdFontSmall,
				img->sx / 2 - (strlen(m_noDataLabel) * gdFontSmall->w / 2),
				img->sy / 2 - gdFontSmall->h / 2,
				(unsigned char *)m_noDataLabel, black);
	}

	m_rawData = gdImagePngPtr(img, &m_rawDataSize);
	if (m_rawData != NULL)
	{
		ret = TRUE;
	}

	gdImageDestroy(img);

	return ret;
}


//
// Clear chart
//

void PieChart::Clear(void)
{
	int i;

	if (m_rawData != NULL)
	{
		gdFree(m_rawData);
		m_rawData = NULL;
		m_rawDataSize = 0;
	}
	for(i = 0; i < MAX_PIE_ELEMENTS; i++)
		safe_free_and_null(m_labels[i]);
	m_valueCount = 0;
}
