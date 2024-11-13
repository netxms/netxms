/*
** NetXMS - Network Management System
** Copyright (C) 2019-2024 Raden Solutions
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
   uint32_t m_id;
   TCHAR *m_description;
   uint32_t m_leftInterfaceParent;
   uint32_t m_leftObjectId;
   uint32_t m_leftPatchPannelId;
   uint32_t m_leftPortNumber;
   bool m_leftFront; //true for front false for back
   uint32_t m_rightObjectId;
   uint32_t m_rightInterfaceParent;
   uint32_t m_rightPatchPannelId;
   uint32_t m_rightPortNumber;
   bool m_rightFront; //true for front false for back

public:
   PhysicalLink(DB_RESULT hResult, int row);
   PhysicalLink(const NXCPMessage& msg);
   PhysicalLink();
   ~PhysicalLink()
   {
      MemFree(m_description);
   }

   uint32_t getId() const { return m_id; }
   uint32_t getLeftObjectId() const { return m_leftObjectId; }
   uint32_t getLeftParentId() const { return m_leftInterfaceParent; }
   uint32_t getLeftPatchPanelId() const { return m_leftPatchPannelId; }
   uint32_t getLeftPortNumber() const { return m_leftPortNumber; }
   bool isLeftFront() const { return m_leftFront; }
   uint32_t getRightObjectId() const { return m_rightObjectId; }
   uint32_t getRightParentId() const { return m_rightInterfaceParent; }
   uint32_t getRightPatchPanelId() const { return m_rightPatchPannelId; }
   uint32_t getRightPortNumber() const { return m_rightPortNumber; }
   bool isRightFront() const { return m_rightFront; }

   json_t *toJson() const;
   void saveToDatabase();
   uint32_t fillMessage(NXCPMessage *pMsg, uint32_t base, bool includeLeft, bool includeRight, bool rightFirst) const;
   void updateId() { m_id = CreateUniqueId(IDG_PHYSICAL_LINK); }
};

/**
 * Index of all physical links
 */
static SharedPointerIndex<PhysicalLink> s_physicalLinks;

/**
 * Physical link constructor from database
 */
PhysicalLink::PhysicalLink(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_description = DBGetField(hResult, row, 1, nullptr, 0);

   m_leftObjectId = DBGetFieldLong(hResult, row, 2);
   shared_ptr<NetObj> object = FindObjectById(m_leftObjectId, OBJECT_INTERFACE);
   m_leftInterfaceParent = (object != nullptr) ? static_cast<Interface&>(*object).getParentNodeId() : 0;
   m_leftPatchPannelId = DBGetFieldLong(hResult, row, 3);
   m_leftPortNumber = DBGetFieldLong(hResult, row, 4);
   m_leftFront = DBGetFieldLong(hResult, row, 5) ? true : false;

   m_rightObjectId = DBGetFieldLong(hResult, row, 6);
   object = FindObjectById(m_rightObjectId, OBJECT_INTERFACE);
   m_rightInterfaceParent = (object != nullptr) ? static_cast<Interface&>(*object).getParentNodeId() : 0;
   m_rightPatchPannelId = DBGetFieldLong(hResult, row, 7);
   m_rightPortNumber = DBGetFieldLong(hResult, row, 8);
   m_rightFront = DBGetFieldLong(hResult, row, 9) ? true : false;
}

/**
 * Physical link constructor from message
 */
PhysicalLink::PhysicalLink(const NXCPMessage& msg)
{
   long base = VID_LINK_LIST_BASE;
   m_id = msg.getFieldAsInt32(base++);
   m_description = msg.getFieldAsString(base++);

   m_leftObjectId = msg.getFieldAsInt32(base++);
   shared_ptr<NetObj> object = FindObjectById(m_leftObjectId, OBJECT_INTERFACE);
   m_leftInterfaceParent = (object != nullptr) ? static_cast<Interface&>(*object).getParentNodeId() : 0;
   m_leftPatchPannelId = msg.getFieldAsInt32(base++);
   m_leftPortNumber = msg.getFieldAsInt32(base++);
   m_leftFront = msg.getFieldAsBoolean(base++);

   m_rightObjectId = msg.getFieldAsInt32(base++);
   object = FindObjectById(m_rightObjectId, OBJECT_INTERFACE);
   m_rightInterfaceParent = (object != nullptr) ? static_cast<Interface&>(*object).getParentNodeId() : 0;
   m_rightPatchPannelId = msg.getFieldAsInt32(base++);
   m_rightPortNumber = msg.getFieldAsInt32(base++);
   m_rightFront = msg.getFieldAsBoolean(base++);
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
         _T("right_object_id"), _T("right_patch_pannel_id"), _T("right_port_number"), _T("right_front"), nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("physical_links"), _T("id"), m_id, columns);

   if (hStmt != nullptr)
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
static void FillOneLinkSide(NXCPMessage *pMsg, uint32_t base, uint32_t objectId, uint32_t patchPannelId, uint32_t portNum, bool side)
{
   pMsg->setField(base++, objectId);
   pMsg->setField(base++, patchPannelId);
   pMsg->setField(base++, portNum);
   pMsg->setField(base++, side);
}

