/*
** NetXMS - Network Management System
** Copyright (C) 2019 Raden Solutions
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
** File: physical_link.cpp
**
**/

#include "nxcore.h"

#define PL_THREAD_KEY _T("PhysicalLink")

/**
 * Physical link class
 */
class NXCORE_EXPORTABLE PhysicalLink
{
private:
   UINT32 m_id;
   TCHAR *m_description;
   UINT32 m_leftInterfaceParent;
   UINT32 m_leftObjectId;
   UINT32 m_leftPatchPannelId;
   UINT32 m_leftPortNumber;
   bool m_leftFront; //true for front false for back
   UINT32 m_rightObjectId;
   UINT32 m_rightInterfaceParent;
   UINT32 m_rightPatchPannelId;
   UINT32 m_rightPortNumber;
   bool m_rightFront; //true for front false for back

public:
   PhysicalLink(DB_RESULT hResult, int row);
   PhysicalLink(NXCPMessage *request);
   PhysicalLink();
   ~PhysicalLink() { MemFree(m_description); }

   UINT32 getId() const { return m_id; }
   UINT32 getLeftObjectId() const { return m_leftObjectId; }
   UINT32 getLeftParentId() const { return m_leftInterfaceParent; }
   UINT32 getLeftPatchPannelId() const { return m_leftPatchPannelId; }
   UINT32 getRightObjectId() const { return m_rightObjectId; }
   UINT32 getRightParentId() const { return m_rightInterfaceParent; }
   UINT32 getRightPatchPanelId() const { return m_rightPatchPannelId; }

   json_t *toJson() const;
   void saveToDatabase();
   UINT32 fillMessage(NXCPMessage *pMsg, UINT32 base, bool includeLeft, bool includeRight, bool rightFirst) const;
   void updateId() { m_id = CreateUniqueId(IDG_PHYSICAL_LINK); }
};

static AbstractIndex<std::shared_ptr<PhysicalLink>> s_physicalLinks(true);

/**
 * Physical link constructor from database
 */
PhysicalLink::PhysicalLink(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_description = DBGetField(hResult, row, 1, NULL, 0);

   m_leftObjectId = DBGetFieldLong(hResult, row, 2);
   NetObj *object = FindObjectById(m_leftObjectId, OBJECT_INTERFACE);
   if(object != NULL)
   {
      m_leftInterfaceParent = ((Interface *)object)->getParentNodeId();
   }
   else
      m_leftInterfaceParent = 0;

   m_leftPatchPannelId = DBGetFieldLong(hResult, row, 3);
   m_leftPortNumber = DBGetFieldLong(hResult, row, 4);
   m_leftFront = DBGetFieldLong(hResult, 0, 5) ? true : false;

   m_rightObjectId = DBGetFieldLong(hResult, row, 6);
   object = FindObjectById(m_rightObjectId, OBJECT_INTERFACE);
   if(object != NULL)
   {
      m_rightInterfaceParent = ((Interface *)object)->getParentNodeId();
   }
   else
      m_rightInterfaceParent = 0;

   m_rightPatchPannelId = DBGetFieldLong(hResult, row, 7);
   m_rightPortNumber = DBGetFieldLong(hResult, row, 8);
   m_rightFront = DBGetFieldLong(hResult, 0, 9) ? true : false;
}

/**
 * Physical link constructor from message
 */
PhysicalLink::PhysicalLink(NXCPMessage *request)
{
   long base = VID_LINK_LIST_BASE;
   m_id = request->getFieldAsInt32(base++);
   m_description = request->getFieldAsString(base++);

   m_leftObjectId = request->getFieldAsInt32(base++);
   NetObj *object = FindObjectById(m_leftObjectId, OBJECT_INTERFACE);
   if(object != NULL)
   {
      m_leftInterfaceParent = ((Interface *)object)->getParentNodeId();
   }
   else
      m_leftInterfaceParent = 0;
   m_leftPatchPannelId = request->getFieldAsInt32(base++);
   m_leftPortNumber = request->getFieldAsInt32(base++);
   m_leftFront = request->getFieldAsBoolean(base++);

   m_rightObjectId = request->getFieldAsInt32(base++);
   object = FindObjectById(m_rightObjectId, OBJECT_INTERFACE);
   if(object != NULL)
   {
      m_rightInterfaceParent = ((Interface *)object)->getParentNodeId();
   }
   else
      m_rightInterfaceParent = 0;
   m_rightPatchPannelId = request->getFieldAsInt32(base++);
   m_rightPortNumber = request->getFieldAsInt32(base++);
   m_rightFront = request->getFieldAsBoolean(base++);
}

