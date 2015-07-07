/*
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: util.cpp
**
**/

#include "libnxdb.h"

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, UINT32 id)
{
	bool exist = false;

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT %s FROM %s WHERE %s=?"), idColumn, table, idColumn);

	DB_STATEMENT hStmt = DBPrepare(hdb, query);
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			exist = (DBGetNumRows(hResult) > 0);
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
	return exist;
}

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const uuid& id)
{
	bool exist = false;

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT %s FROM %s WHERE %s=?"), idColumn, table, idColumn);

	DB_STATEMENT hStmt = DBPrepare(hdb, query);
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			exist = (DBGetNumRows(hResult) > 0);
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
	return exist;
}

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const TCHAR *id)
{
	bool exist = false;

	TCHAR query[1256];
	_sntprintf(query, sizeof(query), _T("SELECT %s FROM %s WHERE %s='?'"), idColumn, table, idColumn);

	DB_STATEMENT hStmt = DBPrepare(hdb, query);
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, id, DB_BIND_STATIC);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			exist = (DBGetNumRows(hResult) > 0);
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
	return exist;
}

/**
 * Characters to be escaped before writing to SQL
 */
static TCHAR m_szSpecialChars[] = _T("\x01\x02\x03\x04\x05\x06\x07\x08")
                                  _T("\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10")
                                  _T("\x11\x12\x13\x14\x15\x16\x17\x18")
                                  _T("\x19\x1A\x1B\x1C\x1D\x1E\x1F")
                                  _T("#%\\'\x7F");

/**
 * Escape some special characters in string for writing into database.
 * DEPRECATED!
 */
TCHAR LIBNXDB_EXPORTABLE *EncodeSQLString(const TCHAR *pszIn)
{
   TCHAR *pszOut;
   int iPosIn, iPosOut, iStrSize;

   if ((pszIn != NULL) && (*pszIn != 0))
   {
      // Allocate destination buffer
      iStrSize = (int)_tcslen(pszIn) + 1;
      for(iPosIn = 0; pszIn[iPosIn] != 0; iPosIn++)
         if (_tcschr(m_szSpecialChars, pszIn[iPosIn])  != NULL)
            iStrSize += 2;
      pszOut = (TCHAR *)malloc(iStrSize * sizeof(TCHAR));

      // Translate string
      for(iPosIn = 0, iPosOut = 0; pszIn[iPosIn] != 0; iPosIn++)
         if (_tcschr(m_szSpecialChars, pszIn[iPosIn]) != NULL)
         {
            pszOut[iPosOut++] = _T('#');
            pszOut[iPosOut++] = bin2hex(pszIn[iPosIn] >> 4);
            pszOut[iPosOut++] = bin2hex(pszIn[iPosIn] & 0x0F);
         }
         else
         {
            pszOut[iPosOut++] = pszIn[iPosIn];
         }
      pszOut[iPosOut] = 0;
   }
   else
   {
      // Encode empty strings as #00
      pszOut = (TCHAR *)malloc(4 * sizeof(TCHAR));
      _tcscpy(pszOut, _T("#00"));
   }
   return pszOut;
}

/**
 * Restore characters encoded by EncodeSQLString()
 * Characters are decoded "in place"
 */
void LIBNXDB_EXPORTABLE DecodeSQLString(TCHAR *pszStr)
{
   int iPosIn, iPosOut;

   if (pszStr == NULL)
      return;

   for(iPosIn = 0, iPosOut = 0; pszStr[iPosIn] != 0; iPosIn++)
   {
      if (pszStr[iPosIn] == _T('#'))
      {
         iPosIn++;
         pszStr[iPosOut] = hex2bin(pszStr[iPosIn]) << 4;
         iPosIn++;
         pszStr[iPosOut] |= hex2bin(pszStr[iPosIn]);
         iPosOut++;
      }
      else
      {
         pszStr[iPosOut++] = pszStr[iPosIn];
      }
   }
   pszStr[iPosOut] = 0;
}

/**
 * Get database schema version
 * Will return 0 for unknown and -1 in case of SQL errors
 */
int LIBNXDB_EXPORTABLE DBGetSchemaVersion(DB_HANDLE conn)
{
	DB_RESULT hResult;
	int version = 0;

	// Read schema version from 'metadata' table, where it should
	// be stored starting from schema version 87
	// We ignore SQL error in this case, because table 'metadata'
	// may not exist in old schema versions
   hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='SchemaVersion'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         version = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }

	// If database schema version is less than 87, version number
	// will be stored in 'config' table
	if (version == 0)
	{
		hResult = DBSelect(conn, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
				version = DBGetFieldLong(hResult, 0, 0);
			DBFreeResult(hResult);
		}
		else
		{
			version = -1;
		}
	}

	return version;
}

/**
 * Get database syntax
 */
int LIBNXDB_EXPORTABLE DBGetSyntax(DB_HANDLE conn)
{
	DB_RESULT hResult;
	TCHAR syntaxId[256];
	bool read = false;
	int syntax;

   // Get database syntax
   hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='Syntax'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, syntaxId, sizeof(syntaxId) / sizeof(TCHAR));
			read = true;
      }
      else
      {
         _tcscpy(syntaxId, _T("UNKNOWN"));
      }
      DBFreeResult(hResult);
   }

	// If database schema version is less than 87, syntax
	// will be stored in 'config' table, so try it
	if (!read)
	{
		hResult = DBSelect(conn, _T("SELECT var_value FROM config WHERE var_name='DBSyntax'"));
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, syntaxId, sizeof(syntaxId) / sizeof(TCHAR));
				read = true;
			}
			else
			{
				_tcscpy(syntaxId, _T("UNKNOWN"));
			}
			DBFreeResult(hResult);
		}
	}

   if (!_tcscmp(syntaxId, _T("MYSQL")))
   {
      syntax = DB_SYNTAX_MYSQL;
   }
   else if (!_tcscmp(syntaxId, _T("PGSQL")))
   {
      syntax = DB_SYNTAX_PGSQL;
   }
   else if (!_tcscmp(syntaxId, _T("MSSQL")))
   {
      syntax = DB_SYNTAX_MSSQL;
   }
   else if (!_tcscmp(syntaxId, _T("ORACLE")))
   {
      syntax = DB_SYNTAX_ORACLE;
   }
   else if (!_tcscmp(syntaxId, _T("SQLITE")))
   {
      syntax = DB_SYNTAX_SQLITE;
   }
   else if (!_tcscmp(syntaxId, _T("DB2")))
   {
      syntax = DB_SYNTAX_DB2;
   }
   else
   {
		syntax = DB_SYNTAX_UNKNOWN;
   }

	return syntax;
}
