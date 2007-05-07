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

void HttpResponse::SetBody(TCHAR *data, int size, BOOL bAppend)
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
			size = _tcslen(data);
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

	// FIXME: hack
	SetHeader(_T("Pragma"), _T("no-cache"));
	SetHeader(_T("Cache-Control"), _T("max-age=1, must-revalidate"));
	SetHeader(_T("Expires"), _T("Fri, 30 Oct 1998 14:19:41 GMT"));

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
	memcpy(out, (TCHAR *)tmp, tmp.Size());
	if ((m_bodyLen > 0) && (m_body != NULL))
		memcpy(out + tmp.Size(), m_body, m_bodyLen);
#endif

	return out;
}


//
// Start HTML page
//

void HttpResponse::BeginPage(TCHAR *pszTitle)
{
	SetType(_T("text/html"));
	SetBody(_T("<html><head><title>"));
	SetBody(pszTitle != NULL ? pszTitle : _T("NetXMS Web Interface"), -1, TRUE);
	SetBody(_T("</title>\r\n")
		     _T("<link rel=\"stylesheet\" type=\"text/css\" href=\"/netxms.css\" media=\"screen, tv, projection\" title=\"Default\" />\r\n")
			  _T("</head><body>"), -1, TRUE);
}
