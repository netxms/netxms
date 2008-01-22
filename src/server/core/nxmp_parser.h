/* $Id: nxmp_parser.h,v 1.11 2008-01-22 19:21:10 victor Exp $ */
/* 
** NetXMS - Network Management System
** NetXMS Server
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: nxmp_parser.h
**
**/

#ifndef _nxmp_parser_h
#define _nxmp_parser_h

#include "nxmp_parser.tab.hpp"

typedef void *yyscan_t;


//
// Constants
//

#define MAX_STRING_SIZE    4096

#define CTX_NONE           0
#define CTX_EVENT          1
#define CTX_DCI            2
#define CTX_THRESHOLD      3
#define CTX_TRAP				4
#define CTX_TEMPLATE			5

#define NXMP_SETEVENT_TRAP							0
#define NXMP_SETEVENT_THRESHOLD_ACTIVATE		1
#define NXMP_SETEVENT_THRESHOLD_DEACTIVATE	2


//
// Management pack's data
//

class NXMP_Lexer;
class NXMP_Parser;

class NXMP_Data
{
private:
   EVENT_TEMPLATE *m_pEventList;
   EVENT_TEMPLATE *m_pCurrEvent;
   DWORD m_dwNumEvents;

	NXC_TRAP_CFG_ENTRY *m_pTrapList;
	NXC_TRAP_CFG_ENTRY *m_pCurrTrap;
	DWORD m_dwNumTraps;

	Template **m_ppTemplateList;
	Template *m_pCurrTemplate;
	DWORD m_dwNumTemplates;
	DCItem *m_pCurrDCI;
	Threshold *m_pCurrThreshold;

	DWORD m_dwCurrDCIIndex;
	TCHAR *m_pszValidationErrorText;
	int m_nMaxValidationErrorLen;

   int m_nContext;
   NXMP_Lexer *m_pLexer;
   NXMP_Parser *m_pParser;

   void Error(const char *pszMsg, ...);

public:
   NXMP_Data(NXMP_Lexer *pLexer, NXMP_Parser *pParser);
   ~NXMP_Data();

	void SetCurrDCIIndex(DWORD dwIndex) { m_dwCurrDCIIndex = dwIndex; }
   BOOL Validate(DWORD dwFlags, TCHAR *pszErrorText, int nLen);
	BOOL ValidateThreshold(Threshold *pThreshold, DWORD dwIndex);
   DWORD Install(DWORD dwFlags);

	void SetEvent(char *pszEvent, int nTarget);

   void NewEvent(char *pszName);
   void SetEventText(char *pszText);
   void SetEventDescription(char *pszText);
   void SetEventCode(DWORD dwCode) { if (m_pCurrEvent != NULL) m_pCurrEvent->dwCode = dwCode; }
   void SetEventSeverity(DWORD dwSeverity) { if (m_pCurrEvent != NULL) m_pCurrEvent->dwSeverity = dwSeverity; }
   void SetEventFlags(DWORD dwFlags) { if (m_pCurrEvent != NULL) m_pCurrEvent->dwFlags = dwFlags; }
   void CloseEvent(void) { m_pCurrEvent = NULL; m_nContext = CTX_NONE; }
	DWORD FindEvent(TCHAR *pszName);

   void NewTrap(char *pszOID);
	void SetTrapDescription(char *pszText);
	void SetTrapUserTag(char *pszText);
	void AddTrapParam(char *pszOID, int nPos, char *pszDescr);
   void CloseTrap(void) { m_pCurrTrap = NULL; m_nContext = CTX_NONE; }

   void NewTemplate(char *pszName);
	void CloseTemplate(void) { m_pCurrTemplate = NULL; m_nContext = CTX_NONE; }

	void NewDCI(void);
	void AddDCISchedule(char *pszStr);
	void CloseDCI(void);

	void NewThreshold(void);
	void CloseThreshold(void);

   BOOL ParseVariable(char *pszName, char *pszValue);
};


//
// Modified lexer class
//

class NXMP_Lexer
{
	friend int yylex(YYSTYPE *, yyscan_t, NXMP_Parser *);

protected:
   int m_nSourceSize;
   int m_nSourcePos;
   char *m_pszSource;
   NXMP_Parser *m_pParser;

   int m_nCurrLine;
   int m_nCommentLevel;
   int m_nStrSize;
   char m_szStr[MAX_STRING_SIZE];
   BOOL m_bErrorState;

public:
	NXMP_Lexer(NXMP_Parser *pParser, TCHAR *pszSource);
	virtual ~NXMP_Lexer();

	int LexerInput(char *pBuffer, int nMaxSize);
	void Error(const char *pszMsg);

   int GetCurrLine(void) { return m_nCurrLine; }

   void SetErrorState(void) { m_bErrorState = TRUE; }
   BOOL IsErrorState(void) { return m_bErrorState; }
};


//
// Parser class
//

class NXMP_Parser
{
protected:
   TCHAR *m_pszErrorText;
   NXMP_Lexer *m_pLexer;

public:
   NXMP_Parser(void);
   ~NXMP_Parser();

   NXMP_Data *Parse(TCHAR *pszSource);
   void Error(const char *pszMsg);

   TCHAR *GetErrorText(void) { return CHECK_NULL(m_pszErrorText); }
};

#endif
