/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2020 Raden Solutions
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
** File: nobject.cpp
**
**/

#include <nxsrvapi.h>
#include <netxms-regex.h>

/**
 * Default constructor for the class
 */
NObject::NObject()
{
   m_id = 0;
   m_name[0] = 0;
   m_customAttributes = new StringObjectMap<CustomAttribute>(Ownership::True);
   m_childList = new ObjectArray<NObject>(0, 16, Ownership::False);
   m_parentList = new ObjectArray<NObject>(4, 4, Ownership::False);
   m_customAttributeLock = MutexCreateFast();
   m_rwlockParentList = RWLockCreate();
   m_rwlockChildList = RWLockCreate();
}

/**
 * Destructor
 */
NObject::~NObject()
{
   delete m_customAttributes;
   delete m_childList;
   delete m_parentList;
   MutexDestroy(m_customAttributeLock);
   RWLockDestroy(m_rwlockParentList);
   RWLockDestroy(m_rwlockChildList);
}

/**
 * Clear parent list. Should be called only when parent list is write locked.
 * This method will not check for custom attribute inheritance changes.
 */
void NObject::clearParentList()
{
   m_parentList->clear();
}

/**
 * Clear children list. Should be called only when children list is write locked.
 */
void NObject::clearChildList()
{
   m_childList->clear();
}

/**
 * Add reference to the new child object
 */
void NObject::addChild(NObject *object)
{
   lockChildList(true);
   if (m_childList->contains(object))
   {
      unlockChildList();
      return;     // Already in the child list
   }
   m_childList->add(object);
   unlockChildList();

   // Update custom attribute inheritance
   ObjectArray<std::pair<String, UINT32>> updateList(0, 16, Ownership::True);
   lockCustomAttributes();
   Iterator<std::pair<const TCHAR*, CustomAttribute*>> *iterator = m_customAttributes->iterator();
   while(iterator->hasNext())
   {
      std::pair<const TCHAR*, CustomAttribute*> *pair = iterator->next();
      if(pair->second->isInheritable())
         updateList.add(new std::pair<String, UINT32>(pair->first, pair->second->isRedefined() ? m_id : pair->second->sourceObject));
   }
   delete iterator;
   unlockCustomAttributes();

   for(int i = 0; i < updateList.size(); i++)
      object->setCustomAttribute(updateList.get(i)->first, getCustomAttribute(updateList.get(i)->first), updateList.get(i)->second);
}

/**
 * Add reference to parent object
 */
void NObject::addParent(NObject *object)
{
   lockParentList(true);
   if (m_parentList->contains(object))
   {
      unlockParentList();
      return;     // Already in the parents list
   }
   m_parentList->add(object);
   unlockParentList();
}

/**
 * Delete reference to child object
 */
void NObject::deleteChild(NObject *object)
{
   lockChildList(true);
   m_childList->remove(object);
   unlockChildList();
}

/**
 * Delete reference to parent object
 */
void NObject::deleteParent(NObject *object)
{
   lockParentList(true);
   bool success = m_parentList->remove(object);
   unlockParentList();

   if (success)
   {
      StringList removeList;

      lockCustomAttributes();
      Iterator<std::pair<const TCHAR*, CustomAttribute*>> *iterator = m_customAttributes->iterator();
      while(iterator->hasNext())
      {
         std::pair<const TCHAR*, CustomAttribute*> *pair = iterator->next();
         if (pair->second->isInherited() && !isParent(pair->second->sourceObject))
            removeList.add(pair->first);
      }
      delete iterator;
      unlockCustomAttributes();

      for(int i = 0; i < removeList.size(); i++)
         updateOrDeleteCustomAttributeOnParentRemove(removeList.get(i));
   }
}

/**
 * Check if given object is an our child (possibly indirect, i.e child of child)
 *
 * @param id object ID to test
 */
bool NObject::isChild(UINT32 id)
{
   bool result = isDirectChild(id);

   // If given object is not in child list, check if it is indirect child
   if (!result)
   {
      lockChildList(false);
      for(int i = 0; i < m_childList->size(); i++)
         if (m_childList->get(i)->isChild(id))
         {
            result = true;
            break;
         }
      unlockChildList();
   }

   return result;
}

/**
 * Check if given object is our direct child
 *
 * @param id object ID to test
 */
bool NObject::isDirectChild(UINT32 id)
{
   // Check for our own ID (object ID should never change, so we may not lock object's data)
   if (m_id == id)
      return true;

   bool result = false;

   // First, walk through our own child list
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
      if (m_childList->get(i)->getId() == id)
      {
         result = true;
         break;
      }
   unlockChildList();

   return result;
}

