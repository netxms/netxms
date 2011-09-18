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
#define MAX_PASSWORD	(30+1)

#define MAX_DATABASES	(5)


//
// DB-related structs
//

// struct for the databases configured within the subagent
typedef struct 
{
	TCHAR id[MAX_STR];				// this is how client addresses the database
	TCHAR dsn[MAX_STR];
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
	THREAD queryThreadHandle;
	DB_HANDLE handle;
	bool connected;
	int version;					// in xxx format
	MUTEX accessMutex;
} DatabaseInfo;

typedef struct {
	TCHAR name[MAX_STR];
	StringMap* attrs;
} DBParameter;

typedef struct {
	int version;						// minimum database version in xxx format for this query
	TCHAR* prefix;						// parameter prefix, e.g. "Informix.Dbspaces."
	TCHAR* query;						// the query
	int	  queryColumns;						// number of columns returned by query
	DBParameter* values[MAX_DATABASES];		// list of values
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

#endif   /* _INFORMIX_SUBAGENT_H_ */
