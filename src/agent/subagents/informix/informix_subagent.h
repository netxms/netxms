/*
** NetXMS subagent for Informix monitoring
** Copyright (C) 2011-2026 Raden Solutions
**/

#ifndef _INFORMIX_SUBAGENT_H_
#define _INFORMIX_SUBAGENT_H_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

//
// Misc defines
//

#define MYNAMESTR          _T("INFORMIX")
#define DEBUG_TAG          _T("informix")
#define MAX_STR            256
#define MAX_USERNAME       (30 + 1)
#define DB_NULLARG_MAGIC   _T("1099")
#define NUM_PARAM_GROUPS   3

//
// DB-related structs
//

/**
 * Connection information (as configured)
 */
struct ConnectionInfo
{
   TCHAR id[MAX_STR];            // connection ID (how metrics address this connection)
   TCHAR endpoint[MAX_STR];      // Informix server instance (connect target)
   TCHAR database[MAX_STR];      // DSN / database name
   TCHAR username[MAX_USERNAME];
   TCHAR password[MAX_PASSWORD];
   uint32_t connectionTTL;
};

/**
 * Parameter group definition (query template shared by all connections)
 */
struct ParameterGroup
{
   int version;            // minimum database version (xxx format) required for this group
   const TCHAR *prefix;    // parameter prefix, e.g. "Informix.Dbspace.Pages."
   const TCHAR *query;     // query to execute
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
   bool m_connected;
   int m_version;
   StringObjectMap<StringMap> *m_data[NUM_PARAM_GROUPS];   // one entity->attributes map per parameter group
   Mutex m_dataLock;
   Condition m_stopCondition;

   void pollerThread();
   bool poll();

public:
   DatabaseConnection(const ConnectionInfo *info);
   ~DatabaseConnection();

   void run();
   void stop();

   const TCHAR *getId() const { return m_info.id; }
   bool isConnected() const { return m_connected; }

   LONG getData(int groupIndex, const TCHAR *entity, const TCHAR *key, TCHAR *value);
};

//
// Global variables
//

extern DB_DRIVER g_driverHandle;
extern const ParameterGroup g_paramGroups[NUM_PARAM_GROUPS];

#endif   /* _INFORMIX_SUBAGENT_H_ */
