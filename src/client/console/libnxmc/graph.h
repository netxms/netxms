/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
** Copyright (C) 2007, 2008 Victor Kirhenshtein
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
** File: graph.h
**
**/

#ifndef _graph_h_
#define _graph_h_


//
// Constants
//

#define MAX_GRAPH_ITEMS    16
#define ZOOM_HISTORY_SIZE  16

#define GRAPH_TYPE_LINE		0
#define GRAPH_TYPE_AREA		1
#define GRAPH_TYPE_STACKED	2

#define GCS_CLASSIC			0
#define GCS_LIGHT				1

#define NXGF_SHOW_GRID     0x0001
#define NXGF_SHOW_LEGEND   0x0002
#define NXGF_SHOW_RULER    0x0004
#define NXGF_AUTOSCALE     0x0008
#define NXGF_IS_ACTIVE     0x0010
#define NXGF_IS_UPDATING   0x0020
#define NXGF_SHOW_TITLE    0x0040
#define NXGF_IS_SELECTING  0x0080


//
// Zoom information structure
//

struct LIBNXMC_EXPORTABLE ZOOM_INFO
{
   time_t timeFrom;
   time_t timeTo;
};


//
// Style of graph item
//

struct LIBNXMC_EXPORTABLE GRAPH_ITEM_STYLE
{
	int type;	// Line. area, or stacked
	wxColour rgbColor;
	int lineWidth;
	BOOL showAverage;
	BOOL showThresholds;
	BOOL showTrend;
};


//
// DCI information class
//

class LIBNXMC_EXPORTABLE DCIInfo
{
   // Attributes
public:
   DWORD m_dwNodeId;
   DWORD m_dwItemId;
   int m_iSource;
   int m_iDataType;
   TCHAR *m_pszParameter;
   TCHAR *m_pszDescription;

   // Methods
public:
   DCIInfo();
   DCIInfo(DCIInfo *pSrc);
   DCIInfo(DWORD dwNodeId, NXC_DCI *pItem);
   ~DCIInfo();
};


//
// nxGraph class
//

class LIBNXMC_EXPORTABLE nxGraph : public wxWindow
{
private:
   ZOOM_INFO m_zoomInfo[ZOOM_HISTORY_SIZE];
   int m_zoomLevel;
	wxBitmap *m_bitmap;
	NXC_DCI_DATA *m_data[MAX_GRAPH_ITEMS];
	DCIInfo **m_dciInfo;
	double m_maxValue;
	double m_currMaxValue;
	double m_secondsPerPixel;
	int m_lastGridSizeY;
	int m_numItems;
	time_t m_timeFrom;
	time_t m_timeTo;
	wxRect m_rectGraph;
	wxPoint m_currMousePos;
	wxString m_title;

	void DrawAreaGraph(wxMemoryDC &dc, NXC_DCI_DATA *data, wxColour rgbColor, int gridSize);
	void DrawLineGraph(wxMemoryDC &dc, NXC_DCI_DATA *data, wxColour rgbColor, int gridSize);
	int NextMonthOffset(time_t timeStamp);

public:
	DWORD m_flags;
	wxColour m_rgbGridColor;
	wxColour m_rgbBkColor;
	wxColour m_rgbAxisColor;
	wxColour m_rgbTextColor;
	wxColour m_rgbSelRectColor;
	wxColour m_rgbRulerColor;
	GRAPH_ITEM_STYLE m_graphItemStyles[MAX_GRAPH_ITEMS];

	nxGraph(wxWindow *parent, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
	virtual ~nxGraph();

	void Redraw();

	void SetData(DWORD index, NXC_DCI_DATA *data);
	void UpdateData(DWORD index, NXC_DCI_DATA *data);
	void SetTimeFrame(time_t timeFrom, time_t timeTo);
	void SetDCIInfo(DCIInfo **info) { m_dciInfo = info; }
	void SetColorScheme(int scheme);
	void SetUpdating(bool isUpdating) { if (isUpdating) m_flags |= NXGF_IS_UPDATING; else m_flags &= ~NXGF_IS_UPDATING; }

	bool IsRulerVisible() { return m_flags & NXGF_SHOW_RULER ? true : false; }
	bool IsGridVisible() { return m_flags & NXGF_SHOW_GRID ? true : false; }
	bool IsLegendVisible() { return m_flags & NXGF_SHOW_LEGEND ? true : false; }
	bool IsTitleVisible() { return m_flags & NXGF_SHOW_TITLE ? true : false; }
	bool IsAutoscale() { return m_flags & NXGF_AUTOSCALE ? true : false; }
	
	void SetRulerVisible(bool visible) { if (visible) m_flags |= NXGF_SHOW_RULER; else m_flags &= ~NXGF_SHOW_RULER; }
	void SetGridVisible(bool visible) { if (visible) m_flags |= NXGF_SHOW_GRID; else m_flags &= ~NXGF_SHOW_GRID; }
	void SetLegendVisible(bool visible) { if (visible) m_flags |= NXGF_SHOW_LEGEND; else m_flags &= ~NXGF_SHOW_LEGEND; }
	void SetTitleVisible(bool visible) { if (visible) m_flags |= NXGF_SHOW_TITLE; else m_flags &= ~NXGF_SHOW_TITLE; }

	wxBitmap *DrawGraphOnBitmap(wxSize &graphSize);

protected:
	void OnPaint(wxPaintEvent &event);
	void OnSetFocus(wxFocusEvent &event);
	void OnKillFocus(wxFocusEvent &event);
	void OnSize(wxSizeEvent &event);

	DECLARE_EVENT_TABLE();
};

#endif
