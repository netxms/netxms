/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: parser.cpp
**
**/

#include "libnxlp.h"
#include <expat.h>

/**
 * Context state texts
 */
static const TCHAR *s_states[] = { _T("MANUAL"), _T("AUTO"), _T("INACTIVE") };

/**
 * XML parser state for creating LogParser object from XML
 */
#define XML_STATE_INIT        -1
#define XML_STATE_END         -2
#define XML_STATE_ERROR       -255
#define XML_STATE_PARSER      0
#define XML_STATE_RULES       1
#define XML_STATE_RULE        2
#define XML_STATE_MATCH       3
#define XML_STATE_EVENT       4
#define XML_STATE_FILE        5
#define XML_STATE_ID          6
#define XML_STATE_LEVEL       7
#define XML_STATE_SOURCE      8
#define XML_STATE_CONTEXT     9
#define XML_STATE_MACROS      10
#define XML_STATE_MACRO       11
#define XML_STATE_DESCRIPTION 12

struct XML_PARSER_STATE
{
	LogParser *parser;
	int state;
	String regexp;
	String event;
	String file;
	StringList files;
	ObjectArray<int> encodings;
	String id;
	String level;
	String source;
	String context;
	String description;
	int contextAction;
	String ruleContext;
	int numEventParams;
	String errorText;
	String macroName;
	String macro;
	bool invertedRule;
	bool breakFlag;
	XML_PARSER_STATE(): encodings(1, 1, true) {}
};

/**
 * Parser default constructor
 */
LogParser::LogParser()
{
	m_numRules = 0;
	m_rules = NULL;
	m_cb = NULL;
	m_userArg = NULL;
	m_name = NULL;
	m_fileName = NULL;
	m_fileEncoding = LP_FCP_ACP;
	m_eventNameList = NULL;
	m_eventResolver = NULL;
	m_thread = INVALID_THREAD_HANDLE;
	m_recordsProcessed = 0;
	m_recordsMatched = 0;
	m_processAllRules = FALSE;
	m_traceLevel = 0;
	m_traceCallback = NULL;
	_tcscpy(m_status, LPS_INIT);
}

/**
 * Parser copy constructor
 */
LogParser::LogParser(LogParser *src)
{
	m_numRules = src->m_numRules;
	m_rules = (LogParserRule **)malloc(sizeof(LogParserRule *) * m_numRules);
	for(int i = 0; i < m_numRules; i++)
		m_rules[i] = new LogParserRule(src->m_rules[i], this);

	m_cb = src->m_cb;
	m_userArg = src->m_userArg;
	m_name = (src->m_name != NULL) ? _tcsdup(src->m_name) : NULL;
	m_fileName = (src->m_fileName != NULL) ? _tcsdup(src->m_fileName) : NULL;
	m_fileEncoding = src->m_fileEncoding;

	if (src->m_eventNameList != NULL)
	{
		int count;
		for(count = 0; src->m_eventNameList[count].text != NULL; count++);
		m_eventNameList = (count > 0) ? (CODE_TO_TEXT *)nx_memdup(src->m_eventNameList, sizeof(CODE_TO_TEXT) * (count + 1)) : NULL;
	}
	else
	{
		m_eventNameList = NULL;
	}

	m_eventResolver = src->m_eventResolver;
	m_thread = INVALID_THREAD_HANDLE;
	m_recordsProcessed = 0;
	m_recordsMatched = 0;
	m_processAllRules = src->m_processAllRules;
	m_traceLevel = src->m_traceLevel;
	m_traceCallback = src->m_traceCallback;
	_tcscpy(m_status, LPS_INIT);
}

/**
 * Destructor
 */
LogParser::~LogParser()
{
	int i;

	for(i = 0; i < m_numRules; i++)
		delete m_rules[i];
	safe_free(m_rules);
	safe_free(m_name);
	safe_free(m_fileName);
}

/**
 * Trace
 */
