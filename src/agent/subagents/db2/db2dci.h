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

#define NUM_OF_DCI 64

enum Dci
{
   DCI_DBMS_VERSION,
   DCI_NUM_AVAILABLE,
   DCI_NUM_UNAVAILABLE,
   DCI_DATA_L_SIZE,
   DCI_DATA_P_SIZE,
   DCI_INDEX_L_SIZE,
   DCI_INDEX_P_SIZE,
   DCI_LONG_L_SIZE,
   DCI_LONG_P_SIZE,
   DCI_LOB_L_SIZE,
   DCI_LOB_P_SIZE,
   DCI_XML_L_SIZE,
   DCI_XML_P_SIZE,
   DCI_INDEX_TYPE1,
   DCI_INDEX_TYPE2,
   DCI_REORG_PENDING,
   DCI_REORG_ABORTED,
   DCI_REORG_EXECUTING,
   DCI_REORG_NULL,
   DCI_REORG_PAUSED,
   DCI_LOAD_IN_PROGRESS,
   DCI_LOAD_NULL,
   DCI_LOAD_PENDING,
   DCI_ACCESS_RO,
   DCI_NO_LOAD_RESTART,
   DCI_NUM_REORG_REC_ALTERS,
   DCI_INDEX_REQUIRE_REBUILD,
   DCI_LARGE_RIDS,
   DCI_LARGE_RIDS_PENDING,
   DCI_NO_LARGE_RIDS,
   DCI_LARGE_SLOTS,
   DCI_LARGE_SLOTS_PENDING,
   DCI_NO_LARGE_SLOTS,
   DCI_DICTIONARY_SIZE,
   DCI_NULL
};

Dci StringToDci(const TCHAR* stringDci);

#endif /* DB2DCI_H_ */