/**
 * Check if given object is an our child (possibly indirect, i.e child of child)
 *
 * @param id object ID to test
 */
bool NObject::isParent(UINT32 id)
{
   bool result = isDirectParent(id);

   // If given object is not in parent list, check if it is indirect parent
   if (!result)
   {
      lockParentList(false);
      for(int i = 0; i < m_parentList->size(); i++)
         if (m_parentList->get(i)->isParent(id))
         {
            result = true;
            break;
         }
      unlockParentList();
   }

   return result;
}

/**
 * Check if given object is our direct parent
 *
 * @param id object ID to test
 */
bool NObject::isDirectParent(UINT32 id)
{
   // Check for our own ID (object ID should never change, so we may not lock object's data)
   if (m_id == id)
      return true;

   bool result = false;

   // First, walk through our own Parent list
   lockParentList(false);
   for(int i = 0; i < m_parentList->size(); i++)
      if (m_parentList->get(i)->getId() == id)
      {
         result = true;
         break;
      }
   unlockParentList();

   return result;
}

/**
 * Get custom attribute value from parent by name
 */
SharedString NObject::getCustomAttributeFromParent(const TCHAR *name)
{
   SharedString value;
   lockParentList(false);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      value = m_parentList->get(i)->getCustomAttribute(name);
      if (!value.isNull())
         break;
   }
   unlockParentList();
   return value;
}

/**
 * Get children IDs in printable form
 */
const TCHAR *NObject::dbgGetChildList(TCHAR *szBuffer)
{
   TCHAR *pBuf = szBuffer;
   *pBuf = 0;
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      _sntprintf(pBuf, 10, _T("%d "), m_childList->get(i)->getId());
      while(*pBuf)
         pBuf++;
   }
   unlockChildList();
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}

/**
 * Get parents IDs in printable form
 */
const TCHAR *NObject::dbgGetParentList(TCHAR *szBuffer)
{
   TCHAR *pBuf = szBuffer;
   *pBuf = 0;
   lockParentList(false);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      _sntprintf(pBuf, 10, _T("%d "), m_parentList->get(i)->getId());
      while(*pBuf)
         pBuf++;
   }
   unlockParentList();
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}

static UINT32 GetResultingFlag(UINT32 flags, StateChange change, UINT32 flag)
{
   if (change == StateChange::IGNORE)
      return flags;
   else if (change == StateChange::CLEAR)
      return flags &= ~flag;
   else
      return flags |= flag;
}

/**
 * Set custom attribute
 */
void NObject::setCustomAttribute(const TCHAR *name, SharedString value, StateChange inheritable)
{
   lockCustomAttributes();
   CustomAttribute *curr = m_customAttributes->get(name);
   UINT32 source = 0;
   bool remove = false;

   if (curr == nullptr)
   {
      curr = new CustomAttribute(value, GetResultingFlag(0, inheritable, CAF_INHERITABLE));
      m_customAttributes->set(name, curr);
      onCustomAttributeChange();
   }
   else if (_tcscmp(curr->value, value) || (curr->flags != GetResultingFlag(curr->flags, inheritable, CAF_INHERITABLE)))
   {
      remove = (curr->flags != GetResultingFlag(curr->flags, inheritable, CAF_INHERITABLE));
      curr->value = value;
      if (curr->isInherited())
         curr->flags = CAF_INHERITABLE | CAF_REDEFINED;
      else
         curr->flags = GetResultingFlag(curr->flags, inheritable, CAF_INHERITABLE);
      onCustomAttributeChange();
   }

   bool inherit = curr->isInheritable();
   if (inherit)
      source = (curr->isRedefined() || !curr->isInherited()) ? m_id : curr->sourceObject;
   unlockCustomAttributes();

   if (inherit)
      populate(name, value, source);
   else if (remove)
      populateRemove(name);
}

/**
 * Set custom attribute
 */
void NObject::setCustomAttribute(const TCHAR *name, SharedString value, UINT32 parent)
{
   lockCustomAttributes();

   CustomAttribute *curr = m_customAttributes->get(name);
   bool callPopulate = true;
   if (curr == nullptr)
   {
      curr = new CustomAttribute(value, CAF_INHERITABLE, parent);
      m_customAttributes->set(name, curr);
      onCustomAttributeChange();
   }
   else if (_tcscmp(curr->value, value) || (curr->sourceObject == 0))
   {
      callPopulate = (curr->isInheritable() && !curr->isRedefined()) || !curr->isInherited();
      curr->flags |= CAF_INHERITABLE;
      if (curr->sourceObject == 0) //If already defined, but not by inheritance
         curr->flags |= CAF_REDEFINED;

      if (!curr->isRedefined())
         curr->value = value;

      curr->sourceObject = parent;
      onCustomAttributeChange();
   }

   uint32_t source = parent;
   if (curr->isRedefined())
   {
      source = m_id;
      value = curr->value;
   }

   unlockCustomAttributes();

   if (callPopulate)
      populate(name, value, source);
}

