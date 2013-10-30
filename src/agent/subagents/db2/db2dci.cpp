/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Raden Solutions
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

#include "db2dci.h"

Dci StringToDci(const TCHAR* stringDci)
{
   IfEqualsReturn(stringDci, DCI_DBMS_VERSION)
   IfEqualsReturn(stringDci, DCI_NUM_AVAILABLE)
   IfEqualsReturn(stringDci, DCI_NUM_UNAVAILABLE)
   IfEqualsReturn(stringDci, DCI_DATA_L_SIZE)
   IfEqualsReturn(stringDci, DCI_DATA_P_SIZE)

   return DCI_NULL;
}
