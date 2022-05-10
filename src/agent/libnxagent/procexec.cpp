/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
 * Create new key-value executor object
 */
KeyValueOutputProcessExecutor::KeyValueOutputProcessExecutor(const TCHAR *command, bool shellExec) : ProcessExecutor(command, shellExec)
{
   m_sendOutput = true;
   m_replaceNullCharacters = true;
   m_separator = _T('=');
}

/**
 * Key-value executor output handler
 */
void KeyValueOutputProcessExecutor::onOutput(const char *text, size_t length)
{
   TCHAR *buffer;
#ifdef UNICODE
   buffer = WideStringFromMBStringSysLocale(text);
#else
   buffer = MemCopyStringA(text);
#endif
   TCHAR *newLinePtr = nullptr, *lineStartPtr = buffer, *eqPtr = nullptr;
   do
   {
      newLinePtr = _tcschr(lineStartPtr, _T('\r'));
      if (newLinePtr == nullptr)
         newLinePtr = _tcschr(lineStartPtr, _T('\n'));
      if (newLinePtr != nullptr)
      {
         *newLinePtr = 0;
         m_buffer.append(lineStartPtr);
         if (m_buffer.length() > MAX_RESULT_LENGTH * 3)
         {
            nxlog_debug(4, _T("ParamExec::onOutput(): result too long - %s"), m_buffer.cstr());
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
            nxlog_debug(4, _T("ParamExec::onOutput(): result too long - %s"), m_buffer.cstr());
            stop();
            m_buffer.clear();
         }
         break;
      }

      if (m_buffer.length() > 1)
      {
         eqPtr = _tcschr(m_buffer.getBuffer(), m_separator);
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

   MemFree(buffer);
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

/**
 * Create new line output executor object
 */
LineOutputProcessExecutor::LineOutputProcessExecutor(const TCHAR *command, bool shellExec) : ProcessExecutor(command, shellExec)
{
   m_sendOutput = true;
   m_replaceNullCharacters = true;
}

/**
 * Line output executor output handler
 */
void LineOutputProcessExecutor::onOutput(const char *text, size_t length)
{
   TCHAR *buffer;
#ifdef UNICODE
   buffer = WideStringFromMBStringSysLocale(text);
#else
   buffer = MemCopyStringA(text);
#endif
   TCHAR *newLinePtr = nullptr, *lineStartPtr = buffer;
   do
   {
      newLinePtr = _tcschr(lineStartPtr, _T('\r'));
      if (newLinePtr == nullptr)
      {
         newLinePtr = _tcschr(lineStartPtr, _T('\n'));
      }
      else
      {
         if (*(newLinePtr + 1) == '\n')
         {
            *newLinePtr = 0;
            newLinePtr++;
         }
      }
      if (newLinePtr != nullptr)
      {
         *newLinePtr = 0;
         m_buffer.append(lineStartPtr);
      }
      else
      {
         m_buffer.append(lineStartPtr);
         break;
      }

      m_data.add(m_buffer);
      m_buffer.clear();
      lineStartPtr = newLinePtr + 1;
   } while (*lineStartPtr != 0);

   MemFree(buffer);
}

/**
 * End of output callback
 */
void LineOutputProcessExecutor::endOfOutput()
{
   if (m_buffer.length() > 0)
   {
      m_data.add(m_buffer);
      m_buffer.clear();
   }
}