/**
 * Set custom attribute value from INT32
 */
void NObject::setCustomAttribute(const TCHAR *key, INT32 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%d"), (int)value);
   setCustomAttribute(key, buffer, StateChange::IGNORE);
}

/**
 * Set custom attribute value from UINT32
 */
void NObject::setCustomAttribute(const TCHAR *key, UINT32 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%u"), (unsigned int)value);
   setCustomAttribute(key, buffer, StateChange::IGNORE);

}

/**
 * Set custom attribute value from INT64
 */
void NObject::setCustomAttribute(const TCHAR *key, INT64 value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, INT64_FMT, value);
   setCustomAttribute(key, buffer, StateChange::IGNORE);
}

/**
 * Set custom attribute value from UINT64
 */
void NObject::setCustomAttribute(const TCHAR *key, UINT64 value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, UINT64_FMT, value);
   setCustomAttribute(key, buffer, StateChange::IGNORE);
}

/**
 * Update inherited custom attribute for child node
 */
void NObject::populate(const TCHAR *name, SharedString value, UINT32 source)
{
   lockChildList(true);
   for(int i = 0; i < m_childList->size(); i++)
   {
      m_childList->get(i)->setCustomAttribute(name, value, source);
   }
   unlockChildList();
}

/**
 * Update inherited custom attribute remove for child node
 */
void NObject::populateRemove(const TCHAR *name)
{
   lockChildList(true);
   for(int i = 0; i < m_childList->size(); i++)
   {
      m_childList->get(i)->deletePopulatedCustomAttribute(name);
   }
   unlockChildList();
}

/**
 * Set custom attributes from database query
 */
void NObject::setCustomAttributesFromDatabase(DB_RESULT hResult)
{
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      TCHAR *name = DBGetField(hResult, i, 0, nullptr, 0);
      if (name != nullptr)
      {
         TCHAR *value = DBGetField(hResult, i, 1, nullptr, 0);
         if (value != nullptr)
         {
            CustomAttribute *curr = new CustomAttribute(value, DBGetFieldULong(hResult, i, 2));
            m_customAttributes->setPreallocated(name, curr);
            MemFree(value);
         }
         else
         {
            MemFree(name);
         }
      }
   }
}

/**
 * Set custom attribute from message
 */
bool NObject::setCustomAttributeFromMessage(NXCPMessage *msg, UINT32 base)
{
   bool success = false;
   TCHAR *name = msg->getFieldAsString(base++);
   TCHAR *value = msg->getFieldAsString(base++);
   UINT32 flags = msg->getFieldAsUInt32(base++);
   if ((name != nullptr) && (value != nullptr))
   {
      setCustomAttribute(name, value, (flags & CAF_INHERITABLE) > 0 ? StateChange::SET : StateChange::CLEAR);
      success = true;
   }
   MemFree(name);
   MemFree(value);
   return success;
}

/**
 * Set custom attributes from NXCP message
 */
void NObject::setCustomAttributesFromMessage(NXCPMessage *msg)
{
   StringList existingAttibutes;
   StringList deletionList;
   ObjectArray<std::pair<String, UINT32>> updateList(0, 16, Ownership::True);

   int count = msg->getFieldAsUInt32(VID_NUM_CUSTOM_ATTRIBUTES);
   UINT32 fieldId = VID_CUSTOM_ATTRIBUTES_BASE;
   for(int i = 0; i < count; i++, fieldId += 3)
   {
      if (setCustomAttributeFromMessage(msg, fieldId))
         existingAttibutes.addPreallocated(msg->getFieldAsString(fieldId));
   }

   lockCustomAttributes();
   Iterator<std::pair<const TCHAR*, CustomAttribute*>> *iterator = m_customAttributes->iterator();
   while(iterator->hasNext())
   {
      std::pair<const TCHAR*, CustomAttribute*> *pair = iterator->next();
      if ((pair->second->isInherited() == pair->second->isRedefined()) && !existingAttibutes.contains(pair->first))
      {
         if(pair->second->isRedefined())
         {
            updateList.add(new std::pair<String, UINT32>(pair->first, pair->second->sourceObject));
         }
         else if (pair->second->isInheritable())
         {
            deletionList.add(pair->first);
         }
         iterator->remove();
      }
   }
   delete iterator;
   unlockCustomAttributes();

   for(int i = 0; i < deletionList.size(); i++)
      populateRemove(deletionList.get(i));

   for(int i = 0; i < updateList.size(); i++)
      setCustomAttribute(updateList.get(i)->first, getCustomAttributeFromParent(updateList.get(i)->first), updateList.get(i)->second);
}