/**
 * Physical link object constructor from database
 */
PhysicalLink::PhysicalLink()
{
   m_id = CreateUniqueId(IDG_PHYSICAL_LINK);
   m_description = MemCopyString(_T(""));
   m_leftObjectId = 0;
   m_leftInterfaceParent = 0;
   m_leftPatchPannelId = 0;
   m_leftPortNumber = 0;
   m_leftFront = true;
   m_rightObjectId = 0;
   m_rightInterfaceParent = 0;
   m_rightPatchPannelId = 0;
   m_rightPortNumber = 0;
   m_rightFront = true;
}

/**
 * To JSON method for physical link
 */
json_t *PhysicalLink::toJson() const
{
   json_t *root = new json_t;
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "leftObjectId", json_integer(m_leftObjectId));
   json_object_set_new(root, "leftPatchPannelId", json_integer(m_leftPatchPannelId));
   json_object_set_new(root, "leftPortNumber", json_integer(m_leftPortNumber));
   json_object_set_new(root, "leftFront", json_boolean(m_leftFront));
   json_object_set_new(root, "rightObjectId", json_integer(m_rightObjectId));
   json_object_set_new(root, "rightPatchPannelId", json_integer(m_rightPatchPannelId));
   json_object_set_new(root, "rightPortNumber", json_integer(m_rightPortNumber));
   json_object_set_new(root, "rightFront", json_boolean(m_rightFront));
   return root;
}

/**
 * Saves physical link to database
 */
void PhysicalLink::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const TCHAR *columns[] = { _T("description"),
         _T("left_object_id"), _T("left_patch_pannel_id"),  _T("left_port_number"), _T("left_front"),
         _T("right_object_id"), _T("right_patch_pannel_id"), _T("right_port_number"), _T("right_front"), NULL };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("physical_links"), _T("id"), m_id, columns);

   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_leftObjectId);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_leftPatchPannelId);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_leftPortNumber);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_leftFront ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_rightObjectId);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_rightPatchPannelId);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_rightPortNumber);
      DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_rightFront ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_id);

      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Fill information about one link side
 */
static void FillOneLinkSide(NXCPMessage *pMsg, UINT32 base, UINT32 objectId, UINT32 patchPannelId, UINT32 portNum, bool side)
{
   pMsg->setField(base++, objectId);
   pMsg->setField(base++, patchPannelId);
   pMsg->setField(base++, portNum);
   pMsg->setField(base++, side);
}

/**
 * Fill message with link data
 */
UINT32 PhysicalLink::fillMessage(NXCPMessage *pMsg, UINT32 base, bool includeLeft, bool includeRight, bool rightFirst) const
{
   pMsg->setField(base++, m_id);
   pMsg->setField(base++, m_description);
   if(rightFirst)
   {
      if(includeRight)
         FillOneLinkSide(pMsg, base, m_rightObjectId, m_rightPatchPannelId, m_rightPortNumber, m_rightFront);
      else
         FillOneLinkSide(pMsg, base, 0, 0, 0, false);
      base += 4;

      if(includeLeft)
         FillOneLinkSide(pMsg, base, m_leftObjectId, m_leftPatchPannelId, m_leftPortNumber, m_leftFront);
      else
         FillOneLinkSide(pMsg, base, 0, 0, 0, false);
      base += 4;
   }
   else
   {
      if(includeLeft)
         FillOneLinkSide(pMsg, base, m_leftObjectId, m_leftPatchPannelId, m_leftPortNumber, m_leftFront);
      else
         FillOneLinkSide(pMsg, base, 0, 0, 0, false);
      base += 4;

      if(includeRight)
         FillOneLinkSide(pMsg, base, m_rightObjectId, m_rightPatchPannelId, m_rightPortNumber, m_rightFront);
      else
         FillOneLinkSide(pMsg, base, 0, 0, 0, false);
      base += 4;
   }

   return base;
}