/**
 * Fill message with link data
 */
uint32_t PhysicalLink::fillMessage(NXCPMessage *pMsg, uint32_t base, bool includeLeft, bool includeRight, bool rightFirst) const
{
   pMsg->setField(base++, m_id);
   pMsg->setField(base++, m_description);
   if (rightFirst)
   {
      if (includeRight)
         FillOneLinkSide(pMsg, base, m_rightObjectId, m_rightPatchPannelId, m_rightPortNumber, m_rightFront);
      else
         FillOneLinkSide(pMsg, base, 0, 0, 0, false);
      base += 4;

      if (includeLeft)
         FillOneLinkSide(pMsg, base, m_leftObjectId, m_leftPatchPannelId, m_leftPortNumber, m_leftFront);
      else
         FillOneLinkSide(pMsg, base, 0, 0, 0, false);
      base += 4;
   }
   else
   {
      if (includeLeft)
         FillOneLinkSide(pMsg, base, m_leftObjectId, m_leftPatchPannelId, m_leftPortNumber, m_leftFront);
      else
         FillOneLinkSide(pMsg, base, 0, 0, 0, false);
      base += 4;

      if (includeRight)
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
   if (result == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   int count = DBGetNumRows(result);
   for(int i = 0; i < count; i++)
   {
      auto link = make_shared<PhysicalLink>(result, i);
      s_physicalLinks.put(link->getId(), link);
   }
   DBFreeResult(result);

   DBConnectionPoolReleaseConnection(hdb);
   return true;
}

/**
 * Delete physical link from database
 */
static void DeletePhysicalLinkFromDB(void *linkId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM physical_links WHERE id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_CTYPE_UINT32, CAST_FROM_POINTER(linkId, uint32_t));
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Fill message callback data structure
 */
struct FillMessageCallbackData
{
   NXCPMessage *msg;
   IntegerArray<UINT32> *filter;
   uint32_t userId;
   uint32_t patchPanelId;
   uint32_t objCount;
   uint32_t base;
};

/**
 * Fill message with link information
 */
static EnumerationCallbackResult FillMessageCallback(PhysicalLink *link, FillMessageCallbackData *data)
{
   bool rightFirst = false;
   if (data->filter != nullptr) //filter objects
   {
      rightFirst = data->filter->indexOf(link->getRightObjectId()) != -1 || data->filter->indexOf(link->getRightParentId()) != -1;
      if (data->filter->indexOf(link->getLeftObjectId()) == -1 && !rightFirst && data->filter->indexOf(link->getLeftParentId()) == -1)
         return _CONTINUE;

      if (data->patchPanelId != 0 && link->getLeftPatchPanelId() != data->patchPanelId && link->getRightPatchPanelId() != data->patchPanelId)
         return _CONTINUE;
   }

   // check access
   shared_ptr<NetObj> object = FindObjectById(link->getLeftObjectId());
   bool accessLeft;
   bool accessRight;
   if (object != nullptr)
      accessLeft = object->checkAccessRights(data->userId, OBJECT_ACCESS_READ);
   else
      accessLeft = false;

   object = FindObjectById(link->getRightObjectId());
   if (object != nullptr)
      accessRight = object->checkAccessRights(data->userId, OBJECT_ACCESS_READ);
   else
      accessRight = false;

   if (accessLeft || accessRight)
   {
      link->fillMessage(data->msg, data->base, accessLeft, accessRight, rightFirst);
      data->base += 20;
      data->objCount++;
   }

   return _CONTINUE;
}

/**
 * Get physical links for object
 */
void GetObjectPhysicalLinks(const NetObj *object, uint32_t userId, uint32_t patchPanelId, NXCPMessage *msg)
{
   IntegerArray<uint32_t> m_filter;
   if (object != nullptr)
   {
      m_filter.add(object->getId());
      if ((object->getObjectClass() == OBJECT_RACK) && (patchPanelId == 0))
      {
         unique_ptr<SharedObjectArray<NetObj>> nodes = object->getChildren(OBJECT_NODE);
         for(int i = 0; i < nodes->size(); i++)
         {
            m_filter.add(nodes->get(i)->getId());
         }
      }
   }
   FillMessageCallbackData data;
   data.msg = msg;
   data.filter = (object != nullptr) ? &m_filter : nullptr;
   data.userId = userId;
   data.patchPanelId = patchPanelId;
   data.objCount = 0;
   data.base = VID_LINK_LIST_BASE;
   s_physicalLinks.forEach(FillMessageCallback, &data);
   msg->setField(VID_LINK_COUNT, data.objCount);
}

static StringBuffer GetRackComment(shared_ptr<Rack> rack, uint32_t patchPannelId, uint32_t portNumber)
{
   StringBuffer tmp;
   tmp.append(rack->getName());
   tmp.append(_T("/"));
   tmp.append(rack->getRackPasiveElementDescription(patchPannelId));
   tmp.append(_T("/"));
   tmp.append(portNumber);
   return tmp;
}

/**
 * Wrapper for link part
 */
class LinkPart
{
   shared_ptr<PhysicalLink> link;
   bool right;
public:

   LinkPart(const shared_ptr<PhysicalLink> &l, bool isRight) : link(l)
   {
      right = isRight;
   }

   /**
    * Change reference to link and side
    */
   void update(const shared_ptr<PhysicalLink> &l, bool isRight)
   {
      link = l;
      right = isRight;
   }

   uint32_t getParentId() const
   {
      return right ? link->getRightParentId() : link->getLeftParentId();
   }
   uint32_t getObjectId() const
   {
      return right ? link->getRightObjectId() : link->getLeftObjectId();
   }
   uint32_t getPatchPanelId() const
   {
      return right ? link->getRightPatchPanelId() : link->getLeftPatchPanelId();
   }
   uint32_t getPortNumber() const
   {
      return right ? link->getRightPortNumber() : link->getLeftPortNumber();
   }
   bool isFront() const
   {
      return right ? link->isRightFront() :  link->isLeftFront();
   }
};

/**
 * Go thought all patch panel connections to collect rout information in routeInformation and return end node
 */
bool GetEndNode(LinkPart *part, StringBuffer *routeInformation)
{
   if (!routeInformation->isEmpty())
   {
      routeInformation->append(_T(" -> "));
   }
   shared_ptr<NetObj> remoteObject = FindObjectById(part->getObjectId(), OBJECT_RACK);
   if (remoteObject != nullptr)
   {
      routeInformation->append(GetRackComment(static_pointer_cast<Rack>(remoteObject), part->getPatchPanelId(), part->getPortNumber()));
   }
   else
   {
      routeInformation->append(_T("["));
      routeInformation->append(part->getObjectId());
      routeInformation->append(_T("]"));
   }

   bool found = false;
   unique_ptr<SharedObjectArray<PhysicalLink>> links = s_physicalLinks.getAll();
   for (const shared_ptr<PhysicalLink> &link : *links)
   {
      if (link->getLeftObjectId() == part->getObjectId() && link->getLeftPatchPanelId() == part->getPatchPanelId()
            && link->getLeftPortNumber() == part->getPortNumber() && link->isLeftFront() != part->isFront())
      {
         part->update(link, true);
         found = true;
         break;
      }
      if (link->getRightObjectId() == part->getObjectId() && link->getRightPatchPanelId() == part->getPatchPanelId()
            && link->getRightPortNumber() == part->getPortNumber() && (link->isRightFront() != part->isFront()))
      {
         part->update(link, false);
         found = true;
         break;
      }
   }

   if (!found)
      return false;

   if (part->getParentId() == 0)
   {
      return GetEndNode(part, routeInformation);
   }
   else
   {
      return true;
   }
}

/**
 * Get physical links for node
 */
ObjectArray<L1_NEIGHBOR_INFO> GetL1Neighbors(const Node *root)
{
   ObjectArray<L1_NEIGHBOR_INFO> result(0, 16, Ownership::True);
   uint32_t id = root->getId();
   unique_ptr<SharedObjectArray<PhysicalLink>> links = s_physicalLinks.findAll([id](PhysicalLink *link) -> bool { return (link->getLeftParentId() == id) || (link->getRightParentId() == id); });
   for (const shared_ptr<PhysicalLink> &link : *links)
   {
      LinkPart localLinkPart(link, false);
      LinkPart remoteLinkPart(link, true);

      if (link->getRightParentId() == root->getId()) //swap links with direction if required
      {
         localLinkPart = LinkPart(link, true);
         remoteLinkPart = LinkPart(link, false);
      }

      StringBuffer path;
      if (remoteLinkPart.getParentId() != 0 || GetEndNode(&remoteLinkPart, &path))
      {
         L1_NEIGHBOR_INFO *info = new L1_NEIGHBOR_INFO();
         info->ifLocal = localLinkPart.getObjectId();
         info->ifRemote = remoteLinkPart.getObjectId();
         info->objectId = remoteLinkPart.getParentId();
         info->routeInfo = path;

         result.add(info);
      }
   }

   return result;
}

/**
 * Find objects, that should be deleted
 */
static bool FindPLinksForDeletionCallback(PhysicalLink *link, UINT32 *id)
{
   return (link->getLeftObjectId() == *id) || (link->getRightObjectId() == *id);
}

/**
 * Find objects, that should be deleted
 */
static bool FindPLinksForDeletionCallback2(PhysicalLink *link, std::pair<UINT32, UINT32> *context)
{
   return ((link->getLeftObjectId() == context->first) || (link->getRightObjectId() == context->first)) &&
          ((link->getLeftPatchPanelId() == context->second) || (link->getRightPatchPanelId() == context->second));
}

/**
 * Delete physical links on object delete
 * Should be passed object id: interface or rack
 */
void DeleteObjectFromPhysicalLinks(uint32_t id)
{
   unique_ptr<SharedObjectArray<PhysicalLink>> objectsForDeletion = s_physicalLinks.findAll(FindPLinksForDeletionCallback, &id);

   bool modified = false;
   for(int i = 0; i < objectsForDeletion->size(); i++)
   {
      shared_ptr<PhysicalLink> link = objectsForDeletion->getShared(i);
      ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, DeletePhysicalLinkFromDB, CAST_TO_POINTER(link->getId(), void*));
      s_physicalLinks.remove(link->getId());
      modified = true;
   }

   if (modified)
      NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);
}

