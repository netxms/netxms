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
** File: httpresponse.cpp
**
**/


#include "nxhttpd.h"


//
// Constructor
//

HttpResponse::HttpResponse()
{
	m_code = HTTP_INTERNALSERVERERROR;
	m_codeString = _T("500 Internal server error");
	m_body = NULL;
	m_bodyLen = 0;
}


//
// Destructor
//

HttpResponse::~HttpResponse()
{
	safe_free(m_body);
}


//
// Set body
//

void HttpResponse::SetBody(const TCHAR *data, int size, BOOL bAppend)
{
	// FIXME: add validation
	if ((m_body != NULL) && !bAppend)
	{
		free(m_body);
		m_body = NULL;
	}

	if (data != NULL)
	{
		int offset;

		if (size == -1)
		{
			size = (int)_tcslen(data);
		}

		if (bAppend)
		{
			offset = m_bodyLen;
			m_body = (char *)realloc(m_body, m_bodyLen + size);
		}
		else
		{
			offset = 0;
			m_body = (char *)malloc(size);
		}

		m_bodyLen = offset + size;
		memcpy(m_body + offset, data, size);
	}
	else
	{
		if (!bAppend)
			m_bodyLen = 0;
	}
}


//
// Set response code
//

void HttpResponse::SetCode(int code)
{
	switch (code)
	{
		case HTTP_OK:
			m_codeString = _T("200 OK");
			break;
		case HTTP_MOVEDPERMANENTLY:
			m_codeString = _T("301 Moved");
			break;
		case HTTP_FOUND:
			m_codeString = _T("302 Found");
			break;
		case HTTP_BADREQUEST:
			m_codeString = _T("400 Bad Request");
			break;
		case HTTP_UNAUTHORIZED:
			m_codeString = _T("401 Unauthorized");
			break;
		case HTTP_FORBIDDEN:
			m_codeString = _T("403 Forbidden");
			break;
		case HTTP_NOTFOUND:
			m_codeString = _T("404 Not Found");
			break;
		default:
			code = HTTP_INTERNALSERVERERROR;
			m_codeString = _T("500 Internal server error");
			break;
	}
	m_code = code;
}


//
// Set body and code from JSON object
// Code will be set to OK, except if JSON builder fails - 
// then it will be set to 500
//

void HttpResponse::SetJSONBody(JSONObjectBuilder &json)
{
	const TCHAR *data;

	data = json.CreateOutput();
	if (data != NULL)
	{
		SetCode(HTTP_OK);
		SetBody(data);
		//SetType("application/json");
	}
	else
	{
		SetCode(HTTP_INTERNALSERVERERROR);
		SetBody(m_codeString);
	}
}


//
// Build output stream
//

char *HttpResponse::BuildStream(int &size)
{
	String tmp;
	TCHAR szBuffer[64];
	char *out;
	DWORD i;

	_sntprintf(szBuffer, 64, _T("%u"), m_bodyLen);
	SetHeader(_T("Content-Lenght"), szBuffer);

	tmp = _T("HTTP/1.0 ");
	tmp += (TCHAR *)m_codeString;
	tmp += _T("\r\n");

	for(i = 0; i < m_headers.Size(); i++)
	{
		tmp += m_headers.GetKeyByIndex(i);
		tmp += _T(": ");
		tmp += m_headers.GetValueByIndex(i);
		tmp += _T("\r\n");
	}
	tmp += _T("\r\n");

	// TODO: add validation!
	size = tmp.Size() + m_bodyLen;
	out = (char *)malloc(size);

#ifdef UNICODE
#error NOT IMPLEMENTED YET
#else
	// TODO: convert to UTF-8 ????
	memcpy(out, (TCHAR *)tmp, tmp.Size());
	if ((m_bodyLen > 0) && (m_body != NULL))
		memcpy(out + tmp.Size(), m_body, m_bodyLen);
#endif

	return out;
}


//
// Start HTML page
//

void HttpResponse::BeginPage(const TCHAR *pszTitle)
{
	SetType(_T("text/html"));
	SetBody(_T("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n")	// Push IE6 and IE 5 into quirks mode
	        _T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\r\n")
	        _T("<html>\r\n<head>\r\n<title>"));
	SetBody(pszTitle != NULL ? pszTitle : _T("NetXMS Web Interface"), -1, TRUE);
	SetBody(_T("</title>\r\n")
	        _T("<script type=\"text/javascript\" src=\"/xmlextras.js\"></script>\r\n")
	        _T("<script type=\"text/javascript\" src=\"/common.js\"></script>\r\n")
	        _T("<link rel=\"stylesheet\" type=\"text/css\" href=\"/netxms.css\" media=\"screen, tv, projection\" title=\"Default\" />\r\n")
	        _T("<!--[if lt IE 7.]>\r\n")
	        _T("<script defer type=\"text/javascript\" src=\"/pngfix.js\"></script>\r\n")
	        _T("<![endif]-->\r\n")
	        _T("</head><body>"), -1, TRUE);
}


//
// Start box
//

void HttpResponse::StartBox(const TCHAR *pszTitle, const TCHAR *pszClass, const TCHAR *pszId, const TCHAR *pszTableClass, BOOL bContentOnly)
{
	String temp;

	if (!bContentOnly)
	{
		if (pszId != NULL)
		{
			temp.AddFormattedString(_T("<div id=\"%s\""), pszId);
			if (pszClass != NULL)
			{
				temp.AddFormattedString(_T(" class=\"%s\">\r\n"), pszClass);
			}
			else
			{
				temp += _T(">\r\n");
			}
		}
		else
		{
			temp.AddFormattedString(_T("<div class=\"%s\">\r\n"), (pszClass != NULL) ? pszClass : _T("box"));
		}
		AppendBody(temp);
	}
   m_dwBoxRowNumber = 0;
	if (pszTitle != NULL)
	{
		AppendBody(_T("<div class=\"boxTitle\"><table><tr valign=\"middle\"><td>"));
		AppendBody(pszTitle);
		if (pszTableClass != NULL)
		{
			temp = _T("");
			temp.AddFormattedString(_T("</td></tr></table></div><table class=\"%s\">\r\n"), pszTableClass);
			AppendBody(temp);
		}
		else
		{
			AppendBody(_T("</td></tr></table></div><table>\r\n"));
		}
	}
	else
	{
		if (pszTableClass != NULL)
		{
			temp = _T("");
			temp.AddFormattedString(_T("</td></tr></table></div><table class=\"%s\">\r\n"), pszTableClass);
			AppendBody(temp);
		}
		else
		{
			AppendBody(_T("<table>\r\n"));
		}
	}
}


//
// Start table header
//

void HttpResponse::StartTableHeader(const TCHAR *pszClass)
{
	if (pszClass == NULL)
	{
		AppendBody(_T("<tr class=\"tableHeader\">\r\n"));
	}
	else
	{
		AppendBody(_T("<tr class=\""));
		AppendBody(pszClass);
		AppendBody(_T("\">\r\n"));
	}
}


//
// Start box row
//

void HttpResponse::StartBoxRow(void)
{
   if (m_dwBoxRowNumber & 1)
   {
      AppendBody(_T("<tr class=\"odd\">"));
   }
   else
   {
      AppendBody(_T("<tr class=\"even\">"));
   }
   m_dwBoxRowNumber++;
}
