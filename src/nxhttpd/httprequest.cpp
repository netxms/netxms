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

HttpRequest::HttpRequest()
{
	m_raw = "";
	m_uri = "";
	m_rawQuery = "";

	method = METHOD_INVALID;
}

HttpRequest::~HttpRequest()
{
}

bool HttpRequest::Read(SOCKET s)
{
	struct timeval tv;
	fd_set rdfs;
	bool ret = false;
	time_t started;
	char buffer[2048];
	
	time(&started);

	while(ret == false && m_raw.size() < 10240)
	{
		if (started + 5 < time(NULL))
		{
			// timeout
			break;
		}

		FD_ZERO(&rdfs);
		FD_SET(s, &rdfs);
		
		tv.tv_sec = 0;
		tv.tv_usec = 100;

		int err = select(s + 1, &rdfs, NULL, NULL, &tv);
		if (err == -1)
		{
			// error
			break;
		}
		if (err == 0)
		{
			// timeout;
			continue;
		}

		int size = ::recv(s, buffer, sizeof(buffer), 0);
		if (size <= 0)
		{
			// error/eof
			break;
		}

		m_raw += string(buffer, size);

		if (IsComplete())
		{
			if (Parse())
			{
				ret = true;
			}

			break;
		}
	}

	return ret;
}

bool HttpRequest::IsComplete(void)
{
	bool ret = false;

	if (m_raw.size() < 15) // minimal: 'GET / HTTP/1.0\n' = 15
	{
		return false;
	}

	// we assume that eos is CRLF, not LF or any mix of them
	if (m_raw.substr(0, 3) == "GET")
	{
		if (m_raw.substr(m_raw.size() - 4, 4) == "\r\n\r\n")
		{
			method = METHOD_GET;
			ret = true;
		}
	}
	else
	{
		if (m_raw.substr(0, 4) == "POST")
		{
			int endOfHeaders = m_raw.find("\r\n\r\n", 0);
			if (endOfHeaders != string::npos)
			{
				int start = m_raw.find("\r\nContent-Length: ", 0);
				if (start != string::npos)
				{
					start += 18; // strlen("\r\nContent-Length: ")
					int end = m_raw.find("\r\n", start + 1);
					if (end > start)
					{
						if (m_raw.size() >=
							endOfHeaders + 4 + atoi(m_raw.substr(start, end - start).c_str()))
						{
							method = METHOD_POST;
							ret = true;
						}
					}
				}
				else
				{
					// broken POST?
					ret = true;
				}
			}
		}
	}

	return ret;
}

bool HttpRequest::Parse(void)
{
	bool ret = false;

	if (method == METHOD_INVALID)
	{
		return false;
	}


	int start = m_raw.find(" ", 0);
	int end = m_raw.find(" ", start + 1);

	if (start != string::npos && end != string::npos)
	{
		// get uri
		m_uri = m_raw.substr(start + 1, end - start - 1);

		start = m_uri.find("?");
		if (start != string::npos)
		{
			// found query data
			m_rawQuery = m_uri.substr(start + 1);
			m_uri = m_uri.substr(0, start);
		}

		if (method == METHOD_POST)
		{
			start = m_raw.find("\r\n\r\n");

			m_rawQuery += "&";
			m_rawQuery += m_raw.substr(start + 4);

			// ie sends \r\n at the end of req.
			start = m_rawQuery.find("\r");
			if (start != string::npos)
			{
				m_rawQuery = m_rawQuery.substr(0, start);
			}
			start = m_rawQuery.find("\n");
			if (start != string::npos)
			{
				m_rawQuery = m_rawQuery.substr(0, start);
			}
		}

		// parse query
		ret = ParseQueryString();
	}

	return ret;
}

bool HttpRequest::ParseQueryString(void)
{
	bool ret = true;

	vector<string> v;
	Split(v, m_rawQuery, "&");

	vector<string>::const_iterator it;
	for (it = v.begin(); it != v.end(); it++)
	{
		vector<string> v1;
		Split(v1, *it, "=");

		if (v1.size() == 2 && !v1[0].empty())
		{
			// TODO: add %xx decode
			m_query[v1[0]] = v1[1];
		}
	}

	return ret;
}

void HttpRequest::SetQueryParam(const std::string name, std::string value)
{
	m_query[name] = value;
}

bool HttpRequest::GetQueryParam(const std::string name, std::string &value)
{
	bool ret = false;
	map<string, string>::iterator it;

	it = m_query.find(name);

	if (it != m_query.end())
	{
		ret = true;
		value = m_query[name];
	}

	return ret;
}

string HttpRequest::GetURI(void)
{
	return m_uri;
}
