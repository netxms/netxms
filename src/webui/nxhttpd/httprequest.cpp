/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein & Victor Kirhenshtein
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
** File: httprequest.cpp
**
**/

#include "nxhttpd.h"

#ifdef _DEBUG
#define REQUEST_READ_TIMESLICE	300
#else
#define REQUEST_READ_TIMESLICE	5
#endif


//
// Constructor
//

HttpRequest::HttpRequest()
{
	m_raw = "";
	m_uri = "";
	m_rawQuery = "";

	m_method = METHOD_INVALID;
}


//
// Destructor
//

HttpRequest::~HttpRequest()
{
}


//
// Read request from socket
//

BOOL HttpRequest::Read(SOCKET s)
{
	struct timeval tv;
	fd_set rdfs;
	BOOL ret = FALSE;
	time_t started;
	TCHAR buffer[2048];
	int err, size;
	
	time(&started);

	while(!ret && m_raw.Size() < 10240)
	{
		if (started + REQUEST_READ_TIMESLICE < time(NULL))
		{
			// timeout
			break;
		}

		FD_ZERO(&rdfs);
		FD_SET(s, &rdfs);
		
		tv.tv_sec = 0;
		tv.tv_usec = 100;

		err = select(SELECT_NFDS(s + 1), &rdfs, NULL, NULL, &tv);
		if (err == -1)
		{
			// error
#ifndef _WIN32
			if (errno != EINTR)
#endif
				break;
#ifndef _WIN32
			else
				continue;
#endif
		}
		if (err == 0)
		{
			// timeout;
			continue;
		}

		size = recv(s, buffer, sizeof(buffer), 0);
		if (size <= 0)
		{
			// error/eof
			break;
		}

		m_raw.AddString(buffer, size);

		if (IsComplete())
		{
			if (Parse())
			{
				ret = TRUE;
			}
			break;
		}
	}

	return ret;
}


//
// Check if request is complete
//

BOOL HttpRequest::IsComplete(void)
{
	BOOL ret = FALSE;
	TCHAR szBuffer[256];
	int start, end, endOfHeaders;

	if (m_raw.Size() < 15) // minimal: 'GET / HTTP/1.0\n' = 15
	{
		return FALSE;
	}

	// we assume that eos is CRLF, not LF or any mix of them
	if (!_tcscmp(m_raw.SubStr(0, 3, szBuffer), _T("GET")))
	{
		if (!_tcscmp(m_raw.SubStr(m_raw.Size() - 4, 4, szBuffer), _T("\r\n\r\n")))
		{
			m_method = METHOD_GET;
			ret = TRUE;
		}
	}
	else
	{
		if (!_tcscmp(m_raw.SubStr(0, 4, szBuffer), _T("POST")))
		{
			endOfHeaders = m_raw.Find(_T("\r\n\r\n"), 0);
			if (endOfHeaders != String::npos)
			{
				start = m_raw.Find(_T("\r\nContent-Length: "), 0);
				if (start != String::npos)
				{
					start += 18; // strlen("\r\nContent-Length: ")
					end = m_raw.Find(_T("\r\n"), start + 1);
					if (end > start)
					{
						if (end - start <= 8)
						{
							if ((int)m_raw.Size() >=
								endOfHeaders + 4 + atoi(m_raw.SubStr(start, end - start, szBuffer)))
							{
								m_method = METHOD_POST;
								ret = TRUE;
							}
						}
						else
						{
							// Too large content length
							ret = TRUE;
						}
					}
				}
				else
				{
					// broken POST?
					ret = TRUE;
				}
			}
		}
	}

	return ret;
}


//
// Parse request
//

BOOL HttpRequest::Parse(void)
{
	BOOL ret = FALSE;
	int start, end;

	if (m_method == METHOD_INVALID)
	{
		return FALSE;
	}

	start = m_raw.Find(_T(" "), 0);
	end = m_raw.Find(_T(" "), start + 1);

	if ((start != String::npos) && (end != String::npos))
	{
		// get uri
		m_uri.SetBuffer(m_raw.SubStr(start + 1, end - start - 1));

		start = m_uri.Find(_T("?"));
		if (start != String::npos)
		{
			// found query data
			m_rawQuery.SetBuffer(m_uri.SubStr(start + 1, -1));
			m_uri.SetBuffer(m_uri.SubStr(0, start));
		}

		if (m_method == METHOD_POST)
		{
			start = m_raw.Find(_T("\r\n\r\n"));

			m_rawQuery += _T("&");
			m_rawQuery.AddDynamicString(m_raw.SubStr(start + 4, -1));

			// ie sends \r\n at the end of req.
			start = m_rawQuery.Find(_T("\r"));
			if (start != String::npos)
			{
				m_rawQuery.SetBuffer(m_rawQuery.SubStr(0, start));
			}
			start = m_rawQuery.Find(_T("\n"));
			if (start != String::npos)
			{
				m_rawQuery.SetBuffer(m_rawQuery.SubStr(0, start));
			}
		}

		// parse query
		ret = ParseQueryString();
	}

	return ret;
}


//
// Parse query string (exctract separate arguments divided by &)
//

BOOL HttpRequest::ParseQueryString(void)
{
	int firstPos, secondPos;
	TCHAR *pszParam;

	firstPos = 0;
	secondPos = m_rawQuery.Find(_T("&"));
	pszParam = m_rawQuery.SubStr(firstPos, secondPos);
	ParseParameter(pszParam);
	while(secondPos != String::npos)
	{
		firstPos = secondPos + 1;
		secondPos = m_rawQuery.Find(_T("&"), firstPos);
		pszParam = m_rawQuery.SubStr(firstPos, (secondPos != String::npos) ? (secondPos - firstPos) : -1);
		ParseParameter(pszParam);
	}
	return TRUE;
}


//
// Decode %xx in query parameter
//

static void DecodeQueryParam(TCHAR *pszText)
{
	int nCode;
	TCHAR *p;

	for(p = pszText; *p != 0; p++)
	{
		if (*p == '%')
		{
			if (*(p + 1) != 0)
			{
				nCode = hex2bin(*(p + 1)) << 4;
				if (*(p + 2) != 0)
					nCode += hex2bin(*(p + 2));
				*p = nCode;
				memmove(p + 1, p + 3, _tcslen(p + 2) * sizeof(TCHAR));
			}
		}
	}
}


//
// Parse single query parameter and add it to parameter map
//

BOOL HttpRequest::ParseParameter(TCHAR *pszParam)
{
	TCHAR *pSep;
	BOOL bRet = FALSE;

	if (pszParam != NULL)
	{
		pSep = _tcschr(pszParam, _T('='));
		if (pSep != NULL)
		{
			*pSep = 0;
			pSep++;
			DecodeQueryParam(pSep);
			m_query.set(pszParam, pSep);
			bRet = TRUE;
		}
		free(pszParam);
	}
	return bRet;
}


//
// Set query parameter
//

void HttpRequest::SetQueryParam(const TCHAR *pszName, const TCHAR *pszValue)
{
	m_query.set(pszName, pszValue);
}


//
// Get query parameter
//

BOOL HttpRequest::GetQueryParam(const TCHAR *pszName, String &value)
{
	BOOL ret = FALSE;
	const TCHAR *pszValue;

	pszValue = m_query.get(pszName);
	if (pszValue != NULL)
	{
		value = pszValue;
		ret = TRUE;
	}
	return ret;
}


//
// Get method name
//

const TCHAR *HttpRequest::GetMethodName(void)
{
	return (m_method == METHOD_GET) ? _T("GET") : ((m_method == METHOD_POST) ? _T("POST") : _T("INVALID"));
}
