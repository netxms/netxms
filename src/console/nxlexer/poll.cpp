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
** File: poll.cpp
**
**/

#include "nxlexer.h"


//
// Externals
//

BOOL IsLineStart(WindowAccessor &acc, int nPos);


//
// Lexer for node poller output
//

void PollLexer(UINT nStartPos, int nLen, int nInitStyle,
               char **ppszWords, WindowAccessor &acc)
{
   UINT nPos, nEndPos;
   int nState = nInitStyle, nNewState, nNextState, nShift;
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
               case '[':
                  nNewState = NX_STYLE_TIMESTAMP;
                  break;
               case '\x7F':
						ch = acc.SafeGetCharAt(nPos + 1);
						switch(ch)
						{
							case '.':
								nNewState = NX_STYLE_HIDDEN;
								nNextState = NX_STYLE_DEFAULT;
								break;
							case 'i':
								nNewState = NX_STYLE_HIDDEN;
								nNextState = NX_STYLE_POLL_INFO;
								break;
							case 'w':
								nNewState = NX_STYLE_HIDDEN;
								nNextState = NX_STYLE_POLL_WARNING;
								break;
							case 'e':
								nNewState = NX_STYLE_HIDDEN;
								nNextState = NX_STYLE_POLL_ERROR;
								break;
							default:
								break;
						}
						if (nNewState == NX_STYLE_HIDDEN)
							nPos++;
					default:
						break;
            }
            break;
			case NX_STYLE_HIDDEN:
				nNewState = nNextState;
				break;
         case NX_STYLE_TIMESTAMP:
            switch(ch)
            {
               case '\r':
               case '\n':
					case ']':
                  nNewState = NX_STYLE_DEFAULT;
                  break;
               default:
                  break;
            }
            break;
         default:
				if ((ch == '\r') || (ch =='\n'))
					nNewState = NX_STYLE_DEFAULT;
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
