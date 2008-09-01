/* $Id$ */
/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: parser.cpp
**
**/

#include "libnxlp.h"

#if HAVE_LIBEXPAT
#include <expat.h>
#endif


//
// XML parser state for creating LogParser object from XML
//

#define XML_STATE_INIT		-1
#define XML_STATE_END		-2
#define XML_STATE_ERROR    -255
#define XML_STATE_PARSER	0
#define XML_STATE_RULES		1
#define XML_STATE_RULE		2
#define XML_STATE_MATCH		3
#define XML_STATE_EVENT		4
#define XML_STATE_FILE     5

typedef struct
{
	LogParser *parser;
	int state;
	String regexp;
	String event;
	String file;
	int numEventParams;
} XML_PARSER_STATE;


//
// Parser default constructor
//

LogParser::LogParser()
{
	m_numRules = 0;
	m_rules = NULL;
	m_cb = NULL;
	m_userArg = NULL;
	m_fileName = NULL;
}


//
// Destructor
//

LogParser::~LogParser()
{
	int i;

	for(i = 0; i < m_numRules; i++)
		delete m_rules[i];
	safe_free(m_rules);
	safe_free(m_fileName);
}


//
// Add rule
//

BOOL LogParser::AddRule(const char *regexp, DWORD event, int numParams)
{
	LogParserRule *rule;
	BOOL isOK;

	rule = new LogParserRule(regexp, event, numParams);
	isOK = rule->IsValid();
	if (isOK)
	{
		m_rules = (LogParserRule **)realloc(m_rules, sizeof(LogParserRule *) * (m_numRules + 1));
		m_rules[m_numRules++] = rule;
	}
	else
	{
		delete rule;
	}
	return isOK;
}


//
// Match log line
//

BOOL LogParser::MatchLine(const char *line)
{
	int i;

	for(i = 0; i < m_numRules; i++)
		if (m_rules[i]->Match(line, m_cb, m_userArg))
			return TRUE;
	return FALSE;
}


//
// Set associated file name
//

void LogParser::SetFileName(const TCHAR *name)
{
	safe_free(m_fileName);
	m_fileName = (name != NULL) ? _tcsdup(name) : NULL;
}


//
// Create parser configuration from XML
//

#if HAVE_LIBEXPAT

static void StartElement(void *userData, const char *name, const char **attrs)
{
	if (!strcmp(name, "parser"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "file"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_FILE;
	}
	else if (!strcmp(name, "rules"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_RULES;
	}
	else if (!strcmp(name, "rule"))
	{
		((XML_PARSER_STATE *)userData)->regexp = _T("");
		((XML_PARSER_STATE *)userData)->event = _T("");
		((XML_PARSER_STATE *)userData)->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "match"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_MATCH;
	}
	else if (!strcmp(name, "event"))
	{
		((XML_PARSER_STATE *)userData)->numEventParams = XMLGetAttrDWORD(attrs, "params", 0);
		((XML_PARSER_STATE *)userData)->state = XML_STATE_EVENT;
	}
	else
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_ERROR;
	}
}

static void EndElement(void *userData, const char *name)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	if (!strcmp(name, "parser"))
	{
		ps->state = XML_STATE_END;
	}
	else if (!strcmp(name, "file"))
	{
		ps->parser->SetFileName(ps->file);
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "rules"))
	{
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "rule"))
	{
		DWORD event;
		char *eptr;

		event = strtoul(ps->event, &eptr, 0);
		ps->parser->AddRule((const char *)ps->regexp, event, ps->numEventParams);
		ps->state = XML_STATE_RULES;
	}
	else if (!strcmp(name, "match"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "event"))
	{
		ps->state = XML_STATE_RULE;
	}
}

static void CharData(void *userData, const XML_Char *s, int len)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	switch(ps->state)
	{
		case XML_STATE_MATCH:
			ps->regexp.AddString(s, len);
			break;
		case XML_STATE_EVENT:
			ps->event.AddString(s, len);
			break;
		case XML_STATE_FILE:
			ps->file.AddString(s, len);
			break;
		default:
			break;
	}
}

#endif

BOOL LogParser::CreateFromXML(const char *xml, int xmlLen, char *errorText, int errBufSize)
{
#if HAVE_LIBEXPAT
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_PARSER_STATE state;
	BOOL success;

	// Parse XML
	state.parser = this;
	state.state = -1;
	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, StartElement, EndElement);
	XML_SetCharacterDataHandler(parser, CharData);
	success = (XML_Parse(parser, xml, (xmlLen == -1) ? strlen(xml) : xmlLen, TRUE) != XML_STATUS_ERROR);

	if (!success && (errorText != NULL))
	{
		snprintf(errorText, errBufSize, "%s at line %d",
               XML_ErrorString(XML_GetErrorCode(parser)),
               XML_GetCurrentLineNumber(parser));
	}
	XML_ParserFree(parser);

	return success;
#else

	if (errorText != NULL)
		strncpy(errorText, "Compiled without XML support", errBufSize);
	return FALSE;
#endif
}