/**
 * Delete custom attribute
 */
void NObject::deleteCustomAttribute(const TCHAR *name)
{
   lockCustomAttributes();
   CustomAttribute *ca = m_customAttributes->get(name);
   bool populate = false;
   bool redefined = false;
   uint32_t parent;
   if ((ca != nullptr) && (!ca->isInherited() || ca->isRedefined()))
   {
      populate = ca->isInheritable() && !ca->isRedefined();
      redefined = ca->isRedefined();
      parent = ca->sourceObject;

      if (ca->isRedefined())
      {
         ca->flags &= ~CAF_REDEFINED;
         ca->value = getCustomAttributeFromParent(name);
      }
      else
      {
         m_customAttributes->remove(name);
      }

      onCustomAttributeChange();
   }
   unlockCustomAttributes();

   if (populate)
      populateRemove(name);
   else if (redefined)
      setCustomAttribute(name, getCustomAttributeFromParent(name), parent);
}

/**
 * Delete custom attribute
 */
void NObject::updateOrDeleteCustomAttributeOnParentRemove(const TCHAR *name)
{
   lockCustomAttributes();
   CustomAttribute *ca = m_customAttributes->get(name);
   bool populate = false;
   if (ca != nullptr)
   {
      populate = ca->isInheritable() && !ca->isRedefined();

      if (ca->isRedefined())
      {
         ca->sourceObject = 0;
         ca->flags &= ~CAF_REDEFINED;
      }
      else
      {
         m_customAttributes->remove(name);
      }

      onCustomAttributeChange();
   }
   unlockCustomAttributes();

   if (populate)
      populateRemove(name);
}

/**
 * Delete custom attribute
 */
void NObject::deletePopulatedCustomAttribute(const TCHAR *name)
{
   lockCustomAttributes();
   CustomAttribute *ca = m_customAttributes->get(name);
   bool populate = false;
   if (ca != nullptr)
   {
      if (!ca->isRedefined())
      {
         m_customAttributes->remove(name);
         populate = true;
      }
      else
      {
         ca->sourceObject = 0;
      }

      onCustomAttributeChange();
   }
   unlockCustomAttributes();
   if (populate)
      populateRemove(name);
}

/**
 * Get custom attribute into buffer
 */
TCHAR *NObject::getCustomAttribute(const TCHAR *name, TCHAR *buffer, size_t size) const
{
   TCHAR *result;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(name);
   if (attr != nullptr)
   {
      _tcslcpy(buffer, attr->value, size);
      result = buffer;
   }
   else
   {
      result = nullptr;
   }
   unlockCustomAttributes();
   return result;
}

/**
 * Get custom attribute into buffer
 */
SharedString NObject::getCustomAttribute(const TCHAR *name) const
{
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(name);
   SharedString result = (attr != nullptr) ? attr->value : SharedString();
   unlockCustomAttributes();
   return result;
}

/**
 * Get copy of custom attribute. Returned value must be freed by caller
 */
TCHAR *NObject::getCustomAttributeCopy(const TCHAR *name) const
{
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(name);
   TCHAR *result = (attr != nullptr) ? MemCopyString(attr->value) : nullptr;
   unlockCustomAttributes();
   return result;
}

/**
 * Get custom attribute value by key as INT32
 */
INT32 NObject::getCustomAttributeAsInt32(const TCHAR *key, INT32 defaultValue) const
{
   INT32 result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(key);
   if (attr != nullptr)
      result = _tcstol(attr->value, nullptr, 0);
   unlockCustomAttributes();
   return result;
}

/**
 * Get custom attribute value by key as UINT32
 */
UINT32 NObject::getCustomAttributeAsUInt32(const TCHAR *key, UINT32 defaultValue) const
{
   UINT32 result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(key);
   if (attr != nullptr)
      result = _tcstoul(attr->value, nullptr, 0);
   unlockCustomAttributes();
   return result;
}

