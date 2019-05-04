/*
** NetXMS multiplatform core agent
** Copyright (C) 2016 Raden Solutions
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
** File: localdb.h
**
**/


#ifndef _localdb_h_
#define _localdb_h_

/**
 * Database schema version
 */
#define DB_SCHEMA_VERSION     8

bool OpenLocalDatabase();
void CloseLocalDatabase();
DB_HANDLE GetLocalDatabaseHandle();

TCHAR *ReadMetadata(const TCHAR *attr, TCHAR *buffer);
INT32 ReadMetadataAsInt(const TCHAR *attr);
bool WriteMetadata(const TCHAR *name, const TCHAR *value);
bool WriteMetadata(const TCHAR *name, INT32 value);

bool UpgradeDatabase();

#endif
