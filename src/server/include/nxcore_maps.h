/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: nxcore_maps.h
**
**/

#ifndef _nxcore_maps_h_
#define _nxcore_maps_h_

#include <netxms_maps.h>


//
// Server-side submap class
//

class nxSubmapSrv : public nxSubmap
{
protected:
   DWORD m_dwMapId;     // ID of parent map

public:
   nxSubmapSrv(DB_RESULT hResult, int nRow, DWORD dwMapId);
   nxSubmapSrv(DWORD dwObjectId, DWORD dwMapId);

   DWORD SaveToDB(void);
};


//
// Server-side map class
//

class nxMapSrv : public nxMap
{
protected:
   int m_nRefCount;

public:
   nxMapSrv(DB_RESULT hResult, int nRow);
   nxMapSrv(DWORD dwMapId, DWORD dwObjectId, const TCHAR *pszName, const TCHAR *pszDescription);

   DWORD SaveToDB(void);
   BOOL CheckUserRights(DWORD dwUserId, DWORD dwDesiredAccess);

   void IncRefCount(void) { Lock(); m_nRefCount++; Unlock(); }
   void DecRefCount(void) { Lock(); if (m_nRefCount > 0) m_nRefCount--; Unlock(); }
   int GetRefCount(void) { int nRef; Lock(); nRef = m_nRefCount; Unlock(); return nRef; }
};


//
// Functions
//

void CreateMapListMessage(CSCPMessage &msg, DWORD dwUserId);
DWORD CreateNewMap(DWORD rootObj, const TCHAR *name, DWORD *newId);
void LoadMaps(void);
DWORD GetMapIdFromName(TCHAR *pszName, DWORD *pdwMapId);
BOOL LockMaps(void);
void UnlockMaps(void);
nxMapSrv *FindMapByID(DWORD dwMapId);


#endif   /* _nxcore_maps_h_ */
