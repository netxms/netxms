/* 
** NetXMS - Network Management System
** Lexer for Scintilla
** Copyright (C) 2006 Victor Kirhenshtein
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
** $module: nxlexer.cpp
**
**/

#include "nxlexer.h"


//
// Exportable functions call spec
//

#define LEXER_CALL __stdcall


//
// Externals
//

void ConfigLexer(UINT nStartPos, int nLen, int nInitStyle,
                 char **ppszWords, WindowAccessor &acc);


//
// Check if given keyword is a part of keyword list
//

BOOL IsKeyword(char *pszList, char *pszWord)
{
   TCHAR *pszNext, szBuffer[256];

   pszNext = ExtractWord(pszList, szBuffer);
   while(szBuffer[0] != 0)
   {
      if (!stricmp(szBuffer, pszWord))
         return TRUE;
      pszNext = ExtractWord(pszNext, szBuffer);
   }
   return FALSE;
}


//
// Get lexer counts
//

extern "C" int LEXER_CALL GetLexerCount(void)
{
   return 1;
}


//
// Get lexer name
//

extern "C" void LEXER_CALL GetLexerName(UINT nIndex, char *pszBuffer, int nBufLen)
{
   switch(nIndex)
   {
      case 0:
         nx_strncpy(pszBuffer, "nxconfig", nBufLen);
         break;
      default:
         break;
   }
}


//
// Lexer dispatcher
//

extern "C" void LEXER_CALL Lex(UINT nIndex, UINT nStartPos, int nLen,
                               int nStyle, char **ppszWords, WindowID wId, char *pszProps)
{
   PropSet ps;
   ps.SetMultiple(pszProps);
   WindowAccessor acc(wId, ps);

   acc.StartAt(nStartPos);
   acc.StartSegment(nStartPos);
   switch(nIndex)
   {
      case 0:
         ConfigLexer(nStartPos, nLen, nStyle, ppszWords, acc);
         break;
      default:
         break;
   }
   acc.Flush();
}


//
// Folder dispatcher
//

extern "C" void LEXER_CALL Fold(UINT nIndex, UINT nStartPos, int nLen,
                                int nStyle, char **ppszWords, WindowID wId, char *pszProps)
{
}


//
// DLL entry point
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}
