/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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

#include <nxconfig.h>


//
// Constants
//

#define MAP_OBJECT_SIZE_X     40
#define MAP_OBJECT_SIZE_Y     40
#define MAP_OBJECT_INTERVAL_X 50
#define MAP_OBJECT_INTERVAL_Y 50
#define MAP_TEXT_BOX_HEIGHT   24
#define MAP_TOP_MARGIN        10
#define MAP_LEFT_MARGIN       20
#define MAP_BOTTOM_MARGIN     10
#define MAP_RIGHT_MARGIN      20
#define MAX_CONNECTOR_NAME		128
#define MAX_PORT_COUNT			16


//
// Submap attributes
//

#define SUBMAP_ATTR_AUTOMATIC_LAYOUT      0x00000001
#define SUBMAP_ATTR_HAS_BK_IMAGE          0x00000002
#define SUBMAP_ATTR_LAYOUT_COMPLETED      0x00010000


//
// Submap layout methods
//

#define SUBMAP_LAYOUT_DUMB                0
#define SUBMAP_LAYOUT_RADIAL              1
#define SUBMAP_LAYOUT_REINGOLD_TILFORD    2


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
#define LINK_TYPE_MULTILINK   2


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
	TCHAR szPort1[MAX_CONNECTOR_NAME];
	TCHAR szPort2[MAX_CONNECTOR_NAME];
	int portIdCount;
	DWORD portId1[MAX_PORT_COUNT];
	DWORD portId2[MAX_PORT_COUNT];
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
// Connected object list - used as source for nxSubmap::DoLayout
//

class LIBNXMAP_EXPORTABLE nxmap_ObjList
{
protected:
   DWORD m_dwNumObjects;
   DWORD *m_pdwObjectList;
   DWORD m_dwNumLinks;
   OBJLINK *m_pLinkList;

public:
   nxmap_ObjList();
   nxmap_ObjList(nxmap_ObjList *pSrc);
   nxmap_ObjList(CSCPMessage *pMsg);
   ~nxmap_ObjList();

   void AddObject(DWORD dwId);
   void LinkObjects(DWORD dwId1, DWORD dwId2);
   void LinkObjectsEx(DWORD dwId1, DWORD dwId2, const TCHAR *pszPort1, const TCHAR *pszPort2, DWORD portId1, DWORD portId2);
   void Clear(void);

   DWORD GetNumObjects(void) { return m_dwNumObjects; }
   DWORD *GetObjects(void) { return m_pdwObjectList; }
   DWORD GetNumLinks(void) { return m_dwNumLinks; }
   OBJLINK *GetLinks(void) { return m_pLinkList; }

	void CreateMessage(CSCPMessage *pMsg);
};


//
// Graph's vertex
//

class LIBNXMAP_EXPORTABLE nxmap_Vertex
{
protected:
   DWORD m_dwId;
   DWORD m_dwNumChilds;
   nxmap_Vertex **m_ppChildList;
   DWORD m_dwNumParents;
   nxmap_Vertex **m_ppParentList;
   int m_posX;
   int m_posY;
   BOOL m_isProcessed;  // Processed flag for various recursive operations

   void LinkParent(nxmap_Vertex *pVtx);
   void UnlinkParent(nxmap_Vertex *pVtx);

public:
   nxmap_Vertex(DWORD dwId);
   ~nxmap_Vertex();

   DWORD GetId(void) { return m_dwId; }
   int GetPosX(void) { return m_posX; }
   int GetPosY(void) { return m_posY; }
   DWORD GetNumChilds(void) { return m_dwNumChilds; }
   nxmap_Vertex *GetChild(DWORD dwIndex) { return (dwIndex < m_dwNumChilds) ? m_ppChildList[dwIndex] : NULL; }
   DWORD GetNumParents(void) { return m_dwNumParents; }
   nxmap_Vertex *GetParent(DWORD dwIndex) { return (dwIndex < m_dwNumParents) ? m_ppParentList[dwIndex] : NULL; }
   BOOL IsParentOf(nxmap_Vertex *pVertex);
   BOOL IsProcessed(void) { return m_isProcessed; }

