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
** File: netxms_maps.h
**
**/

#ifndef _netxms_maps_h_
#define _netxms_maps_h_

#ifdef _WIN32
#ifdef LIBNXMAP_EXPORTS
#define LIBNXMAP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXMAP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXMAP_EXPORTABLE
#endif


//
// Constants
//

#define MAP_OBJECT_SIZE_X     40
#define MAP_OBJECT_SIZE_Y     40
#define MAP_OBJECT_INTERVAL   30
#define MAP_TEXT_BOX_HEIGHT   20
#define MAP_TOP_MARGIN        10
#define MAP_LEFT_MARGIN       15


//
// Submap attributes
//

#define SUBMAP_ATTR_LAYOUT_COMPLETED      0x00010000


//
// Object-on-map structure
//

struct MAP_OBJECT
{
   DWORD dwId;
   LONG x;
   LONG y;
   DWORD dwState;    // Runtime field, can be used freely by application
};


//
// Link between objects
//

struct OBJLINK
{
   DWORD dwId1;
   DWORD dwId2;
};


//
// Access list entry
//

struct MAP_ACL_ENTRY
{
   DWORD dwUserId;
   DWORD dwAccess;
};


//
// Submap class
//

class LIBNXMAP_EXPORTABLE nxSubmap
{
protected:
   DWORD m_dwId;
   DWORD m_dwAttr;
   DWORD m_dwNumObjects;
   MAP_OBJECT *m_pObjectList;
   DWORD m_dwNumLinks;
   OBJLINK *m_pLinkList;

   void CommonInit(void);

public:
   nxSubmap();
   nxSubmap(DWORD dwObjectId);
   nxSubmap(CSCPMessage *pMsg);
   virtual ~nxSubmap();

   DWORD Id(void) { return m_dwId; }

   void CreateMessage(CSCPMessage *pMsg);
   void ModifyFromMessage(CSCPMessage *pMsg);

   BOOL IsLayoutCompleted(void) { return (m_dwAttr & SUBMAP_ATTR_LAYOUT_COMPLETED) ? TRUE : FALSE; }
   void DoLayout(DWORD dwNumObjects, DWORD *pdwObjectList,
                 DWORD dwNumLinks, OBJLINK *pLinkList,
                 int nIdealX, int nIdealY);
   POINT GetObjectPosition(DWORD dwObjectId);
   POINT GetObjectPositionByIndex(DWORD dwIndex);
   void SetObjectPosition(DWORD dwObjectId, int x, int y);
   void SetObjectPositionByIndex(DWORD dwIndex, int x, int y);

   DWORD GetNumObjects(void) { return m_dwNumObjects; }
   DWORD GetObjectIdFromIndex(DWORD dwIndex) { return m_pObjectList[dwIndex].dwId; }
   DWORD GetObjectIndex(DWORD dwObjectId);

   void SetObjectState(DWORD dwObjectId, DWORD dwState);
   void SetObjectStateByIndex(DWORD dwIndex, DWORD dwState) { m_pObjectList[dwIndex].dwState = dwState; }
   DWORD GetObjectState(DWORD dwObjectId);
   DWORD GetObjectStateFromIndex(DWORD dwIndex) { return m_pObjectList[dwIndex].dwState; }

   DWORD GetNumLinks(void) { return m_dwNumLinks; }
   OBJLINK *GetLinkByIndex(DWORD dwIndex) { return &m_pLinkList[dwIndex]; }
};


//
// Map class
//

class LIBNXMAP_EXPORTABLE nxMap
{
protected:
   TCHAR *m_pszName;
   TCHAR *m_pszDescription;
   DWORD m_dwObjectId;
   DWORD m_dwNumSubmaps;
   nxSubmap **m_ppSubmaps;
   DWORD m_dwACLSize;
   MAP_ACL_ENTRY *m_pACL;
   MUTEX m_mutex;

   void CommonInit(void);
   void Lock(void) { MutexLock(m_mutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_mutex); }

public:
   nxMap();
   nxMap(DWORD dwObjectId, TCHAR *pszName, TCHAR *pszDescription);
   nxMap(CSCPMessage *pMsg);
   virtual ~nxMap();

   DWORD ObjectId(void) { return m_dwObjectId; }

   DWORD GetSubmapCount(void) { return m_dwNumSubmaps; }
   nxSubmap *GetSubmap(DWORD dwObjectId);
   nxSubmap *GetSubmapByIndex(DWORD dwIndex) { return dwIndex < m_dwNumSubmaps ? m_ppSubmaps[dwIndex] : NULL; }

   void CreateMessage(CSCPMessage *pMsg);
   void ModifyFromMessage(CSCPMessage *pMsg);
};

#endif
