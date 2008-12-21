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
** File: jsonutil.cpp
**
**/

#include "nxhttpd.h"


//
// Escape string
//

static void EscapeStringForJSON(String &str)
{
	int i;
	TCHAR chr[2], buffer[16];
   
	str.EscapeCharacter(_T('\\'), _T('\\'));
   str.EscapeCharacter(_T('"'), _T('\\'));
	
   str.Translate(_T("\r"), _T("\\r"));
   str.Translate(_T("\n"), _T("\\n"));
   str.Translate(_T("\t"), _T("\\t"));

	chr[1] = 0;
	for(i = 1; i < ' '; i++)
	{
		_sntprintf(buffer, 16, _T("\\u%04X"), i);
		chr[0] = i;
		str.Translate(chr, buffer);
	}
}


//
// Constructor
//

JSONObjectBuilder::JSONObjectBuilder()
{
	m_data = _T("{");
	m_level = 1;
	m_tabSize = 2;
	m_isFirst = TRUE;
}


//
// Destructor
//

JSONObjectBuilder::~JSONObjectBuilder()
{
}


//
// Process element start
//

void JSONObjectBuilder::StartElement()
{
	if (m_isFirst)
	{
		m_data += _T("\r\n");
		m_isFirst = FALSE;
	}
	else
	{
		m_data += _T(",\r\n");
	}
}


//
// Start array
//

void JSONObjectBuilder::StartArray(const TCHAR *name)
{
	StartElement();
	if (name != NULL)
		m_data.AddFormattedString(_T("%*s\"%s\": ["), m_level * m_tabSize, _T(""), name);
	else
		m_data.AddFormattedString(_T("%*s["), m_level * m_tabSize, _T(""));
	NextLevel();
}


//
// End array
//

void JSONObjectBuilder::EndArray()
{
	PrevLevel();
	m_data.AddFormattedString(_T("\r\n%*s]"), m_level * m_tabSize, _T(""));
}


//
// Start object
//

void JSONObjectBuilder::StartObject(const TCHAR *name)
{
	StartElement();
	if (name != NULL)
		m_data.AddFormattedString(_T("%*s\"%s\": {"), m_level * m_tabSize, _T(""), name);
	else
		m_data.AddFormattedString(_T("%*s{"), m_level * m_tabSize, _T(""));
	NextLevel();
}


//
// End object
//

void JSONObjectBuilder::EndObject()
{
	PrevLevel();
	m_data.AddFormattedString(_T("\r\n%*s}"), m_level * m_tabSize, _T(""));
}


//
// Add string
//

void JSONObjectBuilder::AddString(const TCHAR *name, const TCHAR *value)
{
	String temp;

	StartElement();
	if (name != NULL)
		m_data.AddFormattedString(_T("%*s\"%s\": \""), m_level * m_tabSize, _T(""), name);
	else
		m_data.AddFormattedString(_T("%*s\""), m_level * m_tabSize, _T(""));
	temp = value;
	EscapeStringForJSON(temp);
	m_data += (const TCHAR *)temp;
	m_data += _T("\"");
}


//
// Add integer
//

void JSONObjectBuilder::AddInt32(const TCHAR *name, int value)
{
	StartElement();
	if (name != NULL)
		m_data.AddFormattedString(_T("%*s\"%s\": %d"), m_level * m_tabSize, _T(""), name, value);
	else
		m_data.AddFormattedString(_T("%*s%d"), m_level * m_tabSize, _T(""), value);
}


//
// Add unsigned integer
//

void JSONObjectBuilder::AddUInt32(const TCHAR *name, DWORD value)
{
	StartElement();
	if (name != NULL)
		m_data.AddFormattedString(_T("%*s\"%s\": %u"), m_level * m_tabSize, _T(""), name, value);
	else
		m_data.AddFormattedString(_T("%*s%u"), m_level * m_tabSize, _T(""), value);
}


//
// Add 64bit unsined integer
//

void JSONObjectBuilder::AddUInt64(const TCHAR *name, QWORD value)
{
	StartElement();
	if (name != NULL)
		m_data.AddFormattedString(_T("%*s\"%s\": ") UINT64_FMT, m_level * m_tabSize, _T(""), name, value);
	else
		m_data.AddFormattedString(_T("%*s") UINT64_FMT, m_level * m_tabSize, _T(""), value);
}


//
// Add GUID
//

void JSONObjectBuilder::AddGUID(const TCHAR *name, uuid_t guid)
{
	TCHAR buffer[256];

	AddString(name, uuid_to_string(guid, buffer));
}


//
// Add RCC
//

void JSONObjectBuilder::AddRCC(DWORD rcc)
{
	AddUInt32(_T("rcc"), rcc);
	if (rcc != RCC_SUCCESS)
		AddString(_T("errorText"), NXCGetErrorText(rcc));
}


//
// Create output (finishes object - no further adds possible)
//

const TCHAR *JSONObjectBuilder::CreateOutput()
{
	if (m_level > 1)
		return NULL;	// Object is not properly closed

	m_data += _T("\r\n}\r\n");
	return m_data;
}
