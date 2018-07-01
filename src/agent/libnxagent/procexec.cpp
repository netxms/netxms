/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: procexec.cpp
**
**/

#include "libnxagent.h"

/**
 * Create new parameter executor object
 */
KeyValueOutputProcessExecutor::KeyValueOutputProcessExecutor(const TCHAR *command) : ProcessExecutor(command)
{
   m_sendOutput = true;
   m_separator = _T('=');
}

/**
 * Parameter executor output handler
 */
void KeyValueOutputProcessExecutor::onOutput(const char *text)
{
   if (text == NULL)
      return;

   TCHAR *buffer;
#ifdef UNICODE
   buffer = WideStringFromMBStringSysLocale(text);
#else
   buffer = _tcsdup(text);
#endif
   TCHAR *newLinePtr = NULL, *lineStartPtr = buffer, *eqPtr = NULL;
   do
   {
      newLinePtr = _tcschr(lineStartPtr, _T('\r'));
      if (newLinePtr == NULL)
         newLinePtr = _tcschr(lineStartPtr, _T('\n'));
      if (newLinePtr != NULL)
      {
         *newLinePtr = 0;
         m_buffer.append(lineStartPtr);
         if (m_buffer.length() > MAX_RESULT_LENGTH * 3)
         {
            nxlog_debug(4, _T("ParamExec::onOutput(): result too long - %s"), (const TCHAR *)m_buffer);
            stop();
            m_buffer.clear();
            break;
         }
      }
      else
      {
         m_buffer.append(lineStartPtr);
         if (m_buffer.length() > MAX_RESULT_LENGTH * 3)
         {
            nxlog_debug(4, _T("ParamExec::onOutput(): result too long - %s"), (const TCHAR *)m_buffer);
            stop();
            m_buffer.clear();
         }
         break;
      }

      if (m_buffer.length() > 1)
      {
         eqPtr = _tcschr(m_buffer.getBuffer(), _T('='));
         if (eqPtr != NULL)
         {
            *eqPtr = 0;
            eqPtr++;
            Trim(m_buffer.getBuffer());
            Trim(eqPtr);
            m_data.set(m_buffer.getBuffer(), eqPtr);
         }
      }
      m_buffer.clear();
      lineStartPtr = newLinePtr + 1;
   } while (*lineStartPtr != 0);

   free(buffer);
}

/**
 * End of output callback
 */
void KeyValueOutputProcessExecutor::endOfOutput()
{
   if (m_buffer.length() > 0)
   {
      TCHAR *ptr = _tcschr(m_buffer.getBuffer(), m_separator);
      if (ptr != NULL)
      {
         *ptr = 0;
         ptr++;
         Trim(m_buffer.getBuffer());
         Trim(ptr);
         m_data.set(m_buffer.getBuffer(), ptr);
      }
      m_buffer.clear();
   }
}