   void LinkChild(nxmap_Vertex *pVtx);
   void UnlinkChild(nxmap_Vertex *pVtx);
   void SetPosition(int x, int y) { m_posX = x; m_posY = y; }
   void SetAsProcessed(void) { m_isProcessed = TRUE; }
   void SetAsUnprocessed(void) { m_isProcessed = FALSE; }
};


//
// Connected graph
//

class LIBNXMAP_EXPORTABLE nxmap_Graph
{
protected:
   DWORD m_dwVertexCount;
   nxmap_Vertex **m_ppVertexList;
   nxmap_Vertex *m_pRoot;

   void SetAsUnprocessed(void);
   void NormalizeVertexLinks(nxmap_Vertex *pRoot);

public:
   nxmap_Graph();
   nxmap_Graph(DWORD dwNumObjects, DWORD *pdwObjectList, DWORD dwNumLinks, OBJLINK *pLinkList);
   ~nxmap_Graph();

   DWORD GetVertexCount(void) { return m_dwVertexCount; }
   nxmap_Vertex *FindVertex(DWORD dwId);
   DWORD GetVertexIndex(nxmap_Vertex *pVertex);
   nxmap_Vertex *GetRootVertex(void) { return (m_pRoot != NULL) ? m_pRoot : (m_dwVertexCount > 0 ? m_ppVertexList[0] : NULL); }
   nxmap_Vertex *GetVertexByIndex(DWORD dwIndex) { return dwIndex < m_dwVertexCount ? m_ppVertexList[dwIndex] : NULL; }

   void SetRootVertex(DWORD dwId);
   void NormalizeLinks(void);
   void NormalizeVertexPositions(void);
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

   POINT GetMinSize(void);

   BOOL GetAutoLayoutFlag(void) { return (m_dwAttr & SUBMAP_ATTR_AUTOMATIC_LAYOUT) ? TRUE : FALSE; }
   void SetAutoLayoutFlag(BOOL bFlag) { if (bFlag) m_dwAttr |= SUBMAP_ATTR_AUTOMATIC_LAYOUT; else m_dwAttr &= ~SUBMAP_ATTR_AUTOMATIC_LAYOUT; }

   BOOL GetBkImageFlag(void) { return (m_dwAttr & SUBMAP_ATTR_HAS_BK_IMAGE) ? TRUE : FALSE; }
   void SetBkImageFlag(BOOL bFlag) { if (bFlag) m_dwAttr |= SUBMAP_ATTR_HAS_BK_IMAGE; else m_dwAttr &= ~SUBMAP_ATTR_HAS_BK_IMAGE; }

   BOOL IsLayoutCompleted(void) { return (m_dwAttr & SUBMAP_ATTR_LAYOUT_COMPLETED) ? TRUE : FALSE; }

   void DoLayout(DWORD dwNumObjects, DWORD *pdwObjectList,
                 DWORD dwNumLinks, OBJLINK *pLinkList,
                 int nIdealX, int nIdealY, int nMethod,
					  BOOL bNormalize);
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

	void LinkObjects(DWORD dwObj1, const TCHAR *pszPort1, DWORD dwObj2, const TCHAR *pszPort2, int nType);
	void UnlinkObjects(DWORD dwObj1, DWORD dwObj2);

	void DeleteObject(DWORD dwObject);
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

   void CommonInit();

public:
   nxMap();
   nxMap(DWORD dwMapId, DWORD dwObjectId, const TCHAR *pszName, const TCHAR *pszDescription);
   nxMap(CSCPMessage *pMsg);
   virtual ~nxMap();

   void Lock() { MutexLock(m_mutex); }
   void Unlock() { MutexUnlock(m_mutex); }

	void SetName(const TCHAR *name) { safe_free(m_pszName); m_pszName = _tcsdup(name); }

   DWORD MapId() { return m_dwMapId; }
   DWORD ObjectId() { return m_dwObjectId; }
   TCHAR *Name() { return CHECK_NULL(m_pszName); }