void LogParser::trace(int level, const TCHAR *format, ...)
{
	va_list args;

	if ((m_traceCallback == NULL) || (level > m_traceLevel))
		return;

	va_start(args, format);
	m_traceCallback(format, args);
	va_end(args);
}


//
// Add rule
//

bool LogParser::addRule(LogParserRule *rule)
{
	bool isOK;

	isOK = rule->isValid();
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

bool LogParser::addRule(const TCHAR *regexp, DWORD eventCode, const TCHAR *eventName, int numParams)
{
	return addRule(new LogParserRule(this, regexp, eventCode, eventName, numParams));
}


//
// Check context
//

const TCHAR *LogParser::checkContext(LogParserRule *rule)
{
	const TCHAR *state;
	
	if (rule->getContext() == NULL)
	{
		trace(5, _T("  rule has no context"));
		return s_states[CONTEXT_SET_MANUAL];
	}
		
	state = m_contexts.get(rule->getContext());
	if (state == NULL)
	{
		trace(5, _T("  context '%s' inactive, rule should be skipped"), rule->getContext());
		return NULL;	// Context inactive, don't use this rule		
	}
	
	if (!_tcscmp(state, s_states[CONTEXT_CLEAR]))
	{
		trace(5, _T("  context '%s' inactive, rule should be skipped"), rule->getContext());
		return NULL;
	}
	else
	{
		trace(5, _T("  context '%s' active (mode=%s)"), rule->getContext(), state);
		return state;
	}
}

/**
 * Match log record
 */
bool LogParser::matchLogRecord(bool hasAttributes, const TCHAR *source, DWORD eventId,
										 DWORD level, const TCHAR *line, DWORD objectId)
{
	int i;
	const TCHAR *state;
	bool matched = false;

	if (hasAttributes)
		trace(5, _T("Match event: source=\"%s\" id=%u level=%d text=\"%s\""), source, eventId, level, line);
	else
		trace(5, _T("Match line: \"%s\""), line);

	m_recordsProcessed++;
	for(i = 0; i < m_numRules; i++)
	{
		trace(6, _T("checking rule %d \"%s\""), i + 1, m_rules[i]->getDescription());
		if ((state = checkContext(m_rules[i])) != NULL)
		{
			bool ruleMatched = hasAttributes ?
				m_rules[i]->matchEx(source, eventId, level, line, m_cb, objectId, m_userArg) : 
				m_rules[i]->match(line, m_cb, objectId, m_userArg);
			if (ruleMatched)
			{
				trace(5, _T("rule %d \"%s\" matched"), i + 1, m_rules[i]->getDescription());
				if (!matched)
					m_recordsMatched++;
				
				// Update context
				if (m_rules[i]->getContextToChange() != NULL)
				{
					m_contexts.set(m_rules[i]->getContextToChange(), s_states[m_rules[i]->getContextAction()]);
					trace(5, _T("rule %d \"%s\": context %s set to %s"), i + 1, m_rules[i]->getDescription(), m_rules[i]->getContextToChange(), s_states[m_rules[i]->getContextAction()]);
				}
				
				// Set context of this rule to inactive if rule context mode is "automatic reset"
				if (!_tcscmp(state, s_states[CONTEXT_SET_AUTOMATIC]))
				{
					m_contexts.set(m_rules[i]->getContext(), s_states[CONTEXT_CLEAR]);
					trace(5, _T("rule %d \"%s\": context %s cleared because it was set to automatic reset mode"),
							i + 1, m_rules[i]->getDescription(), m_rules[i]->getContext());
				}
				matched = true;
				if (!m_processAllRules || m_rules[i]->getBreakFlag())
					break;
			}
		}
	}
	if (i < m_numRules)
		trace(5, _T("processing stopped at rule %d \"%s\"; result = %s"), i + 1,
				m_rules[i]->getDescription(), matched ? _T("true") : _T("false"));
	else
		trace(5, _T("Processing stopped at end of rules list; result = %s"), matched ? _T("true") : _T("false"));
	return matched;
}

/**
 * Match simple log line
 */
bool LogParser::matchLine(const TCHAR *line, DWORD objectId)
{
	return matchLogRecord(false, NULL, 0, 0, line, objectId);
}

/**
 * Match log event (text with additional attributes - source, severity, event id)
 */
bool LogParser::matchEvent(const TCHAR *source, DWORD eventId, DWORD level, const TCHAR *line, DWORD objectId)
{
	return matchLogRecord(true, source, eventId, level, line, objectId);
}

/**
 * Set associated file name
 */
void LogParser::setFileName(const TCHAR *name)
{
	safe_free(m_fileName);
	m_fileName = (name != NULL) ? _tcsdup(name) : NULL;
	if (m_name == NULL)
		m_name = _tcsdup(name);	// Set parser name to file name
}


//
// Set parser name
//

void LogParser::setName(const TCHAR *name)
{
	safe_free(m_name);
	m_name = _tcsdup((name != NULL) ? name : CHECK_NULL(m_fileName));
}


//
// Add macro
//

void LogParser::addMacro(const TCHAR *name, const TCHAR *value)
{
	m_macros.set(name, value);
}

/**
 * Get macro
 */
const TCHAR *LogParser::getMacro(const TCHAR *name)
{
	const TCHAR *value;

	value = m_macros.get(name);
	return CHECK_NULL_EX(value);
}

/**
 * Create parser configuration from XML - callback for element start
 */
static void StartElement(void *userData, const char *name, const char **attrs)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	if (!strcmp(name, "parser"))
	{
		ps->state = XML_STATE_PARSER;
		ps->parser->setProcessAllFlag(XMLGetAttrBoolean(attrs, "processAll", false));
		ps->parser->setTraceLevel(XMLGetAttrInt(attrs, "trace", 0));
		const char *name = XMLGetAttr(attrs, "name");
		if (name != NULL)
		{
#ifdef UNICODE
			WCHAR *wname = WideStringFromUTF8String(name);
			ps->parser->setName(wname);
			free(wname);
#else
			ps->parser->setName(name);
#endif
		}
	}
	else if (!strcmp(name, "file"))
	{
		ps->state = XML_STATE_FILE;
		const char *encoding = XMLGetAttr(attrs, "encoding");
		if (encoding != NULL)
		{
			if (!stricmp(encoding, "acp") || (*encoding == 0))
			{
				ps->encodings.add(new int(LP_FCP_ACP));
			}
			else if (!stricmp(encoding, "utf8") || !stricmp(encoding, "utf-8"))
			{
				ps->encodings.add(new int(LP_FCP_UTF8));
			}
			else if (!stricmp(encoding, "ucs2") || !stricmp(encoding, "ucs-2") || !stricmp(encoding, "utf-16"))
			{
				ps->encodings.add(new int(LP_FCP_UCS2));
			}
			else if (!stricmp(encoding, "ucs4") || !stricmp(encoding, "ucs-4") || !stricmp(encoding, "utf-32"))
			{
				ps->encodings.add(new int(LP_FCP_UCS4));
			}
			else
			{
				ps->errorText = _T("Invalid file encoding");
				ps->state = XML_STATE_ERROR;
			}
		}
		else
		{
			ps->encodings.add(new int(LP_FCP_ACP));
		}
	}
	else if (!strcmp(name, "macros"))
	{
		ps->state = XML_STATE_MACROS;
	}
	else if (!strcmp(name, "macro"))
	{
		const char *name;

		ps->state = XML_STATE_MACRO;
		name = XMLGetAttr(attrs, "name");
#ifdef UNICODE
		ps->macroName = L"";
		ps->macroName.addMultiByteString(name, (DWORD)strlen(name), CP_UTF8);
#else
		ps->macroName = CHECK_NULL_A(name);
#endif
		ps->macro = NULL;
	}
	else if (!strcmp(name, "rules"))
	{
		ps->state = XML_STATE_RULES;
	}
	else if (!strcmp(name, "rule"))
	{
		ps->regexp = NULL;
		ps->invertedRule = false;
		ps->event = NULL;
		ps->context = NULL;
		ps->contextAction = CONTEXT_SET_AUTOMATIC;
		ps->description = NULL;
		ps->id = NULL;
		ps->source = NULL;
		ps->level = NULL;
#ifdef UNICODE
		ps->ruleContext = L"";
		const char *context = XMLGetAttr(attrs, "context");
		if (context != NULL)
			ps->ruleContext.addMultiByteString(context, (DWORD)strlen(context), CP_UTF8);
#else
		ps->ruleContext = XMLGetAttr(attrs, "context");
#endif
		ps->breakFlag = XMLGetAttrBoolean(attrs, "break", false);
		ps->state = XML_STATE_RULE;
		ps->numEventParams = 0;
	}
	else if (!strcmp(name, "match"))
	{
		ps->state = XML_STATE_MATCH;
		ps->invertedRule = XMLGetAttrBoolean(attrs, "invert", false);
	}
	else if (!strcmp(name, "id") || !strcmp(name, "facility"))
	{
		ps->state = XML_STATE_ID;
	}
	else if (!strcmp(name, "level") || !strcmp(name, "severity"))
	{
		ps->state = XML_STATE_LEVEL;
	}
	else if (!strcmp(name, "source") || !strcmp(name, "tag"))
	{
		ps->state = XML_STATE_SOURCE;
	}
	else if (!strcmp(name, "event"))
	{
		ps->numEventParams = XMLGetAttrDWORD(attrs, "params", 0);
		ps->state = XML_STATE_EVENT;
	}
	else if (!strcmp(name, "context"))
	{
		const char *action;
		
		ps->state = XML_STATE_CONTEXT;
		
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
				ps->contextAction = CONTEXT_SET_AUTOMATIC;
			}			
			else if (!strcmp(mode, "manual"))
			{
				ps->contextAction = CONTEXT_SET_MANUAL;
			}			
			else
			{
				ps->errorText = _T("Invalid context reset mode");
				ps->state = XML_STATE_ERROR;
			}
		}
		else if (!strcmp(action, "clear"))
		{
			ps->contextAction = CONTEXT_CLEAR;
		}
		else
		{
			ps->errorText = _T("Invalid context action");
			ps->state = XML_STATE_ERROR;
		}
	}
	else if (!strcmp(name, "description"))
	{
		ps->state = XML_STATE_DESCRIPTION;
	}
	else
	{
		ps->state = XML_STATE_ERROR;
	}
}

