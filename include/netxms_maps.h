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
#define MAP_OBJECT_INTERVAL_X 40
#define MAP_OBJECT_INTERVAL_Y 20
#define MAP_TEXT_BOX_HEIGHT   24
#define MAP_TOP_MARGIN        10
#define MAP_LEFT_MARGIN       20
#define MAP_BOTTOM_MARGIN     10
#define MAP_RIGHT_MARGIN      20


//
// Submap attributes
//

#define SUBMAP_ATTR_LAYOUT_COMPLETED      0x00010000
#define SUBMAP_ATTR_HAS_BK_IMAGE          0x00020000


//
// User access rights
//

#define MAP_ACCESS_READ       0x0001
#define MAP_ACCESS_WRITE      0x0002
#define MAP_ACCESS_ACL        0x0004
#define MAP_ACCESS_DELETE     0x0008


//
// Object link types
//

#define LINK_TYPE_NORMAL      0
#define LINK_TYPE_VPN         1


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
   LONG nType;
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

   virtual void CommonInit(void);

public:
   nxSubmap();
   nxSubmap(DWORD dwObjectId);
   nxSubmap(CSCPMessage *pMsg);
   virtual ~nxSubmap();

   DWORD Id(void) { return m_dwId; }

   void CreateMessage(CSCPMessage *pMsg);
   void ModifyFromMessage(CSCPMessage *pMsg);

   POINT GetMinSize(void);

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

   BOOL GetBkImageFlag(void) { return (m_dwAttr & SUBMAP_ATTR_HAS_BK_IMAGE) ? TRUE : FALSE; }
   void SetBkImageFlag(BOOL bFlag) { if (bFlag) m_dwAttr |= SUBMAP_ATTR_HAS_BK_IMAGE; else m_dwAttr &= ~SUBMAP_ATTR_HAS_BK_IMAGE; }
};


//
// Callback type for submap creation
//

class nxMap;
typedef nxSubmap * (* SUBMAP_CREATION_CALLBACK)(DWORD, nxMap *);


//
// Map class
//

class LIBNXMAP_EXPORTABLE nxMap
{
protected:
   DWORD m_dwMapId;
   TCHAR *m_pszName;
   TCHAR *m_pszDescription;
   DWORD m_dwObjectId;
   DWORD m_dwNumSubmaps;
   nxSubmap **m_ppSubmaps;
   DWORD m_dwACLSize;
   MAP_ACL_ENTRY *m_pACL;
   MUTEX m_mutex;
   SUBMAP_CREATION_CALLBACK m_pfCreateSubmap;

   virtual void CommonInit(void);

public:
   nxMap();
   nxMap(DWORD dwMapId, DWORD dwObjectId, TCHAR *pszName, TCHAR *pszDescription);
   nxMap(CSCPMessage *pMsg);
   virtual ~nxMap();

   void Lock(void) { MutexLock(m_mutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_mutex); }

   DWORD MapId(void) { return m_dwMapId; }
   DWORD ObjectId(void) { return m_dwObjectId; }
   TCHAR *Name(void) { return CHECK_NULL(m_pszName); }

   DWORD GetSubmapCount(void) { return m_dwNumSubmaps; }
   nxSubmap *GetSubmap(DWORD dwObjectId);
   nxSubmap *GetSubmapByIndex(DWORD dwIndex) { return dwIndex < m_dwNumSubmaps ? m_ppSubmaps[dwIndex] : NULL; }
   BOOL IsSubmapExist(DWORD dwObjectId, BOOL bLock = TRUE);

   void CreateMessage(CSCPMessage *pMsg);
   void ModifyFromMessage(CSCPMessage *pMsg);
};

#endif
