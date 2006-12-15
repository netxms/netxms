/* $Id: nxmp_parser.cpp,v 1.1 2006-12-15 11:38:14 victor Exp $ */
/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: nxmp_parser.cpp
**
**/

#include "nxcore.h"
#include "nxmp_parser.h"


//
// Externals
//

int yyparse(NXMP_Lexer *, NXMP_Parser *, NXMP_Data *);
extern int yydebug;


//
// Constructor
//

NXMP_Parser::NXMP_Parser(void)
{
   m_pszErrorText = NULL;
   m_pLexer = NULL;
}


//
// Destructor
//

NXMP_Parser::~NXMP_Parser()
{
   safe_free(m_pszErrorText);
   delete m_pLexer;
}


//
// Error handler
//

void NXMP_Parser::Error(const char *pszMsg)
{
   char szText[1024];

   if (m_pszErrorText == NULL)
   {
      snprintf(szText, 1024, "Error in line %d: %s", m_pLexer->GetCurrLine(), pszMsg);
#ifdef UNICODE
      nLen = strlen(szText) + 1;
      m_pszErrorText = (WCHAR *)malloc(nLen * sizeof(WCHAR));
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szText,
                          -1, m_pszErrorText, nLen, NULL, NULL);
#else
      m_pszErrorText = strdup(szText);
#endif
   }
}


//
// Parser source
//

NXMP_Data *NXMP_Parser::Parse(TCHAR *pszSource)
{
   NXMP_Data *pData;

   m_pLexer = new NXMP_Lexer(this, pszSource);
   pData = new NXMP_Data;
   if (yyparse(m_pLexer, this, pData) != 0)
   {
      delete pData;
      pData = NULL;
   }
   return pData;
}


//
// yyerror() for parser
//

void yyerror(NXMP_Lexer *pLexer, NXMP_Parser *pParser, NXMP_Data *pData, char *pszText)
{
   pParser->Error(pszText);
}