/**
 * Create parser configuration from XML - callback for element end
 */
static void EndElement(void *userData, const char *name)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	if (!strcmp(name, "parser"))
	{
		ps->state = XML_STATE_END;
	}
	else if (!strcmp(name, "file"))
	{
		ps->files.add(ps->file);
		ps->file = _T("");
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "macros"))
	{
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "macro"))
	{
		ps->parser->addMacro(ps->macroName, ps->macro);
		ps->state = XML_STATE_MACROS;
	}
	else if (!strcmp(name, "rules"))
	{
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "rule"))
	{
		DWORD eventCode;
		const TCHAR *eventName = NULL;
		TCHAR *eptr;
		LogParserRule *rule;

		ps->event.trim();
		eventCode = _tcstoul(ps->event, &eptr, 0);
		if (*eptr != 0)
		{
			eventCode = ps->parser->resolveEventName(ps->event, 0);
			if (eventCode == 0)
			{
				eventName = (const TCHAR *)ps->event;
			}
		}
		if (ps->regexp.isEmpty())
			ps->regexp = _T(".*");
		rule = new LogParserRule(ps->parser, (const TCHAR *)ps->regexp, eventCode, eventName, ps->numEventParams);
		if (!ps->ruleContext.isEmpty())
			rule->setContext(ps->ruleContext);
		if (!ps->context.isEmpty())
		{
			rule->setContextToChange(ps->context);
			rule->setContextAction(ps->contextAction);
		}

		if (!ps->description.isEmpty())
			rule->setDescription(ps->description);
		
		if (!ps->source.isEmpty())
			rule->setSource(ps->source);

		if (!ps->level.isEmpty())
			rule->setLevel(_tcstoul(ps->level, NULL, 0));

		if (!ps->id.isEmpty())
		{
			DWORD start, end;
			TCHAR *eptr;

			start = _tcstoul(ps->id, &eptr, 0);
			if (*eptr == 0)
			{
				end = start;
			}
			else	/* TODO: add better error handling */
			{
				while(!_istdigit(*eptr))
					eptr++;
				end = _tcstoul(eptr, NULL, 0);
			}
			rule->setIdRange(start, end);
		}

		rule->setInverted(ps->invertedRule);
		rule->setBreakFlag(ps->breakFlag);

		ps->parser->addRule(rule);
		ps->state = XML_STATE_RULES;
	}
	else if (!strcmp(name, "match"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "id") || !strcmp(name, "facility"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "level") || !strcmp(name, "severity"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "source") || !strcmp(name, "tag"))
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
	else if (!strcmp(name, "description"))
	{
		ps->state = XML_STATE_RULE;
	}
}

