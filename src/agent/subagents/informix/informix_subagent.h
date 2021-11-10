/*
** NetXMS subagent for Informix monitoring
** Copyright (C) 2011 Raden Solutions
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

#define MAX_STR				(255)
#define MAX_QUERY			(8192)
#define MYNAMESTR			_T("INFORMIX")
#define DB_NULLARG_MAGIC	_T("1099")


#define MAX_USERNAME	(30+1)

#define MAX_DATABASES	(5)


//
// DB-related structs
//

// struct for the databases configured within the subagent
struct DatabaseInfo
{
	TCHAR id[MAX_STR];				// this is how client addresses the database
	TCHAR server[MAX_STR];
	TCHAR dsn[MAX_STR];
	TCHAR username[MAX_USERNAME];
	TCHAR password[MAX_PASSWORD];
	THREAD queryThreadHandle;
	DB_HANDLE handle;
	bool connected;
	int version;					// in xxx format
	Mutex *accessMutex;
};

struct DBParameter
{
	TCHAR name[MAX_STR];
	StringMap* attrs;
};

struct DBParameterGroup 
{
	int version;						// minimum database version in xxx format for this query
	const TCHAR* prefix;						// parameter prefix, e.g. "Informix.Dbspaces."
	const TCHAR* query;						// the query
	int	  queryColumns;						// number of columns returned by query
	DBParameter* values[MAX_DATABASES];		// list of values
	int valueCount[MAX_DATABASES];
};

// struct for stat data obtained from the database
struct DatabaseData
{
	DWORD openCursors;
	DWORD sessions;
};

//
// Functions
//

bool getParametersFromDB(int dbIndex);

#endif   /* _INFORMIX_SUBAGENT_H_ */
