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
** File: dataview.cpp
**
**/

#include "nxhttpd.h"


//
// Compare two DCI values
//

int ClientSession::CompareDCIValues(NXC_DCI_VALUE *p1, NXC_DCI_VALUE *p2)
{
	int nRet;

	switch(m_nLastValuesSortMode)
	{
		case 0:	// ID
			nRet = COMPARE_NUMBERS(p1->dwId, p2->dwId);
			break;
		case 1:	// Description
			nRet = _tcsicmp(p1->szDescription, p2->szDescription);
			break;
		case 2:	// Value
			nRet = _tcsicmp(p1->szValue, p2->szValue);
			break;
		case 3:	// Timestamp
			nRet = COMPARE_NUMBERS(p1->dwTimestamp, p2->dwTimestamp);
			break;
		default:
			nRet = 0;
			break;
	}
	return nRet * m_nLastValuesSortDir;
}


//
// Callback for sorting DCI values
//

static int CompareDCIValuesCB(const void *p1, const void *p2, void *pArg)
{
	return ((ClientSession *)pArg)->CompareDCIValues((NXC_DCI_VALUE *)p1, (NXC_DCI_VALUE *)p2);
}


//
// Add last values table column header
//

static void AddLastValuesColumnHeader(HttpResponse &response, TCHAR *sid, int nCol, DWORD dwObjectId,
												  int nSortDir, BOOL bSortBy)
{
	TCHAR szTemp[1024];
	static const TCHAR *colNames[] = 
	{ 
		_T("ID"),
		_T("Description"),
		_T("Value"),
		_T("Timestamp")
	};

	_sntprintf(szTemp, 1024, _T("<td valign=\"center\">%s<a href=\"\" onclick=\"javascript:loadDivContent('last_values','%s','&cmd=getLastValues&mode=%d&dir=%d&obj=%d'); return false;\">%s</a>%s</td>\r\n"),
	           bSortBy ? _T("<table class=\"inner_table\"><tr><td width=\"5%\" style=\"padding-right: 0.3em;\">") : _T(""),
	           sid, nCol, bSortBy ? -nSortDir : nSortDir, dwObjectId, colNames[nCol],
				  bSortBy ? ((nSortDir < 0) ? _T("</td><td><img src=\"/images/sort_down.png\"/></td></tr></table>") : _T("</td><td><img src=\"/images/sort_up.png\"/></td></tr></table>")) : _T(""));
	response.AppendBody(szTemp);
}


//
// Show last values table
//

void ClientSession::ShowLastValues(HttpResponse &response, NXC_OBJECT *pObject, BOOL bReload)
{
	DWORD i, dwResult;
	String row, descr, value;
	TCHAR szTemp[64];

	if (!bReload)
	{
		safe_free_and_null(m_pValueList);
		m_dwNumValues = 0;

		dwResult = NXCGetLastValues(m_hSession, pObject->dwId, &m_dwNumValues, &m_pValueList);
	}
	else
	{
		dwResult = RCC_SUCCESS;
	}

	if (dwResult == RCC_SUCCESS)
	{
		if (m_dwNumValues > 0)
		{
			QSortEx(m_pValueList, m_dwNumValues, sizeof(NXC_DCI_VALUE), this, CompareDCIValuesCB);
			response.StartBox(NULL, _T("objectTable"), _T("last_values"), NULL, bReload);
			response.StartTableHeader(NULL);
			for(i = 0; i < 4; i++)
				AddLastValuesColumnHeader(response, m_sid, i, pObject->dwId,
				                          m_nLastValuesSortDir, (m_nLastValuesSortMode == (int)i));
			response.AppendBody(_T("<td></td></tr>\r\n"));

			for(i = 0; i < m_dwNumValues; i++)
			{
				response.StartBoxRow();
				row = _T("");
				descr = m_pValueList[i].szDescription;
				value = m_pValueList[i].szValue;
				row.AddFormattedString(_T("<td>%d</td><td>%s</td><td>%s</td><td>%s</td>")
				                       _T("<td><img src=\"/images/graph.png\" alt=\"Show graph\"/>&nbsp;")
				                       _T("<img src=\"/images/document.png\" alt=\"Show history\"/></td></tr>"),
											  m_pValueList[i].dwId, EscapeHTMLText(descr),
											  EscapeHTMLText(value),
											  FormatTimeStamp(m_pValueList[i].dwTimestamp, szTemp, TS_LONG_DATE_TIME));
				response.AppendBody(row);
			}

			response.EndBox(bReload);
		}
		else
		{
			ShowInfoMessage(response, _T("No available data"));
		}
	}
	else
	{
		ShowErrorMessage(response, dwResult);
	}
}


//
// Send last values in response to reload request
//

void ClientSession::SendLastValues(HttpRequest &request, HttpResponse &response)
{
	NXC_OBJECT *pObject;
	String tmp;

	if (request.GetQueryParam(_T("mode"), tmp))
		m_nLastValuesSortMode = _tcstol(tmp, NULL, 10);
	if (request.GetQueryParam(_T("dir"), tmp))
		m_nLastValuesSortDir = _tcstol(tmp, NULL, 10);

	if (request.GetQueryParam(_T("obj"), tmp))
	{
		pObject = NXCFindObjectById(m_hSession, _tcstoul(tmp, NULL, 10));
		if (pObject != NULL)
		{
			ShowLastValues(response, pObject, TRUE);
		}
	}
}
