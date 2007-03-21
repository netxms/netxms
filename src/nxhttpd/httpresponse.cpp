/* $Id: httpresponse.cpp,v 1.1 2007-03-21 10:15:18 alk Exp $ */

#pragma warning(disable: 4786)

#include "nxhttpd.h"
#include <libtpt/tpt.h>
#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace std;

//////////////////////////////////////////////////////////////////////////

HttpResponse::HttpResponse()
{
	m_code = HTTP_INTERNALSERVERERROR;
	m_body = NULL;
	m_bodyLen = 0;
	m_headers.clear();
}

HttpResponse::~HttpResponse()
{
	if (m_body != NULL)
	{
		free(m_body);
		m_body = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////

void HttpResponse::SetHeader(const string name, const string value)
{
	m_headers[name] = value;
}

void HttpResponse::SetType(const string type)
{
	m_headers["Content-type"] = type;
}

void HttpResponse::SetBody(std::string data, bool bAppend)
{
	SetBody(data.c_str(), data.size(), bAppend);
}

void HttpResponse::SetBody(const char *data, int size, bool bAppend)
{
	// FIXME: add validation
	if (m_body != NULL && bAppend == false)
	{
		free(m_body);
		m_body = NULL;
	}

	if (data != NULL)
	{
		int offset;

		if (size == 0)
		{
			size = strlen(data);
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

		char tmp[64];
		snprintf(tmp, sizeof(tmp), "%lu", m_bodyLen);
		m_headers["Content-Lenght"] = tmp;
	}
	else
	{
		m_bodyLen = 0;
		m_headers["Content-Lenght"] = "0";
	}
}

void HttpResponse::SetCode(int code)
{
	char *text;

	switch (code)
	{
		case HTTP_OK:
			text = "200 OK";
			break;
		case HTTP_MOVEDPERMANENTLY:
			text = "301 Moved";
			break;
		case HTTP_FOUND:
			text = "302 Found";
			break;
		case HTTP_BADREQUEST:
			text = "400 Bad Request";
			break;
		case HTTP_UNAUTHORIZED:
			text = "401 Unauthorized";
			break;
		case HTTP_FORBIDDEN:
			text = "403 Forbidden";
			break;
		case HTTP_NOTFOUND:
			text = "404 Not Found";
			break;
		default:
			code = 500;
			text = "500 Internal server error";
			break;
	}

	m_code = code;
	m_codeString = text;
}

string HttpResponse::GetCodeString(void)
{
	return m_codeString;
}

//////////////////////////////////////////////////////////////////////////

char *HttpResponse::BuildStream(int &size)
{
	string tmp;
	char *out;

	// FIXME: hack
	SetHeader("Pragma", "no-cache");
	SetHeader("Cache-Control", "max-age=1, must-revalidate");
	SetHeader("Expires", "Fri, 30 Oct 1998 14:19:41 GMT");

	tmp = string("HTTP/1.0 ") + m_codeString + string("\r\n");

	map<string, string>::const_iterator it = m_headers.begin();
	for (; it != m_headers.end(); it++)
	{
		tmp += it->first + string(": ") + it->second + string("\r\n");
	}
	tmp += string("\r\n");

	// add validation!
	size = tmp.size() + m_bodyLen;
	out = (char *)malloc(size);

	memcpy(out, tmp.c_str(), tmp.size());
	if (m_bodyLen > 0 && m_body != NULL)
	{
		memcpy(out + tmp.size(), m_body, m_bodyLen);
	}

	return out;
}

void HttpResponse::CleanStream(char *ptr)
{
	if (ptr != NULL)
	{
		free(ptr);
	}
}

void HttpResponse::TptSet(string var, string value)
{
//	m_symbols.set(var, value);
}

void HttpResponse::TptPush(string array, string value)
{
//	m_symbols.push(var, value);
}

void HttpResponse::TptRender(string tplFile, string contentType)
{
	try
	{
		stringstream stream(stringstream::out);

		TPT::Buffer buffer(tplFile.c_str());
		if (buffer)
		{
			TPT::Parser p(buffer);
			p.addincludepath("/home/alk/work/nms/src/console/web/templates/");

			p.run(stream);

			TPT::ErrorList errlist;

			if (p.geterrorlist(errlist)) {
				std::cout << "Errors!" << std::endl;
				TPT::ErrorList::const_iterator it(errlist.begin()), end(errlist.end());
				for (; it != end; ++it)
					std::cout << (*it) << std::endl;
			}

			SetCode(HTTP_OK);
			SetType(contentType);
			SetBody(stream.str());
		}
		else
		{
			SetCode(HTTP_INTERNALSERVERERROR);
			SetType("text/plain");
			SetBody("Internal error (template not available)\nContact administrator.");
		}
	}
	catch (const exception& e)
	{
		cout << "EXCETPION: " << e.what() << endl;
	}
	catch (...)
	{
		cout << "UNKNOWN EXCEPTION" << endl;
	}
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
