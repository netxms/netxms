/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
** File: keyvaluelist.cpp
**
**/

#include "nms_agent.h"

OptionList::OptionList(const TCHAR *parameters, int offset) 
{
    int index = offset;
    TCHAR buffer[256] = _T("");
    while(AgentGetParameterArg(parameters, index++, buffer, 256))
    {
        Trim(buffer);

        if (buffer[0] == 0)
            break;

        TCHAR *key = buffer;
        
        TCHAR *p = _tcschr(buffer, _T('='));
        if (p != nullptr)
        {
            *p = 0;
            TCHAR *value = p + 1;

            optionmap.set(key, value);
        }
        else
        {
            optionmap.set(key, _T(""));
        }
    }
}

bool OptionList::exists(const TCHAR *key) const
{
    return optionmap.contains(key);
}

const TCHAR* OptionList::get(const TCHAR *key) const
{
    return optionmap.get(key);
}
