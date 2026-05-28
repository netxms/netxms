/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dci_table_creation.h
**
** Per-object idata_<N> / tdata_<N> table and index creation statements.
** Kept in code (not metadata rows) so schema changes don't require
** patching metadata.in and running a metadata rewrite upgrade.
**
** Each builder takes a zero-based slot index and returns true if a
** statement was written into the caller's buffer, false if there is
** nothing to execute for that slot. Slot 0 always yields the CREATE
** TABLE; subsequent slots yield CREATE INDEX or other per-object DDL.
**
** Builders accept an explicit DB syntax so callers that target a
** specific engine (for example, the SQLite export file) can override
** the server's g_dbSyntax.
**
**/

#ifndef _dci_table_creation_h_
#define _dci_table_creation_h_

#include <nxdbapi.h>
#include <nxsrvapi.h>

#define DCI_TABLE_CREATION_SLOT_COUNT 10

/**
 * Build DDL statement (CREATE TABLE at slot 0, CREATE INDEX at slot 1+)
 * for per-object idata_<objectId> table.
 */
static inline bool BuildIDataCreationQuery(int dbSyntax, uint32_t objectId, int slot, wchar_t *query, size_t size)
{
   if (slot == 0)
   {
      if (dbSyntax == DB_SYNTAX_ORACLE)
      {
         nx_swprintf(query, size,
            L"CREATE TABLE idata_%u (item_id integer not null,idata_timestamp number(20) not null,"
            L"idata_value varchar2(255 char) null,raw_value varchar2(255 char) null,"
            L"PRIMARY KEY(item_id,idata_timestamp))", objectId);
      }
      else
      {
         const wchar_t *int64Type = (dbSyntax == DB_SYNTAX_DB2) ? L"BIGINT" : L"bigint";
         nx_swprintf(query, size,
            L"CREATE TABLE idata_%u (item_id integer not null,idata_timestamp %s not null,"
            L"idata_value varchar(255) null,raw_value varchar(255) null,"
            L"PRIMARY KEY(item_id,idata_timestamp))", objectId, int64Type);
      }
      return true;
   }

   if (slot == 1)
   {
      if ((dbSyntax != DB_SYNTAX_MYSQL) && (dbSyntax != DB_SYNTAX_PGSQL) && (dbSyntax != DB_SYNTAX_MSSQL))
         return false;
      nx_swprintf(query, size,
         L"CREATE INDEX idx_idata_%u_timestamp_id ON idata_%u(idata_timestamp,item_id)",
         objectId, objectId);
      return true;
   }

   return false;
}

/**
 * Build DDL statement (CREATE TABLE at slot 0, CREATE INDEX at slot 1+)
 * for per-object hourly or daily aggregate table
 * (idata_1h_<objectId> / idata_1d_<objectId>).
 *
 * Only meaningful for non-TSDB dialects - TSDB aggregation lives in
 * global continuous aggregates, so callers must filter g_dbSyntax themselves.
 */
static inline bool BuildIDataAggregateCreationQuery(int dbSyntax, bool hourly, uint32_t objectId, int slot, wchar_t *query, size_t size)
{
   if (slot != 0)
      return false;

   const wchar_t *suffix = hourly ? L"1h" : L"1d";

   const wchar_t *int64Type = (dbSyntax == DB_SYNTAX_ORACLE) ? L"number(20)" : L"bigint";
   const wchar_t *doubleType;
   switch(dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         doubleType = L"float";
         break;
      case DB_SYNTAX_ORACLE:
         doubleType = L"binary_double";
         break;
      case DB_SYNTAX_SQLITE:
         doubleType = L"real";
         break;
      default:
         doubleType = L"double precision";
         break;
   }

   nx_swprintf(query, size,
      L"CREATE TABLE idata_%s_%u (item_id integer not null,bucket_start %s not null,"
      L"min_value %s null,max_value %s null,avg_value %s null,sample_count integer not null,"
      L"PRIMARY KEY(item_id,bucket_start))",
      suffix, objectId, int64Type, doubleType, doubleType, doubleType);
   return true;
}

/**
 * Build DDL statement (CREATE TABLE at slot 0, CREATE INDEX at slot 1+)
 * for per-object tdata_<objectId> table.
 */
static inline bool BuildTDataCreationQuery(int dbSyntax, uint32_t objectId, int slot, wchar_t *query, size_t size)
{
   if (slot != 0)
      return false;

   const wchar_t *int64Type;
   const wchar_t *textType;
   switch(dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         int64Type = L"bigint";
         textType = L"varchar(max)";
         break;
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         int64Type = L"bigint";
         textType = L"text";
         break;
      case DB_SYNTAX_ORACLE:
         int64Type = L"number(20)";
         textType = L"clob";
         break;
      case DB_SYNTAX_SQLITE:
         int64Type = L"bigint";
         textType = L"varchar";
         break;
      case DB_SYNTAX_MYSQL:
         int64Type = L"bigint";
         textType = L"longtext";
         break;
      case DB_SYNTAX_DB2:
         int64Type = L"BIGINT";
         textType = L"LONG VARCHAR";
         break;
      default:
         int64Type = L"bigint";
         textType = L"text";
         break;
   }

   nx_swprintf(query, size,
      L"CREATE TABLE tdata_%u (item_id integer not null,tdata_timestamp %s not null,"
      L"tdata_value %s null,PRIMARY KEY(item_id,tdata_timestamp))",
      objectId, int64Type, textType);
   return true;
}

#endif   /* _dci_table_creation_h_ */