/**
 * Delete physical links on patch panel delete
 * Should be passed rack id and patch panel id
 */
void DeletePatchPanelFromPhysicalLinks(uint32_t rackId, uint32_t patchPanelId)
{
   std::pair<uint32_t, uint32_t> context(rackId, patchPanelId);
   unique_ptr<SharedObjectArray<PhysicalLink>> objectsForDeletion = s_physicalLinks.findAll(FindPLinksForDeletionCallback2, &context);

   bool modified = false;
   for(int i = 0; i < objectsForDeletion->size(); i++)
   {
      shared_ptr<PhysicalLink> link = objectsForDeletion->getShared(i);
      ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, DeletePhysicalLinkFromDB, CAST_TO_POINTER(link->getId(), void*));
      s_physicalLinks.remove(link->getId());
      modified = true;
   }

   if (modified)
      NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);
}

/**
 * Check if there is access on object
 */
static inline bool CheckAccess(uint32_t objId, uint32_t userId)
{
   shared_ptr<NetObj> object = FindObjectById(objId);
   return (object != nullptr) ? object->checkAccessRights(userId, OBJECT_ACCESS_READ) : true;
}

/**
 * Data for endpoint search callback
 */
struct EndPointSearchData
{
   PhysicalLink *link;
   NetObj *leftObject;
   NetObj *rightObject;
};

