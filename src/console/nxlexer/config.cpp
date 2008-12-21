/* 
** NetXMS - Network Management System
** Lexer for Scintilla
** Copyright (C) 2006, 2007, 2008 Victor Kirhenshtein
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
** File: config.cpp
**
**/

#include "nxlexer.h"


//
// Check if we are at line start
//

BOOL IsLineStart(WindowAccessor &acc, int nPos)
{
   int i;
   char ch;

   if (nPos == 0)
      return TRUE;

   i = nPos - 1;
   do
   {
      ch = acc.SafeGetCharAt(i--);
      if (ch == '\n')
         return TRUE;
   } while(isspace(ch) && (i > 0));
   return (i == 0) ? isspace(ch) : FALSE;
}


//
// Lexer for agent's configuration file
//

void ConfigLexer(UINT nStartPos, int nLen, int nInitStyle,
                 char **ppszWords, WindowAccessor &acc)
{
   UINT i, nPos, nEndPos;
   int nState = nInitStyle, nNewState, nShift;
   char ch;

   if (nStartPos > 0)
   {
      if (acc.SafeGetCharAt(nStartPos - 1) == '\n')
         nState = NX_STYLE_DEFAULT;
   }

   nEndPos = nStartPos + nLen;
   for(nPos = nStartPos, nNewState = nState; nPos < nEndPos; nPos++)
   {
      ch = acc.SafeGetCharAt(nPos);
      nShift = -1;
      switch(nState)
      {
         case NX_STYLE_DEFAULT:
            switch(ch)
            {
               case '#':
                  nNewState = NX_STYLE_COMMENT;
                  break;
               case '=':
                  nNewState = NX_STYLE_SYMBOL;
                  break;
               case '"':
                  nNewState = NX_STYLE_STRING;
                  break;
               case '*':
                  if ((nPos == 0) || (acc.SafeGetCharAt(nPos - 1) == '\n'))
                     nNewState = NX_STYLE_SECTION;
                  break;
               default:
                  if (isalpha(ch) && IsLineStart(acc, nPos))
                  {
                     char szBuffer[256];
                     UINT nOrigPos = nPos;

                     for(i = 0, nPos++; (nPos < nEndPos) && isalpha(ch) && (i < 255); i++)
                     {
                        szBuffer[i] = ch;
                        ch = acc.SafeGetCharAt(nPos++);
                     }
                     szBuffer[i] = 0;
                     if (IsKeyword(ppszWords[0], szBuffer))
                     {
                        acc.ColourTo(nOrigPos - 1, nState);
                        nState = NX_STYLE_KEYWORD;
                        nNewState = NX_STYLE_DEFAULT;
                     }
                  }
                  break;
            }
            break;
         case NX_STYLE_COMMENT:
         case NX_STYLE_ERROR:
            switch(ch)
            {
               case '\r':
               case '\n':
                  nNewState = NX_STYLE_DEFAULT;
                  break;
               default:
                  break;
            }
            break;
         case NX_STYLE_SYMBOL:
            switch(ch)
            {
               case '#':
                  nNewState = NX_STYLE_COMMENT;
                  break;
               case '=':
                  nNewState = NX_STYLE_ERROR;
                  break;
               default:
                  nNewState = NX_STYLE_DEFAULT;
                  break;
            }
            break;
         case NX_STYLE_STRING:
            switch(ch)
            {
               case '\r':
               case '\n':
                  nState = NX_STYLE_ERROR;
                  nNewState = NX_STYLE_DEFAULT;
                  break;
               case '"':
                  nNewState = NX_STYLE_DEFAULT;
                  nShift = 0;
                  break;
               default:
                  break;
            }
            break;
         case NX_STYLE_SECTION:
            switch(ch)
            {
               //case '\r':
               case '\n':
                  nNewState = NX_STYLE_DEFAULT;
                  nShift = 0;
                  break;
               default:
                  break;
            }
            break;
         default:
            break;
      }
      if (nNewState != nState)
      {
         if (nPos > nStartPos)
            acc.ColourTo(nPos + nShift, nState);
         nState = nNewState;
      }
   }
   acc.ColourTo(nEndPos - 1, nState);
}
