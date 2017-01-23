/*
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: nxcore_ps.h
**
**/

#ifndef _nxcore_ps_h_
#define _nxcore_ps_h_

/**
 * Persistent storage functions
 */
void PersistentStorageInit();
void PersistentStorageDestroy();
void SetPersistentStorageValue(const TCHAR *key, const TCHAR *value);
bool DeletePersistentStorageValue(const TCHAR *key);
const TCHAR *GetPersistentStorageValue(const TCHAR *key);
void GetPersistentStorageList(NXCPMessage *msg);
void UpdatePStorageDatabase(DB_HANDLE hdb, UINT32 watchdogId);

/**
 * NXSL class for persistent storage
 */
class NXSL_PersistentStorage : public NXSL_Storage
{
   virtual void write(const TCHAR *name, NXSL_Value *value);
   virtual NXSL_Value *read(const TCHAR *name);
   void remove(const TCHAR *name);
};


/**
 * NXSL persistent storage
 */
extern NXSL_PersistentStorage g_nxslPstorage;

#endif