/**
 * Create parser configuration from XML - callback for element data
 */
static void CharData(void *userData, const XML_Char *s, int len)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	switch(ps->state)
	{
		case XML_STATE_MATCH:
			ps->regexp.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_ID:
			ps->id.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_LEVEL:
			ps->level.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_SOURCE:
			ps->source.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_EVENT:
			ps->event.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_FILE:
			ps->file.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_CONTEXT:
			ps->context.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_DESCRIPTION:
			ps->description.addMultiByteString(s, len, CP_UTF8);
			break;
		case XML_STATE_MACRO:
			ps->macro.addMultiByteString(s, len, CP_UTF8);
			break;
		default:
			break;
	}
}

/**
 * Create parser configuration from XML. Returns array of identical parsers,
 * one for each <file> tag in source XML. Resulting parsers only differs with file names.
 */
ObjectArray<LogParser> *LogParser::createFromXml(const char *xml, int xmlLen, TCHAR *errorText, int errBufSize, bool (*eventResolver)(const TCHAR *, DWORD *))
{
	ObjectArray<LogParser> *parsers = NULL;
	bool success;

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_PARSER_STATE state;
	state.parser = new LogParser;
	state.parser->setEventNameResolver(eventResolver);
	state.state = -1;
	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, StartElement, EndElement);
	XML_SetCharacterDataHandler(parser, CharData);
	success = (XML_Parse(parser, xml, (xmlLen == -1) ? (int)strlen(xml) : xmlLen, TRUE) != XML_STATUS_ERROR);
	if (!success && (errorText != NULL))
	{
		_sntprintf(errorText, errBufSize, _T("%hs at line %d"),
			XML_ErrorString(XML_GetErrorCode(parser)),
			(int)XML_GetCurrentLineNumber(parser));
	}
	XML_ParserFree(parser);
	if (success && (state.state == XML_STATE_ERROR))
	{
		success = false;
		if (errorText != NULL)
			nx_strncpy(errorText, state.errorText, errBufSize);
	}
	else if (success)
	{ 
		parsers = new ObjectArray<LogParser>;
		if (state.files.getSize() > 0)
		{
			for(int i = 0; i < state.files.getSize(); i++)
			{
				LogParser *p = (i > 0) ? new LogParser(state.parser) : state.parser;
				p->setFileName(state.files.getValue(i));
				p->setFileEncoding(*state.encodings.get(i));
				parsers->add(p);
			}
		}
		else
		{
			// It is possible to have parser without <file> tag, syslog parser for example
			parsers->add(state.parser);
		}
	}

	return parsers;
}

/**
 * Resolve event name
 */
DWORD LogParser::resolveEventName(const TCHAR *name, DWORD defVal)
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
