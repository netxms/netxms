/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: xml.cpp
**
**/

#include "libnetxms.h"

/**
 * Encode string for XML
 */
TCHAR LIBNETXMS_EXPORTABLE *EscapeStringForXML(const TCHAR *str, int length)
{
	TCHAR *out;
	const TCHAR *in;
	int inLen, outLen, pos;

	// Calculate length
	inLen = (length == -1) ? (int)_tcslen(str) : length;
	for(in = str, outLen = 0; (inLen > 0) && (*in != 0); in++, outLen++, inLen--)
		if ((*in == _T('&')) || (*in == _T('<')) ||
		    (*in == _T('>')) || (*in == _T('"')) ||
			 (*in == _T('\'')) || (*in < 32))
			outLen += 5;
	outLen++;
	
	// Convert string
	out = (TCHAR *)malloc(outLen * sizeof(TCHAR));
	inLen = (length == -1) ? (int)_tcslen(str) : length;
	for(in = str, pos = 0; inLen > 0; in++, inLen--)
	{
		switch(*in)
		{
			case _T('&'):
				_tcscpy(&out[pos], _T("&amp;"));
				pos += 5;
				break;
			case _T('<'):
				_tcscpy(&out[pos], _T("&lt;"));
				pos += 4;
				break;
			case _T('>'):
				_tcscpy(&out[pos], _T("&gt;"));
				pos += 4;
				break;
			case _T('"'):
				_tcscpy(&out[pos], _T("&quot;"));
				pos += 6;
				break;
			case _T('\''):
				_tcscpy(&out[pos], _T("&apos;"));
				pos += 6;
				break;
			default:
				if (*in < 32)
				{
					_sntprintf(&out[pos], 8, _T("&#x%02X;"), *in);
					pos += 6;
				}
				else
				{
					out[pos++] = *in;
				}
				break;
		}
	}
	out[pos] = 0;

	return out;
}

/**
 * Escape string for XML - return escaped string as String object
 */
String LIBNETXMS_EXPORTABLE EscapeStringForXML2(const TCHAR *str, int length)
{
	String s;
	s.setBuffer(EscapeStringForXML(str, length));
	return s;
}

/**
 * Get attribute for XML tag
 */
const char LIBNETXMS_EXPORTABLE *XMLGetAttr(const char **attrs, const char *name)
{
	int i;

	for(i = 0; attrs[i] != NULL; i += 2)
	{
		if (!stricmp(attrs[i], name))
			return attrs[i + 1];
	}
	return NULL;
}

int LIBNETXMS_EXPORTABLE XMLGetAttrInt(const char **attrs, const char *name, int defVal)
{
	const char *value;

	value = XMLGetAttr(attrs, name);
	return (value != NULL) ? strtol(value, NULL, 0) : defVal;
}

DWORD LIBNETXMS_EXPORTABLE XMLGetAttrDWORD(const char **attrs, const char *name, DWORD defVal)
{
	const char *value;

	value = XMLGetAttr(attrs, name);
	return (value != NULL) ? strtoul(value, NULL, 0) : defVal;
}

bool LIBNETXMS_EXPORTABLE XMLGetAttrBoolean(const char **attrs, const char *name, bool defVal)
{
	const char *value;
	int ival;
	char *eptr;

	value = XMLGetAttr(attrs, name);
	if (value != NULL)
	{
		ival = strtol(value, &eptr, 0);
		if (*eptr == 0)
			return (ival != 0) ? true : false;
		return !stricmp(value, "yes") || !stricmp(value, "true");
	}
	return defVal;
}
