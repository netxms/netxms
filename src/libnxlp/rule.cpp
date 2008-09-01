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
** File: rule.cpp
**
**/

#include "libnxlp.h"

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif


//
// Constructor
//

LogParserRule::LogParserRule(const char *regexp, DWORD event, int numParams)
{
	m_isValid = (regcomp(&m_preg, regexp, REG_ICASE) == 0);
	m_event = event;
	m_numParams = numParams;
	m_pmatch = (numParams > 0) ? (regmatch_t *)malloc(sizeof(regmatch_t) * numParams) : NULL;
}


//
// Destructor
//

LogParserRule::~LogParserRule()
{
	if (m_isValid)
		regfree(&m_preg);
	safe_free(m_pmatch);
}


//
// Match line
//

BOOL LogParserRule::Match(const char *line, LOG_PARSER_CALLBACK cb, void *userArg)
{
	if (!m_isValid)
		return FALSE;

	if (regexec(&m_preg, line, m_numParams, m_pmatch, 0) == 0)
	{
		if (cb != NULL)
		{
			char **params;
			int i, len;

			if (m_numParams > 0)
			{
				params = (char **)alloca(sizeof(char *) * m_numParams);
				for(i = 0; i < m_numParams; i++)
				{
					if (m_pmatch[i].rm_so != -1)
					{
						len = m_pmatch[i].rm_eo - m_pmatch[i].rm_so;
						params[i] = (char *)malloc(len + 1);
						memcpy(params[i], &line[m_pmatch[i].rm_so], len);
						params[i][len] = 0;
					}
					else
					{
						params[i] = strdup("");
					}
				}
			}

			cb(m_event, line, m_numParams, params, userArg);
			
			for(i = 0; i < m_numParams; i++)
				safe_free(params[i]);
		}		
		return TRUE;
	}

	return FALSE;	// no match
}
