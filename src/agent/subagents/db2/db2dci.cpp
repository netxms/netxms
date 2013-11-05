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
   IfEqualsReturn(stringDci, DCI_TABLE_SCANS)
   IfEqualsReturn(stringDci, DCI_ROWS_READ)
   IfEqualsReturn(stringDci, DCI_ROWS_INSERTED)
   IfEqualsReturn(stringDci, DCI_ROWS_UPDATED)
   IfEqualsReturn(stringDci, DCI_ROWS_DELETED)
   IfEqualsReturn(stringDci, DCI_OVERFLOW_ACCESSES)
   IfEqualsReturn(stringDci, DCI_OVERFLOW_CREATES)
   IfEqualsReturn(stringDci, DCI_PAGE_REORGS)
   IfEqualsReturn(stringDci, DCI_DATA_L_PAGES)
   IfEqualsReturn(stringDci, DCI_LOB_L_PAGES)
   IfEqualsReturn(stringDci, DCI_LONG_L_PAGES)
   IfEqualsReturn(stringDci, DCI_INDEX_L_PAGES)
   IfEqualsReturn(stringDci, DCI_XDA_L_PAGES)
   IfEqualsReturn(stringDci, DCI_NO_CHANGE_UPDATES)
   IfEqualsReturn(stringDci, DCI_LOCK_WAIT_TIME)
   IfEqualsReturn(stringDci, DCI_LOCK_WAIT_TIME_GLOBAL)
   IfEqualsReturn(stringDci, DCI_LOCK_WAITS)
   IfEqualsReturn(stringDci, DCI_LOCK_WAITS_GLOBAL)
   IfEqualsReturn(stringDci, DCI_LOCK_ESCALS)
   IfEqualsReturn(stringDci, DCI_LOCK_ESCALS_GLOBAL)
   IfEqualsReturn(stringDci, DCI_STATE_SHARED)
   IfEqualsReturn(stringDci, DCI_STATE_NOT_SHARED)
   IfEqualsReturn(stringDci, DCI_STATE_SHARED_BECOMING)
   IfEqualsReturn(stringDci, DCI_STATE_NOT_SHARED_BECOMING)
   IfEqualsReturn(stringDci, DCI_SHARING_LOCKWAIT_COUNT)
   IfEqualsReturn(stringDci, DCI_SHARING_LOCKWAIT_TIME)
   IfEqualsReturn(stringDci, DCI_DIRECT_WRITES)
   IfEqualsReturn(stringDci, DCI_DIRECT_WRITE_REQS)
   IfEqualsReturn(stringDci, DCI_DIRECT_READS)
   IfEqualsReturn(stringDci, DCI_DIRECT_READ_REQS)
   IfEqualsReturn(stringDci, DCI_DATA_L_READS)
   IfEqualsReturn(stringDci, DCI_DATA_P_READS)
   IfEqualsReturn(stringDci, DCI_DATA_GBP_L_READS)
   IfEqualsReturn(stringDci, DCI_DATA_GBP_P_READS)
   IfEqualsReturn(stringDci, DCI_DATA_GBP_INVALID_PAGES)
   IfEqualsReturn(stringDci, DCI_DATA_LBP_PAGES_FOUND)
   IfEqualsReturn(stringDci, DCI_DATA_GBP_INDEP_PAGES_FOUND_IN_LBP)
   IfEqualsReturn(stringDci, DCI_XDA_L_READS)
   IfEqualsReturn(stringDci, DCI_XDA_P_READS)
   IfEqualsReturn(stringDci, DCI_XDA_GBP_L_READS)
   IfEqualsReturn(stringDci, DCI_XDA_GBP_P_READS)
   IfEqualsReturn(stringDci, DCI_XDA_GBP_INVALID_PAGES)
   IfEqualsReturn(stringDci, DCI_XDA_LBP_PAGES_FOUND)
   IfEqualsReturn(stringDci, DCI_XDA_GBP_INDEP_PAGES_FOUND_IN_LBP)
   IfEqualsReturn(stringDci, DCI_NUM_PAGE_DICT_BUILT)
   IfEqualsReturn(stringDci, DCI_STATS_ROWS_MODIFIED)
   IfEqualsReturn(stringDci, DCI_RTS_ROWS_MODIFIED)
   IfEqualsReturn(stringDci, DCI_COL_OBJECT_L_PAGES)
   IfEqualsReturn(stringDci, DCI_ORGANIZATION_ROWS)
   IfEqualsReturn(stringDci, DCI_ORGANIZATION_COLS)
   IfEqualsReturn(stringDci, DCI_COL_L_READS)
   IfEqualsReturn(stringDci, DCI_COL_P_READS)
   IfEqualsReturn(stringDci, DCI_COL_GBP_L_READS)
   IfEqualsReturn(stringDci, DCI_COL_GBP_P_READS)
   IfEqualsReturn(stringDci, DCI_COL_GBP_INVALID_PAGES)
   IfEqualsReturn(stringDci, DCI_COL_LBP_PAGES_FOUND)
   IfEqualsReturn(stringDci, DCI_COL_GBP_INDEP_PAGES_FOUND_IN_LBP)
   IfEqualsReturn(stringDci, DCI_NUM_COL_REFS)
   IfEqualsReturn(stringDci, DCI_SECTION_EXEC_WITH_COL_REFS)

   return DCI_NULL;
}
