/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: objects.cpp
**
**/

#include "libnxclient.h"
#include <uthash.h>

/**
 * Object cache entry
 */
struct ObjectCacheEntry
{
   UT_hash_handle hh;
   UINT32 id;
   AbstractObject *object;
};

/**
 * Controller constructor
 */
ObjectController::ObjectController(NXCSession *session) : Controller(session)
{
   m_cache = NULL;
   m_cacheLock = MutexCreate();
}

/**
 * Controller destructor
 */
ObjectController::~ObjectController()
{
   ObjectCacheEntry *entry, *tmp;
   HASH_ITER(hh, m_cache, entry, tmp)
   {
      HASH_DEL(m_cache, entry);
      entry->object->decRefCount();
      free(entry);
   }
   MutexDestroy(m_cacheLock);
}

/**
 * Sync all objects
 */
UINT32 ObjectController::sync()
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Sync object set
 */
UINT32 ObjectController::syncObjectSet(UINT32 *idList, size_t length, bool syncComments, UINT16 flags)
{
   // Build request message
   NXCPMessage msg;
   msg.setCode(CMD_GET_SELECTED_OBJECTS);
   msg.setId(m_session->createMessageId());
	msg.setField(VID_SYNC_COMMENTS, (WORD)(syncComments ? 1 : 0));
	msg.setField(VID_FLAGS, (WORD)(flags | OBJECT_SYNC_SEND_UPDATES));	// C library requres objects to go in update messages
   msg.setField(VID_NUM_OBJECTS, length);
	msg.setFieldFromInt32Array(VID_OBJECT_LIST, length, idList);

   // Send request
   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   // Wait for reply
   UINT32 rcc = m_session->waitForRCC(msg.getId());
	if ((rcc == RCC_SUCCESS) && (flags & OBJECT_SYNC_DUAL_CONFIRM))
	{
      rcc = m_session->waitForRCC(msg.getId());
	}
	return rcc;
}

/**
 * Synchronize single object
 * Simple wrapper for syncObjectSet which syncs comments and waits for sync completion
 */
UINT32 ObjectController::syncSingleObject(UINT32 id)
{
	return syncObjectSet(&id, 1, true, OBJECT_SYNC_DUAL_CONFIRM);
}

/**
 * Find object by ID.
 * Caller must decrease reference count for object.
 */
AbstractObject *ObjectController::findObjectById(UINT32 id)
{
   AbstractObject *object = NULL;
   MutexLock(m_cacheLock);
   ObjectCacheEntry *entry;
   HASH_FIND_INT(m_cache, &id, entry);
   if (entry != NULL)
   {
      object = entry->object;
      object->incRefCount();
   }
   MutexUnlock(m_cacheLock);
   return object;
}

/**
 * Abstract object constructor
 */
AbstractObject::AbstractObject(NXCPMessage *msg)
{
   m_id = msg->getFieldAsUInt32(VID_OBJECT_ID);
   msg->getFieldAsBinary(VID_GUID, m_guid, UUID_LENGTH);
   m_class = msg->getFieldAsInt16(VID_OBJECT_CLASS);
   msg->getFieldAsString(VID_OBJECT_NAME, m_name, MAX_OBJECT_NAME);
   m_status = msg->getFieldAsInt16(VID_OBJECT_STATUS);
   m_primaryIP = msg->getFieldAsInetAddress(VID_IP_ADDRESS);
   m_comments = msg->getFieldAsString(VID_COMMENTS);
   m_geoLocation = GeoLocation(*msg);
   m_submapId = msg->getFieldAsUInt32(VID_SUBMAP_ID);

	// Custom attributes
	int count = msg->getFieldAsInt32(VID_NUM_CUSTOM_ATTRIBUTES);
   UINT32 id = VID_CUSTOM_ATTRIBUTES_BASE;
	for(int i = 0; i < count; i++, id += 2)
	{
		m_customAttributes.setPreallocated(msg->getFieldAsString(id), msg->getFieldAsString(id + 1));
	}

   // Parents
	count = msg->getFieldAsInt32(VID_PARENT_CNT);
   m_parents = new IntegerArray<UINT32>(count);
   id = VID_PARENT_ID_BASE;
	for(int i = 0; i < count; i++, id++)
      m_parents->add(msg->getFieldAsUInt32(id));

   // Children
	count = msg->getFieldAsInt32(VID_CHILD_CNT);
   m_children = new IntegerArray<UINT32>(count);
   id = VID_CHILD_ID_BASE;
	for(int i = 0; i < count; i++, id++)
      m_children->add(msg->getFieldAsUInt32(id));
}

/**
 * Abstract object destructor
 */
AbstractObject::~AbstractObject()
{
   safe_free(m_comments);
   delete m_parents;
   delete m_children;
}

/** 
 * Node constructor
 */
Node::Node(NXCPMessage *msg) : AbstractObject(msg)
{
   m_primaryHostname = msg->getFieldAsString(VID_PRIMARY_NAME);
}

/**
 * Node destructor
 */
Node::~Node()
{
   safe_free(m_primaryHostname);
}
