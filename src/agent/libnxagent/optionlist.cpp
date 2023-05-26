/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2022-2023 Raden Solutions
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
 ** File: optionlist.cpp
 **
 **/

#include "libnxagent.h"

/**
 * Create option list from metric arguments
 */
OptionList::OptionList(const TCHAR *metric, int offset)
{
   m_valid = true;
   int index = offset;
   TCHAR buffer[256] = _T("");
   while (true)
   {
      if (!AgentGetParameterArg(metric, index++, buffer, 256))
      {
         m_valid = false;
         break;
      }

      Trim(buffer);

      if (buffer[0] == 0)
         break;

      TCHAR *key = buffer;

      TCHAR *p = _tcschr(buffer, _T('='));
      if (p != nullptr)
      {
         *p = 0;
         TCHAR *value = p + 1;

         m_options.set(key, value);
      }
      else
      {
         m_options.set(key, _T(""));
      }
   }
}

/**
 * Get option as boolean
 */
bool OptionList::getAsBoolean(const TCHAR *key, bool defaultValue) const
{
   const TCHAR *value = m_options.get(key);
   if (value == nullptr)
      return defaultValue;
   if (!_tcsicmp(value, _T("true")))
      return true;
   if (!_tcsicmp(value, _T("false")))
      return false;
   return _tcstol(value, nullptr, 0) != 0;
}
