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

#include "db2dci.h"

Dci StringToDci(const TCHAR* stringDci)
{
   IfEqualsReturn(stringDci, DCI_DBMS_VERSION)
   IfEqualsReturn(stringDci, DCI_NUM_AVAILABLE)
   IfEqualsReturn(stringDci, DCI_NUM_UNAVAILABLE)
   IfEqualsReturn(stringDci, DCI_DATA_L_SIZE)
   IfEqualsReturn(stringDci, DCI_DATA_P_SIZE)
   IfEqualsReturn(stringDci, DCI_INDEX_L_SIZE)
   IfEqualsReturn(stringDci, DCI_INDEX_P_SIZE)
   IfEqualsReturn(stringDci, DCI_LONG_L_SIZE)
   IfEqualsReturn(stringDci, DCI_LONG_P_SIZE)
   IfEqualsReturn(stringDci, DCI_LOB_L_SIZE)
   IfEqualsReturn(stringDci, DCI_LOB_P_SIZE)
   IfEqualsReturn(stringDci, DCI_XML_L_SIZE)
   IfEqualsReturn(stringDci, DCI_XML_P_SIZE)
   IfEqualsReturn(stringDci, DCI_INDEX_TYPE1)
   IfEqualsReturn(stringDci, DCI_INDEX_TYPE2)
   IfEqualsReturn(stringDci, DCI_REORG_PENDING)
   IfEqualsReturn(stringDci, DCI_REORG_ABORTED)
   IfEqualsReturn(stringDci, DCI_REORG_EXECUTING)
   IfEqualsReturn(stringDci, DCI_REORG_NULL)
   IfEqualsReturn(stringDci, DCI_REORG_PAUSED)
   IfEqualsReturn(stringDci, DCI_LOAD_IN_PROGRESS)
   IfEqualsReturn(stringDci, DCI_LOAD_NULL)
   IfEqualsReturn(stringDci, DCI_LOAD_PENDING)
   IfEqualsReturn(stringDci, DCI_ACCESS_RO)
   IfEqualsReturn(stringDci, DCI_NO_LOAD_RESTART)
   IfEqualsReturn(stringDci, DCI_NUM_REORG_REC_ALTERS)
   IfEqualsReturn(stringDci, DCI_INDEX_REQUIRE_REBUILD)
   IfEqualsReturn(stringDci, DCI_LARGE_RIDS)
   IfEqualsReturn(stringDci, DCI_LARGE_RIDS_PENDING)
   IfEqualsReturn(stringDci, DCI_NO_LARGE_RIDS)
   IfEqualsReturn(stringDci, DCI_LARGE_SLOTS)
   IfEqualsReturn(stringDci, DCI_LARGE_SLOTS_PENDING)
   IfEqualsReturn(stringDci, DCI_NO_LARGE_SLOTS)
   IfEqualsReturn(stringDci, DCI_DICTIONARY_SIZE)

   return DCI_NULL;
}