/**
 * Initial load of physical links on server startup
 */
bool LoadPhysicalLinks()
{
   nxlog_debug(1, _T("Loading physical links"));
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelect(hdb, _T("SELECT id,description,left_object_id,left_patch_pannel_id,left_port_number,left_front,")
            _T("right_object_id,right_patch_pannel_id,right_port_number,right_front FROM physical_links"));

   if (result == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   int count = DBGetNumRows(result);
   for(int i = 0; i < count; i++)
   {
      PhysicalLink *link = new PhysicalLink(result, i);
      s_physicalLinks.put(link->getId(), new std::shared_ptr<PhysicalLink>(link));
   }
   DBFreeResult(result);

   DBConnectionPoolReleaseConnection(hdb);
   return true;
}

/**
 * Delete physical link from database
 */
void DeletePhysicalLink(UINT32 *id)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM physical_links WHERE id=?"));
   if(hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_CTYPE_UINT32, *id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   delete id;
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Fill message callback data structure
 */
struct FillMessageCallbackData
{
   NXCPMessage *msg;
   IntegerArray<UINT32> *filter;
   UINT32 userId;
   UINT32 patchPanelId;
   UINT32 objCount;
   UINT32 base;
};

/**
 * Fill message with link information
 */
void FillMessageCallback(std::shared_ptr<PhysicalLink> *pLink, FillMessageCallbackData *data)
{
   bool rightFirst = false;
   PhysicalLink *link = pLink->get();
   if(data->filter != NULL) //filter objects
   {
      rightFirst = data->filter->indexOf(link->getRightObjectId()) != -1 || data->filter->indexOf(link->getRightParentId()) != -1;
      if(data->filter->indexOf(link->getLeftObjectId()) == -1 && !rightFirst &&
            data->filter->indexOf(link->getLeftParentId()) == -1)
         return;

      if(data->patchPanelId != 0 && link->getLeftPatchPannelId() != data->patchPanelId && link->getRightPatchPanelId() != data->patchPanelId)
         return;
   }
   //check access
   NetObj *obj = FindObjectById(link->getLeftObjectId());
   bool accessLeft;
   bool accessRight;
   if(obj != NULL)
      accessLeft = obj->checkAccessRights(data->userId, OBJECT_ACCESS_READ);
   else
      accessLeft = false;

   obj = FindObjectById(link->getRightObjectId());
   if(obj != NULL)
      accessRight = obj->checkAccessRights(data->userId, OBJECT_ACCESS_READ);
   else
      accessRight = false;

   if(accessLeft || accessRight)
   {
      link->fillMessage(data->msg, data->base, accessLeft, accessRight, rightFirst);
      data->base += 20;
      data->objCount++;
   }
}

/**
 * Get physical links for object
 */
void GetObjectPhysicalLinks(NetObj *obj, UINT32 userId, UINT32 patchPannelId, NXCPMessage *msg)
{
   IntegerArray<UINT32> m_filter;
   if (obj != NULL)
   {
      m_filter.add(obj->getId());
      if(obj->getObjectClass() == OBJECT_RACK)
      {
         if(patchPannelId == 0)
         {
            ObjectArray<NetObj> *nodes = obj->getChildList(OBJECT_NODE);
            for(int i = 0; i < nodes->size(); i++)
            {
               m_filter.add(nodes->get(i)->getId());
            }
            delete nodes;
         }
      }
   }
   FillMessageCallbackData data;
   data.msg = msg;
   data.filter = (obj != NULL) ? &m_filter : NULL;
   data.userId = userId;
   data.patchPanelId = patchPannelId;
   data.objCount = 0;
   data.base = VID_LINK_LIST_BASE;
   s_physicalLinks.forEach(FillMessageCallback, &data);
   msg->setField(VID_LINK_COUNT, data.objCount);
}

/**
 * Find objects, that should be deleted
 */
bool FindPLinksForDeletionCallback(std::shared_ptr<PhysicalLink> *link, void *data)
{
   UINT32 *id = reinterpret_cast<UINT32 *>(data);
   if((*link)->getLeftObjectId() == *id || (*link)->getRightObjectId() == *id)
   {
      return true;
   }
   return false;
}

/**
 * Find objects, that should be deleted
 */
bool FindPLinksForDeletionCallback2(std::shared_ptr<PhysicalLink> *link, void *data)
{
   std::pair<UINT32, UINT32> *pair = reinterpret_cast<std::pair<UINT32, UINT32> *>(data);
   if ((*link)->getLeftObjectId() == pair->first || (*link)->getRightObjectId() == pair->first)
   {
      if ((*link)->getLeftPatchPannelId() == pair->second || (*link)->getRightPatchPanelId() == pair->second)
         return true;
   }
   return false;
}

/**
 * Delete physical links on object delete
 * Should be passed object id: interface or rack
 */
void DeleteObjectFromPhysicalLinks(UINT32 id)
{
   ObjectArray<shared_ptr<PhysicalLink>> *objectsForDeletion = s_physicalLinks.findObjects(FindPLinksForDeletionCallback, &id);
   bool modified = false;
   for(int i = 0; i < objectsForDeletion->size(); i++)
   {
      ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, DeletePhysicalLink, new UINT32((*objectsForDeletion->get(i))->getId()));
      s_physicalLinks.remove((*objectsForDeletion->get(i))->getId());
      modified = true;
   }

   if(modified)
      NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);

   delete objectsForDeletion;
}