/**
 * Get custom attribute value by key as INT64
 */
INT64 NObject::getCustomAttributeAsInt64(const TCHAR *key, INT64 defaultValue) const
{
   INT64 result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(key);
   if (attr != nullptr)
      result = _tcstoll(attr->value, nullptr, 0);
   unlockCustomAttributes();
   return result;

}

/**
 * Get custom attribute value by key as UINT64
 */
UINT64 NObject::getCustomAttributeAsUInt64(const TCHAR *key, UINT64 defaultValue) const
{
   UINT64 result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(key);
   if (attr != nullptr)
      result = _tcstoull(attr->value, nullptr, 0);
   unlockCustomAttributes();
   return result;
}

/**
 * Get custom attribute value by key as double
 */
double NObject::getCustomAttributeAsDouble(const TCHAR *key, double defaultValue) const
{
   double result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(key);
   if (attr != nullptr)
      result = _tcstod(attr->value, nullptr);
   unlockCustomAttributes();
   return result;

}

/**
 * Get custom attribute value by key as bool
 */
bool NObject::getCustomAttributeAsBoolean(const TCHAR *key, bool defaultValue) const
{
   bool result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(key);
   if (attr != nullptr)
   {
      if (!_tcsicmp(attr->value, _T("false")))
         result = false;
      else if (!_tcsicmp(attr->value, _T("true")))
         result = true;
      else
         result = (_tcstoul(attr->value, nullptr, 0) != 0) ? true : false;
   }
   unlockCustomAttributes();
   return result;
}

/**
 * Get all custom attributes matching given filter. Filter can be NULL to return all attributes.
 * Filter arguments: attribute name, attribute value, context
 */
StringMap *NObject::getCustomAttributes(bool (*filter)(const TCHAR *, const CustomAttribute *, void *), void *context) const
{
   StringMap *attributes = new StringMap();
   lockCustomAttributes();
   StructArray<KeyValuePair<CustomAttribute>> *filtered = m_customAttributes->toArray(filter, context);
   for(int i = 0; i < filtered->size(); i++)
      attributes->set(filtered->get(i)->key, filtered->get(i)->value->value);
   delete filtered;
   unlockCustomAttributes();
   return attributes;
}

/**
 * Callback filter for matching attribute name by regular expression
 */
static bool RegExpAttrFilter(const TCHAR *name, const CustomAttribute *value, PCRE *preg)
{
   int ovector[30];
   return _pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(name), static_cast<int>(_tcslen(name)), 0, 0, ovector, 30) >= 0;
}

/**
 * Get all custom attributes matching given regular expression. Regular expression can be NULL to return all attributes.
 * Filter arguments: attribute name, attribute value, context
 */
StringMap *NObject::getCustomAttributes(const TCHAR *regexp) const
{
   if (regexp == nullptr)
      return getCustomAttributes(nullptr, nullptr);

   const char *eptr;
   int eoffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(regexp), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (preg == nullptr)
      return new StringMap();

   StringMap *attributes = new StringMap();
   lockCustomAttributes();
   StructArray<KeyValuePair<CustomAttribute>> *filtered = m_customAttributes->toArray(RegExpAttrFilter, preg);
   for(int i = 0; i < filtered->size(); i++)
      attributes->set(filtered->get(i)->key, filtered->get(i)->value->value);
   delete filtered;
   unlockCustomAttributes();
   _pcre_free_t(preg);
   return attributes;
}

/**
 * Get custom attribute as NXSL value
 */
NXSL_Value *NObject::getCustomAttributeForNXSL(NXSL_VM *vm, const TCHAR *name) const
{
   NXSL_Value *value = nullptr;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes->get(name);
   if (attr != nullptr)
      value = vm->createValue(attr->value);
   unlockCustomAttributes();
   return value;
}

/**
 * Get all custom attributes as NXSL hash map
 */
NXSL_Value *NObject::getCustomAttributesForNXSL(NXSL_VM *vm) const
{
   NXSL_HashMap *map = new NXSL_HashMap(vm);
   lockCustomAttributes();
   StructArray<KeyValuePair<CustomAttribute>> *attributes = m_customAttributes->toArray();
   for(int i = 0; i < attributes->size(); i++)
   {
      KeyValuePair<CustomAttribute> *p = attributes->get(i);
      map->set(p->key, vm->createValue(p->value->value));
   }
   unlockCustomAttributes();
   delete attributes;
   return vm->createValue(map);
}

/**
 * Hook method called after change in custom attributes
 */
void NObject::onCustomAttributeChange()
{
}
