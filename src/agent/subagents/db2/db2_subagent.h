/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2025 Raden Solutions
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
#ifndef DB2_SUBAGENT_H_
#define DB2_SUBAGENT_H_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

#define SUBAGENT_NAME      _T("DB2")
#define DEBUG_TAG_DB2      _T("db2")
#define STR_MAX            256
#define DCI_LIST_SIZE      53

/**
 * Collected values. DCI_NULL is the terminator and also the number of values.
 */
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
   DCI_TABLE_SCANS,
   DCI_ROWS_READ,
   DCI_ROWS_INSERTED,
   DCI_ROWS_UPDATED,
   DCI_ROWS_DELETED,
   DCI_OVERFLOW_ACCESSES,
   DCI_OVERFLOW_CREATES,
   DCI_PAGE_REORGS,
   DCI_DATA_L_PAGES,
   DCI_LOB_L_PAGES,
   DCI_LONG_L_PAGES,
   DCI_INDEX_L_PAGES,
   DCI_XDA_L_PAGES,
   DCI_NO_CHANGE_UPDATES,
   DCI_LOCK_WAIT_TIME,
   DCI_LOCK_WAIT_TIME_GLOBAL,
   DCI_LOCK_WAITS,
   DCI_LOCK_WAITS_GLOBAL,
   DCI_LOCK_ESCALS,
   DCI_LOCK_ESCALS_GLOBAL,
   DCI_STATE_SHARED,
   DCI_STATE_NOT_SHARED,
   DCI_STATE_SHARED_BECOMING,
   DCI_STATE_NOT_SHARED_BECOMING,
   DCI_SHARING_LOCKWAIT_COUNT,
   DCI_SHARING_LOCKWAIT_TIME,
   DCI_DIRECT_WRITES,
   DCI_DIRECT_WRITE_REQS,
   DCI_DIRECT_READS,
   DCI_DIRECT_READ_REQS,
   DCI_DATA_L_READS,
   DCI_DATA_P_READS,
   DCI_DATA_GBP_L_READS,
   DCI_DATA_GBP_P_READS,
   DCI_DATA_GBP_INVALID_PAGES,
   DCI_DATA_LBP_PAGES_FOUND,
   DCI_DATA_GBP_INDEP_PAGES_FOUND_IN_LBP,
   DCI_XDA_L_READS,
   DCI_XDA_P_READS,
   DCI_XDA_GBP_L_READS,
   DCI_XDA_GBP_P_READS,
   DCI_XDA_GBP_INVALID_PAGES,
   DCI_XDA_LBP_PAGES_FOUND,
   DCI_XDA_GBP_INDEP_PAGES_FOUND_IN_LBP,
   DCI_NUM_PAGE_DICT_BUILT,
   DCI_STATS_ROWS_MODIFIED,
   DCI_RTS_ROWS_MODIFIED,
   DCI_COL_OBJECT_L_PAGES,
   DCI_ORGANIZATION_ROWS,
   DCI_ORGANIZATION_COLS,
   DCI_COL_L_READS,
   DCI_COL_P_READS,
   DCI_COL_GBP_L_READS,
   DCI_COL_GBP_P_READS,
   DCI_COL_GBP_INVALID_PAGES,
   DCI_COL_LBP_PAGES_FOUND,
   DCI_COL_GBP_INDEP_PAGES_FOUND_IN_LBP,
   DCI_NUM_COL_REFS,
   DCI_SECTION_EXEC_WITH_COL_REFS,
   DCI_NULL
};

/**
 * Database connection information
 */
struct ConnectionInfo
{
   TCHAR id[STR_MAX];           // connection ID (subsection name)
   TCHAR endpoint[STR_MAX];     // catalog alias = connect target
   TCHAR database[STR_MAX];     // legacy "Database" key, used as endpoint if endpoint is not set
   TCHAR username[STR_MAX];
   TCHAR password[MAX_PASSWORD];
   uint32_t connectionTTL;
};

/**
 * Query for polling
 */
struct QUERY
{
   Dci dciList[DCI_LIST_SIZE];   // one entry per result column, in column order
   const TCHAR *query;
};

/**
 * Database connection
 */
class DatabaseConnection
{
private:
   ConnectionInfo m_info;
   THREAD m_pollerThread;
   DB_HANDLE m_session;
   TCHAR m_data[DCI_NULL][STR_MAX];
   bool m_dataValid[DCI_NULL];
   Mutex m_dataLock;
   Condition m_stopCondition;

   void pollerThread();
   bool poll();
   void invalidateData();

public:
   DatabaseConnection(const ConnectionInfo *info);
   ~DatabaseConnection();

   void run();
   void stop();

   const TCHAR *getId() const { return m_info.id; }

   bool getData(Dci dci, TCHAR *value);
};

#endif /* DB2_SUBAGENT_H_ */