/**
 * Delete physical links on patch panel delete
 * Should be passed rack id and patch panel id
 */
void DeletePatchPanelFromPhysicalLinks(UINT32 rackId, UINT32 patchPannelId)
{
   std::pair<UINT32, UINT32> pair = std::pair<UINT32, UINT32>(rackId, patchPannelId);
   ObjectArray<shared_ptr<PhysicalLink>> *objectsForDeletion = s_physicalLinks.findObjects(FindPLinksForDeletionCallback2, &pair);
   bool modified = false;
   for(int i = 0; i < objectsForDeletion->size(); i++)
   {
      ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, DeletePhysicalLink, new UINT32((*objectsForDeletion->get(i))->getId()));
      s_physicalLinks.remove((*objectsForDeletion->get(i))->getId());
      modified = true;
   }

   if(modified)
      NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);

   delete objectsForDeletion;
}

/**
 * Check if there is access on object
 */
static bool CheckAccess(UINT32 objId, UINT32 userId)
{
   bool hasAccess = false;

   NetObj *obj = FindObjectById(objId);
   if(obj != NULL)
      hasAccess = obj->checkAccessRights(userId, OBJECT_ACCESS_READ);

   return hasAccess;
}

/**
 * Add new physical link
 */
bool AddLink(NXCPMessage *msg, UINT32 userId)
{
   auto link = make_shared<PhysicalLink>(msg);

   bool accessLeft = CheckAccess(link->getLeftObjectId(), userId);
   bool accessRight = CheckAccess(link->getRightObjectId(), userId);

   if (!accessLeft || !accessRight)
   {
      return false;
   }

   if (link->getId() != 0)
   {
      std::shared_ptr<PhysicalLink> oldLink = *s_physicalLinks.get(link->getId());
      if(!CheckAccess(oldLink->getLeftObjectId(), userId) || !CheckAccess(oldLink->getRightObjectId(), userId))
      {
         return false;
      }
   }

   if(link->getId() == 0)
      link->updateId();
   else
      s_physicalLinks.remove(link->getId());


   s_physicalLinks.put(link->getId(), new shared_ptr<PhysicalLink>(link));
   NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);

   ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, link, &PhysicalLink::saveToDatabase);
   return true;
}

/**
 * Delete physical link
 */
bool DeleteLink(UINT32 id, UINT32 userId)
{
   std::shared_ptr<PhysicalLink> link = *s_physicalLinks.get(id);
   bool accessLeft = CheckAccess(link->getLeftObjectId(), userId);
   bool accessRight = CheckAccess(link->getRightObjectId(), userId);

   if(!accessLeft || !accessRight)
   {
      return false;
   }

   s_physicalLinks.remove(id);
   NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);

   ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, DeletePhysicalLink, new UINT32(id));
   return true;
}
