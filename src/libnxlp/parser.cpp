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
// Context state texts
//

static const TCHAR *m_states[] = { _T("MANUAL"), _T("AUTO"), _T("INACTIVE") };


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
#define XML_STATE_ID       6
#define XML_STATE_LEVEL    7
#define XML_STATE_SOURCE   8
#define XML_STATE_CONTEXT  9

typedef struct
{
	LogParser *parser;
	int state;
	String regexp;
	String event;
	String file;
	String id;
	String level;
	String source;
	String context;
	int contextAction;
	String ruleContext;
	int numEventParams;
	String errorText;
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
	m_eventNameList = NULL;
	m_eventResolver = NULL;
	m_thread = INVALID_THREAD_HANDLE;
	m_recordsProcessed = 0;
	m_recordsMatched = 0;
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

BOOL LogParser::AddRule(LogParserRule *rule)
{
	BOOL isOK;

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

BOOL LogParser::AddRule(const char *regexp, DWORD event, int numParams)
{
	return AddRule(new LogParserRule(regexp, event, numParams));
}


//
// Check context
//

const TCHAR *LogParser::CheckContext(LogParserRule *rule)
{
	const TCHAR *state;
	
	if (rule->GetContext() == NULL)
		return m_states[CONTEXT_SET_MANUAL];
		
	state = m_contexts.Get(rule->GetContext());
	if (state == NULL)
		return NULL;	// Context inactive, don't use this rule		
	
	return !_tcscmp(state, m_states[CONTEXT_CLEAR]) ? NULL : state;
}


//
// Match log line
//

BOOL LogParser::MatchLine(const char *line, DWORD objectId)
{
	int i;
	const TCHAR *state;

	m_recordsProcessed++;
	for(i = 0; i < m_numRules; i++)
	{
		if (((state = CheckContext(m_rules[i])) != NULL) && m_rules[i]->Match(line, m_cb, objectId, m_userArg))
		{
			m_recordsMatched++;
			
			// Update context
			if (m_rules[i]->GetContextToChange() != NULL)
			{
				m_contexts.Set(m_rules[i]->GetContextToChange(), m_states[m_rules[i]->GetContextAction()]);
			}
			
			// Set context of this rule to inactive if rule context mode is "automatic reset"
			if (!_tcscmp(state, m_states[CONTEXT_SET_AUTOMATIC]))
			{
				m_contexts.Set(m_rules[i]->GetContext(), m_states[CONTEXT_CLEAR]);
			}
			return TRUE;
		}
	}
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
		((XML_PARSER_STATE *)userData)->regexp = NULL;
		((XML_PARSER_STATE *)userData)->event = NULL;
		((XML_PARSER_STATE *)userData)->context = NULL;
		((XML_PARSER_STATE *)userData)->contextAction = CONTEXT_SET_AUTOMATIC;
		((XML_PARSER_STATE *)userData)->ruleContext = XMLGetAttr(attrs, "context");
		((XML_PARSER_STATE *)userData)->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "match"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_MATCH;
	}
	else if (!strcmp(name, "id"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_ID;
	}
	else if (!strcmp(name, "level"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_LEVEL;
	}
	else if (!strcmp(name, "source"))
	{
		((XML_PARSER_STATE *)userData)->state = XML_STATE_SOURCE;
	}
	else if (!strcmp(name, "event"))
	{
		((XML_PARSER_STATE *)userData)->numEventParams = XMLGetAttrDWORD(attrs, "params", 0);
		((XML_PARSER_STATE *)userData)->state = XML_STATE_EVENT;
	}
	else if (!strcmp(name, "context"))
	{
		const char *action;
		
		((XML_PARSER_STATE *)userData)->state = XML_STATE_CONTEXT;
		
		action = XMLGetAttr(attrs, "action");
		if (action == NULL)
			action = "set";
			
		if (!strcmp(action, "set"))
		{
			const char *mode;

			mode = XMLGetAttr(attrs, "reset");
			if (mode == NULL)
				mode = "auto";

			if (!strcmp(mode, "auto"))
			{
				((XML_PARSER_STATE *)userData)->contextAction = CONTEXT_SET_AUTOMATIC;
			}			
			else if (!strcmp(mode, "manual"))
			{
				((XML_PARSER_STATE *)userData)->contextAction = CONTEXT_SET_MANUAL;
			}			
			else
			{
				((XML_PARSER_STATE *)userData)->errorText = _T("Invalid context reset mode");
				((XML_PARSER_STATE *)userData)->state = XML_STATE_ERROR;
			}
		}
		else if (!strcmp(action, "clear"))
		{
			((XML_PARSER_STATE *)userData)->contextAction = CONTEXT_CLEAR;
		}
		else
		{
			((XML_PARSER_STATE *)userData)->errorText = _T("Invalid context action");
			((XML_PARSER_STATE *)userData)->state = XML_STATE_ERROR;
		}
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
		LogParserRule *rule;

		ps->event.Strip();
		event = strtoul(ps->event, &eptr, 0);
		if (*eptr != 0)
			event = ps->parser->ResolveEventName(ps->event);
		rule = new LogParserRule((const char *)ps->regexp, event, ps->numEventParams);
		if (!ps->ruleContext.IsEmpty())
			rule->SetContext(ps->ruleContext);
		if (!ps->context.IsEmpty())
		{
			rule->SetContextToChange(ps->context);
			rule->SetContextAction(ps->contextAction);
		}
		ps->parser->AddRule(rule);
		ps->state = XML_STATE_RULES;
	}
	else if (!strcmp(name, "match"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "id"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "level"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "source"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "event"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "context"))
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
		case XML_STATE_ID:
			ps->id.AddString(s, len);
			break;
		case XML_STATE_LEVEL:
			ps->level.AddString(s, len);
			break;
		case XML_STATE_SOURCE:
			ps->source.AddString(s, len);
			break;
		case XML_STATE_EVENT:
			ps->event.AddString(s, len);
			break;
		case XML_STATE_FILE:
			ps->file.AddString(s, len);
			break;
		case XML_STATE_CONTEXT:
			ps->context.AddString(s, len);
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

	if (success && (state.state == XML_STATE_ERROR))
	{
		success = FALSE;
		if (errorText != NULL)
			strncpy(errorText, state.errorText, errBufSize);
	}
	
	return success;
#else

	if (errorText != NULL)
		strncpy(errorText, "Compiled without XML support", errBufSize);
	return FALSE;
#endif
}


//
// Resolve event name
//

DWORD LogParser::ResolveEventName(const TCHAR *name, DWORD defVal)
{
	if (m_eventNameList != NULL)
	{
		int i;

		for(i = 0; m_eventNameList[i].text != NULL; i++)
			if (!_tcsicmp(name, m_eventNameList[i].text))
				return m_eventNameList[i].code;
	}

	if (m_eventResolver != NULL)
	{
		DWORD val;

		if (m_eventResolver(name, &val))
			return val;
	}

	return defVal;
}