/**
 * Compare two link sides
 */
static bool CompareLinkSides(UINT32 objectId1, UINT32 patchPanelId1, UINT32 port1, bool frontSide1, UINT32 objectId2, UINT32 patchPanelId2, UINT32 port2, bool frontSide2, bool isRack)
{
   if(objectId1 == objectId2)
   {
      if(isRack)
      {
         if(patchPanelId1 == patchPanelId2 && port1 == port2 && frontSide1 == frontSide2)
            return true;
      }
      else
      {
         return true;
      }
   }
   return false;
}

/**
 * Find object with the same endpoint
 */
static bool FindPLinksWithSameEndpoint(PhysicalLink *link, EndPointSearchData *context)
{
   PhysicalLink *slink = context->link;
   if (link->getId() == slink->getId())
      return false;

   if (CompareLinkSides(link->getLeftObjectId(), link->getLeftPatchPanelId(), link->getLeftPortNumber(), link->isLeftFront(),
          slink->getLeftObjectId(), slink->getLeftPatchPanelId(), slink->getLeftPortNumber(), slink->isLeftFront(), context->leftObject->getObjectClass() == OBJECT_RACK) ||
       CompareLinkSides(link->getRightObjectId(), link->getRightPatchPanelId(), link->getRightPortNumber(), link->isRightFront(),
          slink->getLeftObjectId(), slink->getLeftPatchPanelId(), slink->getLeftPortNumber(), slink->isLeftFront(), context->leftObject->getObjectClass() == OBJECT_RACK) ||
       CompareLinkSides(link->getLeftObjectId(), link->getLeftPatchPanelId(), link->getLeftPortNumber(), link->isLeftFront(),
          slink->getRightObjectId(), slink->getRightPatchPanelId(), slink->getRightPortNumber(), slink->isRightFront(), context->rightObject->getObjectClass() == OBJECT_RACK) ||
       CompareLinkSides(link->getRightObjectId(), link->getRightPatchPanelId(), link->getRightPortNumber(), link->isRightFront(),
          slink->getRightObjectId(), slink->getRightPatchPanelId(), slink->getRightPortNumber(), slink->isRightFront(), context->rightObject->getObjectClass() == OBJECT_RACK) )
   {
      return true;
   }
   return false;
}

