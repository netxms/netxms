/**
 * NetXMS - open source network management system
 * Copyright (C) 2013 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef DB2DCI_H_
#define DB2DCI_H_

#include <unicode.h>
#include <wchar.h>

#define _D(x) _T(#x)
#define IfEqualsReturn(str, dci) if(_tcscmp(str, _D(dci)) == 0) return dci;


enum Dci
{
   DCI_DBMS_VERSION,
   DCI_NUM_AVAILABLE,
   DCI_NUM_UNAVAILABLE,
   DCI_DATA_L_SIZE,
   DCI_DATA_P_SIZE,
   DCI_NULL
};

static const TCHAR* const DCI_NAME_STRING[] =
{
   _D(DCI_DBMS_VERSION),
   _D(DCI_NUM_AVAILABLE),
   _D(DCI_NUM_UNAVAILABLE),
   _D(DCI_DATA_L_SIZE),
   _D(DCI_DATA_P_SIZE)
};

Dci StringToDci(const TCHAR* stringDci);

#endif /* DB2DCI_H_ */