   void AddSubmap(nxSubmap *pSubmap);
   DWORD GetSubmapCount(void) { return m_dwNumSubmaps; }
   nxSubmap *GetSubmap(DWORD dwObjectId);
   nxSubmap *GetSubmapByIndex(DWORD dwIndex) { return dwIndex < m_dwNumSubmaps ? m_ppSubmaps[dwIndex] : NULL; }
   BOOL IsSubmapExist(DWORD dwObjectId, BOOL bLock = TRUE);

   void CreateMessage(CSCPMessage *pMsg);
   void ModifyFromMessage(CSCPMessage *pMsg);
};


/********** 1.2.x Map API ********/


//
// Map element types
//

#define MAP_ELEMENT_GENERIC      0
#define MAP_ELEMENT_OBJECT       1
#define MAP_ELEMENT_DECORATION   2


//
// Decoration types
//

#define MAP_DECORATION_GROUP_BOX 0
#define MAP_DECORATION_IMAGE     1


//
// Generic map element
//

class LIBNXMAP_EXPORTABLE NetworkMapElement
{
protected:
	DWORD m_id;
	LONG m_type;
	LONG m_posX;
	LONG m_posY;

public:
	NetworkMapElement(DWORD id);
	NetworkMapElement(DWORD id, Config *config);
	NetworkMapElement(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapElement();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, DWORD baseId);

	DWORD getId() { return m_id; }
	LONG getType() { return m_type; }
	LONG getPosX() { return m_posX; }
	LONG getPosY() { return m_posY; }

	void setPosition(LONG x, LONG y);
};


//
// Object map element
//

class LIBNXMAP_EXPORTABLE NetworkMapObject : public NetworkMapElement
{
protected:
	DWORD m_objectId;

public:
	NetworkMapObject(DWORD id, DWORD objectId);
	NetworkMapObject(DWORD id, Config *config);
	NetworkMapObject(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapObject();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, DWORD baseId);

	DWORD getObjectId() { return m_objectId; }
};


//
// Decoration map element
//

class LIBNXMAP_EXPORTABLE NetworkMapDecoration : public NetworkMapElement
{
protected:
	LONG m_decorationType;
	DWORD m_color;
	TCHAR *m_title;
	LONG m_width;
	LONG m_height;

public:
	NetworkMapDecoration(DWORD id, LONG decorationType);
	NetworkMapDecoration(DWORD id, Config *config);
	NetworkMapDecoration(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapDecoration();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, DWORD baseId);

	LONG getDecorationType() { return m_decorationType; }
	DWORD getColor() { return m_color; }
	const TCHAR *getTitle() { return CHECK_NULL_EX(m_title); }

	LONG getWidth() { return m_width; }
	LONG getHeight() { return m_height; }
};


//
// Link on map
//

class LIBNXMAP_EXPORTABLE NetworkMapLink
{
protected:
	DWORD m_element1;
	DWORD m_element2;
	int m_type;
	TCHAR *m_name;
	TCHAR *m_connectorName1;
	TCHAR *m_connectorName2;
	DWORD m_color;
	DWORD m_statusObject;

public:
	NetworkMapLink(DWORD e1, DWORD e2, int type);
	NetworkMapLink(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapLink();

	void fillMessage(CSCPMessage *msg, DWORD baseId);

	DWORD getElement1() { return m_element1; }
	DWORD getElement2() { return m_element2; }

	const TCHAR *getName() { return CHECK_NULL_EX(m_name); }
	const TCHAR *getConnector1Name() { return CHECK_NULL_EX(m_connectorName1); }
	const TCHAR *getConnector2Name() { return CHECK_NULL_EX(m_connectorName2); }
	int getType() { return m_type; }
	DWORD getColor() { return m_color; }
	DWORD getStatusObject() { return m_statusObject; }

	void setName(const TCHAR *name);
	void setConnector1Name(const TCHAR *name);
	void setConnector2Name(const TCHAR *name);
	void setColor(DWORD color) { m_color = color; }
	void setStatusObject(DWORD object) { m_statusObject = object; }
};


#endif