/**
 * Add new physical link
 */
uint32_t AddPhysicalLink(const NXCPMessage& msg, uint32_t userId)
{
   auto link = make_shared<PhysicalLink>(msg);

   bool accessLeft = false;
   shared_ptr<NetObj> leftObject = FindObjectById(link->getLeftObjectId());
   if (leftObject != nullptr)
   {
      accessLeft = leftObject->checkAccessRights(userId, OBJECT_ACCESS_READ);
   }

   bool accessRight = false;
   shared_ptr<NetObj> rightObject = FindObjectById(link->getRightObjectId());
   if (rightObject != nullptr)
   {
      accessRight = rightObject->checkAccessRights(userId, OBJECT_ACCESS_READ);
   }

   if (!accessLeft || !accessRight)
      return RCC_ACCESS_DENIED;

   if (link->getId() != 0)
   {
      shared_ptr<PhysicalLink> oldLink = s_physicalLinks.get(link->getId());
      if (!CheckAccess(oldLink->getLeftObjectId(), userId) || !CheckAccess(oldLink->getRightObjectId(), userId))
         return RCC_ACCESS_DENIED;
   }

   //check for duplicate
   EndPointSearchData epd;
   epd.link = link.get();
   epd.leftObject = leftObject.get();
   epd.rightObject = rightObject.get();
   unique_ptr<SharedObjectArray<PhysicalLink>> matchedObjects = s_physicalLinks.findAll(FindPLinksWithSameEndpoint, &epd);
   if (matchedObjects->size() > 0 ||
         CompareLinkSides(link->getRightObjectId(), link->getRightPatchPanelId(), link->getRightPortNumber(), link->isRightFront(),
                  link->getLeftObjectId(), link->getLeftPatchPanelId(), link->getLeftPortNumber(), link->isLeftFront(), rightObject->getObjectClass() == OBJECT_RACK))
   {
      return RCC_ENDPOINT_ALREADY_IN_USE;
   }

   if (link->getId() == 0)
      link->updateId();
   else
      s_physicalLinks.remove(link->getId());

   s_physicalLinks.put(link->getId(), link);
   NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);

   ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, link, &PhysicalLink::saveToDatabase);
   return RCC_SUCCESS;
}

/**
 * Delete physical link
 */
bool DeletePhysicalLink(uint32_t id, uint32_t userId)
{
   shared_ptr<PhysicalLink> link = s_physicalLinks.get(id);
   if (link == nullptr)
      return false;

   bool accessLeft = CheckAccess(link->getLeftObjectId(), userId);
   bool accessRight = CheckAccess(link->getRightObjectId(), userId);
   if (!accessLeft || !accessRight)
      return false;

   s_physicalLinks.remove(id);
   NotifyClientSessions(NX_NOTIFY_PHYSICAL_LINK_UPDATE, 0);

   ThreadPoolExecuteSerialized(g_clientThreadPool, PL_THREAD_KEY, DeletePhysicalLinkFromDB, CAST_TO_POINTER(id, void*));
   return true;
}
