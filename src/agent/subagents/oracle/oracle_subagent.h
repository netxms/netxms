/*
** NetXMS subagent for Oracle monitoring
** Copyright (C) 2009-2012 Raden Solutions
**/

#ifndef _oracle_subagent_h_
#define _oracle_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

//
// Misc defines
//

#define MAX_STR				(255)
#define MAX_QUERY			(8192)
#define MYNAMESTR			_T("oracle")
#define DB_NULLARG_MAGIC	_T("8201")

// Oracle-specific
#define MAX_USERNAME	(30+1)
#define MAX_PASSWORD	(30+1)

#define MAX_DATABASES	(5)


//
// DB-related structs
//

// struct for the databases configured within the subagent
typedef struct _DatabaseInfo
{
	TCHAR id[MAX_STR];				// this is how client addresses the database
	TCHAR name[MAX_STR];
	TCHAR server[MAX_STR];
	TCHAR username[MAX_USERNAME];
	TCHAR password[MAX_PASSWORD];
	THREAD queryThreadHandle;
	DB_HANDLE handle;
	bool connected;
	int version;					// in xxx format
	MUTEX accessMutex;
} DatabaseInfo;

typedef struct {
	TCHAR name[MAX_STR];
	StringMap *attrs;
} DBParameter;

typedef struct {
	int version;						// minimum database version in xxx format for this query
	const TCHAR *prefix;						// parameter prefix, e.g. "Oracle.Tablespaces."
	const TCHAR *query;						// the query
	int	 queryColumns;						// number of columns returned by query
	DBParameter *values[MAX_DATABASES];	// list of values
	int valueCount[MAX_DATABASES];
} DBParameterGroup;

// struct for stat data obtained from the database
typedef struct
{
	DWORD openCursors;
	DWORD sessions;
} DatabaseData;

//
// Functions
//

bool getParametersFromDB(int dbIndex);

#endif   /* _oracle_subagent_h_ */
