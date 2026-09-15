/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2026 Raden Solutions
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
 * Bit flag mapping for custom attributes
 */
static FlagNameMapping s_customAttrFlagMapping[] =
{
   { CAF_INHERITABLE, "inheritable" },
   { CAF_REDEFINED, "redefined" },
   { CAF_CONFLICT, "conflict" },
   { CAF_JSON, "json" },
   { 0, nullptr }
};

/**
 * Parse text of structured custom attribute value. Returns JSON object or array,
 * or nullptr if text is not a valid JSON object or array.
 */
static json_t *ParseStructuredValue(const wchar_t *text)
{
   if (text == nullptr)
      return nullptr;

   char *utf8 = UTF8StringFromWideString(text);
   json_error_t error;
   json_t *json = json_loads(utf8, 0, &error);
   MemFree(utf8);

   if ((json != nullptr) && !json_is_object(json) && !json_is_array(json))
   {
      json_decref(json);
      json = nullptr;
   }
   return json;
}

/**
 * Serialize JSON object or array into canonical text form (compact, sorted keys)
 * used for storing structured custom attribute values.
 */
static SharedString SerializeStructuredValue(const json_t *json)
{
   char *text = json_dumps(json, JSON_COMPACT | JSON_SORT_KEYS);
   SharedString value(WideStringFromUTF8String(text), Ownership::True);
   MemFree(text);
   return value;
}

/**
 * Get custom attribute value as JSON: parsed object or array for structured values, string otherwise
 */
json_t *CustomAttribute::valueToJson() const
{
   if (isJson())
   {
      json_t *json = ParseStructuredValue(value);
      if (json != nullptr)
         return json;
   }
   return json_string_t(value);
}

/**
 * Create JSON representation of custom attribute
 */
json_t *CustomAttribute::toJson(const TCHAR *name) const
{
   json_t *root = json_object();
   json_object_set_new(root, "name", json_string_t(name));
   json_object_set_new(root, "value", valueToJson());
   json_object_set_new(root, "flags", json_boolean_object(flags, s_customAttrFlagMapping));
   json_object_set_new(root, "sourceObject", json_integer(sourceObject));
   return root;
}

/**
 * Default constructor for the class
 */
NObject::NObject() : m_childList(0, 32), m_parentList(8, 8), m_customAttributes(Ownership::True), m_customAttributeLock(MutexType::FAST)
{
   m_id = 0;
   m_name[0] = 0;
}

/**
 * Destructor
 */
NObject::~NObject()
{
}

/**
 * Clear parent list. Should be called only when parent list is write locked.
 * This method will not check for custom attribute inheritance changes.
 */
void NObject::clearParentList()
{
   m_parentList.clear();
}

/**
 * Clear children list. Should be called only when children list is write locked.
 */
void NObject::clearChildList()
{
   m_childList.clear();
}

/**
 * Add reference to the new child object
 */
void NObject::addChildReference(const shared_ptr<NObject>& object)
{
   if (object->getId() == m_id)
      return;

   writeLockChildList();
   if (isDirectChildInternal(object->getId()))
   {
      unlockChildList();
      return;     // Already in the child list
   }
   m_childList.add(object);
   unlockChildList();

   // Update custom attribute inheritance
   StringObjectMap<CustomAttribute> inheritedAttributes(Ownership::True);
   lockCustomAttributes();
   auto it = m_customAttributes.begin();
   while(it.hasNext())
   {
      KeyValuePair<CustomAttribute> *pair = it.next();
      if (pair->value->isInheritable())
         inheritedAttributes.set(pair->key, new CustomAttribute(pair->value->value, pair->value->flags & CAF_JSON, pair->value->isRedefined() || pair->value->sourceObject == 0 ? m_id : pair->value->sourceObject));
   }
   unlockCustomAttributes();

   inheritedAttributes.forEach(
      [&object] (const TCHAR *name, const CustomAttribute *attr) -> EnumerationCallbackResult
      {
         object->setInheritedCustomAttribute(name, attr->value, attr->flags, attr->sourceObject, false);
         return _CONTINUE;
      });
}

/**
 * Add reference to parent object
 */
void NObject::addParentReference(const shared_ptr<NObject>& object)
{
   if (object->getId() == m_id)
      return;

   writeLockParentList();
   if (isDirectParentInternal(object->getId()))
   {
      unlockParentList();
      return;     // Already in the parents list
   }
   m_parentList.add(object);
   unlockParentList();
}

/**
 * Delete reference to child object
 */
void NObject::deleteChildReference(uint32_t objectId)
{
   writeLockChildList();
   for(int i = 0; i < m_childList.size(); i++)
      if (m_childList.get(i)->getId() == objectId)
      {
         m_childList.remove(i);
         break;
      }
   unlockChildList();
}

/**
 * Delete reference to parent object
 */
void NObject::deleteParentReference(uint32_t objectId)
{
   writeLockParentList();
   bool success = false;
   for(int i = 0; i < m_parentList.size(); i++)
      if (m_parentList.get(i)->getId() == objectId)
      {
         m_parentList.remove(i);
         success = true;
         break;
      }
   unlockParentList();

   if (success)
   {
      StringList removeList;

      lockCustomAttributes();
      auto it = m_customAttributes.begin();
      while(it.hasNext())
      {
         KeyValuePair<CustomAttribute> *pair = it.next();
         if (pair->value->isConflict() || (pair->value->isInherited() && (objectId == pair->value->sourceObject) && !isParent(objectId))) //Even if it is not direct parent it might be still indirect parent
            removeList.add(pair->key);
      }
      unlockCustomAttributes();

      for(int i = 0; i < removeList.size(); i++)
         updateOrDeleteCustomAttributeOnParentRemove(removeList.get(i), objectId);
   }
}

/**
 * Check if given object is an our child (possibly indirect, i.e child of child)
 *
 * @param id object ID to test
 */
bool NObject::isChild(uint32_t id) const
{
   bool result = isDirectChild(id);

   // If given object is not in child list, check if it is indirect child
   if (!result)
   {
      readLockChildList();
      for(int i = 0; i < m_childList.size(); i++)
         if (m_childList.get(i)->isChild(id))
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
bool NObject::isDirectChild(uint32_t id) const
{
   readLockChildList();
   bool result = isDirectChildInternal(id);
   unlockChildList();
   return result;
}

/**
 * Check if given object is an our parent (possibly indirect, i.e parent of parent)
 *
 * @param id object ID to test
 */
bool NObject::isParent(uint32_t id) const
{
   bool result = isDirectParent(id);

   // If given object is not in parent list, check if it is indirect parent
   if (!result)
   {
      readLockParentList();
      for(int i = 0; i < m_parentList.size(); i++)
         if (m_parentList.get(i)->isParent(id))
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
bool NObject::isDirectParent(uint32_t id) const
{
   readLockParentList();
   bool result = isDirectParentInternal(id);
   unlockParentList();
   return result;
}

/**
 * Get inheritable custom attribute value from parent by name
 */
CustomAttribute NObject::getCustomAttributeFromParent(const TCHAR *name, uint32_t id)
{
   CustomAttribute attr;
   readLockParentList();
   for(int i = 0; i < m_parentList.size(); i++)
   {
      if (m_parentList.get(i)->getId() == id)
      {
         attr = m_parentList.get(i)->getInheritableCustomAttribute(name);
         break;
      }
   }
   unlockParentList();
   return attr;
}

/**
 * Get inheritable custom attribute value from any parent by name. Returned attribute has null value if not found;
 * sourceObject is set to ID of the object where attribute is defined.
 */
CustomAttribute NObject::getCustomAttributeFromParent(const TCHAR *name)
{
   CustomAttribute attr;
   readLockParentList();
   for(int i = 0; i < m_parentList.size(); i++)
   {
      attr = m_parentList.get(i)->getInheritableCustomAttribute(name);
      if (!attr.value.isNull())
      {
         attr.sourceObject = m_parentList.get(i)->getInheritableCustomAttributeParent(name);
         break;
      }
   }
   unlockParentList();
   return attr;
}

/**
 * Get inheritable custom attribute value from parent by name
 */
bool NObject::checkCustomAttributeInConflict(const TCHAR *name, uint32_t newParent)
{
   uint32_t sourceId = newParent;
   bool inConflict = false;
   readLockParentList();
   for(int i = 0; i < m_parentList.size(); i++)
   {
      uint32_t tmp = m_parentList.get(i)->getInheritableCustomAttributeParent(name);
      if (tmp != 0)
      {
         if (sourceId != 0 && sourceId != tmp)
         {
            inConflict = true;
            break;
         }
         sourceId = tmp;
      }
   }
   unlockParentList();
   return inConflict;
}

/**
 * Convert object list to string
 */
static StringBuffer ObjectListToString(const SharedObjectArray<NObject>& list)
{
   StringBuffer buffer;
   for(int i = 0; i < list.size(); i++)
   {
      if (!buffer.isEmpty())
         buffer.append(_T(" "));
      buffer.append(list.get(i)->getId());
   }
   return buffer;
}

/**
 * Get children IDs in printable form
 */
StringBuffer NObject::dbgGetChildList() const
{
   readLockChildList();
   StringBuffer list = ObjectListToString(m_childList);
   unlockChildList();
   return list;
}

/**
 * Get parents IDs in printable form
 */
StringBuffer NObject::dbgGetParentList() const
{
   readLockParentList();
   StringBuffer list = ObjectListToString(m_parentList);
   unlockParentList();
   return list;
}

/**
 * Calculate custom attribute flag change based on state change and flag value
 */
static inline uint32_t CalculateFlagChange(uint32_t flags, StateChange change, uint32_t flag)
{
   if (change == StateChange::IGNORE)
      return flags;
   if (change == StateChange::CLEAR)
      return flags & ~flag;
   return flags | flag;
}

/**
 * Set custom attribute from JSON object or array
 */
void NObject::setCustomAttribute(const wchar_t *name, const json_t *value, StateChange inheritable)
{
   setCustomAttribute(name, SerializeStructuredValue(value), CAF_JSON, inheritable);
}

/**
 * Set custom attribute. Value flags can contain CAF_JSON to mark structured value.
 */
void NObject::setCustomAttribute(const wchar_t *name, const SharedString& value, uint32_t valueFlags, StateChange inheritable)
{
   lockCustomAttributes();
   CustomAttribute *curr = m_customAttributes.get(name);
   bool propagateRemove = false;

   if (curr == nullptr)
   {
      curr = new CustomAttribute(value, CalculateFlagChange(0, inheritable, CAF_INHERITABLE) | valueFlags);
      m_customAttributes.set(name, curr);
      onCustomAttributeChange(name, value);
   }
   else if (_tcscmp(curr->value, value) || ((curr->flags & CAF_JSON) != valueFlags) ||
            (curr->flags != CalculateFlagChange(curr->flags, inheritable, CAF_INHERITABLE)) || (curr->isInherited() && !curr->isRedefined()))
   {
      propagateRemove = (curr->flags != CalculateFlagChange(curr->flags, inheritable, CAF_INHERITABLE));
      curr->value = value;
      if (curr->isInherited())
         curr->flags = CAF_INHERITABLE | CAF_REDEFINED | valueFlags;
      else
         curr->flags = (CalculateFlagChange(curr->flags, inheritable, CAF_INHERITABLE) & ~CAF_JSON) | valueFlags;
      onCustomAttributeChange(name, value);
   }

   bool inherit = curr->isInheritable();
   uint32_t source = (curr->isRedefined() || !curr->isInherited()) ? m_id : curr->sourceObject;
   unlockCustomAttributes();

   if (inherit)
      propagateCustomAttributeChange(name, value, valueFlags, source);
   else if (propagateRemove)
      propagateCustomAttributeRemove(name, source);
}

/**
 * Set custom attribute inherited from parent object
 */
void NObject::setInheritedCustomAttribute(const wchar_t *name, const SharedString& value, uint32_t valueFlags, uint32_t parent, bool conflict)
{
   lockCustomAttributes();

   CustomAttribute *curr = m_customAttributes.get(name);
   bool propagateChanges = true;

   if (curr != nullptr && !curr->isRedefined())
   {
      if ((curr->sourceObject != 0) && checkCustomAttributeInConflict(name, parent))
      {
         curr->flags |= CAF_CONFLICT;
      }
      else if (curr->isConflict())
      {
         curr->flags &= ~CAF_CONFLICT;
      }
   }

   if (curr == nullptr)
   {
      curr = new CustomAttribute(value, CAF_INHERITABLE | valueFlags, parent);
      m_customAttributes.set(name, curr);
      onCustomAttributeChange(name, value);

      if (conflict) //Will be set when redefined flag removed and conflict persists
         curr->flags |= CAF_CONFLICT;
   }
   else if (_tcscmp(curr->value, value) || ((curr->flags & CAF_JSON) != valueFlags) || (curr->sourceObject == 0) || (curr->isConflict() && curr->sourceObject != parent))
   {
      propagateChanges = (curr->isInheritable() && !curr->isRedefined()) || !curr->isInherited();
      curr->flags |= CAF_INHERITABLE;
      if (curr->sourceObject == 0) //If already defined, but not by inheritance
         curr->flags |= CAF_REDEFINED;

      if (!curr->isRedefined())
      {
         curr->value = value;
         curr->flags = (curr->flags & ~CAF_JSON) | valueFlags;
      }

      curr->sourceObject = parent;
      onCustomAttributeChange(name, value);
   }

   uint32_t source = parent;
   SharedString effectiveValue = value;
   uint32_t effectiveValueFlags = valueFlags;
   if (curr->isRedefined())
   {
      source = m_id;
      effectiveValue = curr->value;
      effectiveValueFlags = curr->flags & CAF_JSON;
   }

   unlockCustomAttributes();

   if (propagateChanges)
      propagateCustomAttributeChange(name, effectiveValue, effectiveValueFlags, source);
}

/**
 * Update inherited custom attribute for child node
 */
void NObject::propagateCustomAttributeChange(const TCHAR *name, const SharedString& value, uint32_t valueFlags, uint32_t source)
{
   writeLockChildList();
   for(int i = 0; i < m_childList.size(); i++)
   {
      m_childList.get(i)->setInheritedCustomAttribute(name, value, valueFlags, source, false);
   }
   unlockChildList();
}

/**
 * Update inherited custom attribute remove for child node
 */
void NObject::propagateCustomAttributeRemove(const TCHAR *name, uint32_t parentId)
{
   writeLockChildList();
   for(int i = 0; i < m_childList.size(); i++)
   {
      m_childList.get(i)->deleteInheritedCustomAttribute(name, parentId);
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
            m_customAttributes.setPreallocated(name, curr);
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
 * Set custom attributes from NXCP message. Attributes marked as structured (CAF_JSON) are validated
 * and re-serialized in canonical form; returns RCC_INVALID_ARGUMENT if any of them is not a JSON object or array.
 */
uint32_t NObject::setCustomAttributesFromMessage(const NXCPMessage& msg)
{
   int count = msg.getFieldAsUInt32(VID_NUM_CUSTOM_ATTRIBUTES);
   StringObjectMap<CustomAttribute> newAttributes(Ownership::True);
   uint32_t fieldId = VID_CUSTOM_ATTRIBUTES_BASE;
   for(int i = 0; i < count; i++, fieldId += 3)
   {
      TCHAR name[128];
      msg.getFieldAsString(fieldId, name, 128);
      if (name[0] == '$')
         continue;

      TCHAR *value = msg.getFieldAsString(fieldId + 1);
      if (value == nullptr)
         continue;

      uint32_t flags = msg.getFieldAsUInt32(fieldId + 2) & (CAF_INHERITABLE | CAF_JSON);
      if (flags & CAF_JSON)
      {
         json_t *json = ParseStructuredValue(value);
         MemFree(value);
         if (json == nullptr)
         {
            nxlog_debug_tag(L"obj.attr", 4, L"NObject::setCustomAttributesFromMessage(%s [%u]): value of custom attribute \"%s\" marked as structured is not a valid JSON object or array", m_name, m_id, name);
            return RCC_INVALID_ARGUMENT;
         }
         newAttributes.set(name, new CustomAttribute(SerializeStructuredValue(json), flags));
         json_decref(json);
      }
      else
      {
         newAttributes.set(name, new CustomAttribute(SharedString(value, Ownership::True), flags));
      }
   }

   StringList existingAttibutes;
   StringList deletionList;
   ObjectArray<std::pair<String, uint32_t>> updateList(0, 16, Ownership::True);

   newAttributes.forEach(
      [this, &existingAttibutes] (const TCHAR *name, const CustomAttribute *attr) -> EnumerationCallbackResult
      {
         setCustomAttribute(name, attr->value, attr->flags & CAF_JSON, attr->isInheritable() ? StateChange::SET : StateChange::CLEAR);
         existingAttibutes.add(name);
         return _CONTINUE;
      });

   lockCustomAttributes();
   auto it = m_customAttributes.begin();
   while(it.hasNext())
   {
      KeyValuePair<CustomAttribute> *pair = it.next();
      if (pair->key[0] == '$')
         continue;

      if (pair->value->isRedefined() && !pair->value->isInherited())
      {
         // This inherited attribute was redefined, but parent attribute since gone - reset "redefined" flag
         pair->value->flags &= ~CAF_REDEFINED;
      }
      if ((pair->value->isInherited() == pair->value->isRedefined()) && !existingAttibutes.contains(pair->key))
      {
         if (pair->value->isRedefined())
         {
            updateList.add(new std::pair<String, uint32_t>(pair->key, pair->value->sourceObject));
         }
         else if (pair->value->isInheritable())
         {
            deletionList.add(pair->key);
         }
         onCustomAttributeChange(pair->key, nullptr);
         it.remove();
      }
   }
   unlockCustomAttributes();

   for(int i = 0; i < deletionList.size(); i++)
      propagateCustomAttributeRemove(deletionList.get(i), m_id);

   for(int i = 0; i < updateList.size(); i++)
   {
      const TCHAR *name = updateList.get(i)->first;
      uint32_t parentId = updateList.get(i)->second;
      CustomAttribute parentAttr = getCustomAttributeFromParent(name, parentId);
      if (!parentAttr.value.isNull())
      {
         setInheritedCustomAttribute(name, parentAttr.value, parentAttr.flags & CAF_JSON, parentId, checkCustomAttributeInConflict(name, parentId));
      }
      else
      {
         parentAttr = getCustomAttributeFromParent(name);
         if (!parentAttr.value.isNull())
            setInheritedCustomAttribute(name, parentAttr.value, parentAttr.flags & CAF_JSON, parentAttr.sourceObject, checkCustomAttributeInConflict(name, parentId));
         else
            propagateCustomAttributeRemove(name, 0); // Attribute no longer exist at parent, delete it from this object
      }
   }
   return RCC_SUCCESS;
}

/**
 * Delete custom attribute
 */
void NObject::deleteCustomAttribute(const TCHAR *name)
{
   lockCustomAttributes();
   CustomAttribute *ca = m_customAttributes.get(name);
   bool propagate = false;
   bool redefined = false;
   uint32_t parent;
   if ((ca != nullptr) && (!ca->isInherited() || ca->isRedefined()))
   {
      propagate = ca->isInheritable() && !ca->isRedefined();
      redefined = ca->isRedefined();
      parent = ca->sourceObject;

      if (ca->isRedefined())
      {
         ca->flags &= ~CAF_REDEFINED;
         ca->copyValue(getCustomAttributeFromParent(name, ca->sourceObject));
      }
      else
      {
         m_customAttributes.remove(name);
      }

      onCustomAttributeChange(name, nullptr);
   }
   unlockCustomAttributes();

   if (propagate)
   {
      propagateCustomAttributeRemove(name, m_id);
   }
   else if (redefined)
   {
      CustomAttribute parentAttr = getCustomAttributeFromParent(name, parent);
      if (!parentAttr.value.isNull())
      {
         setInheritedCustomAttribute(name, parentAttr.value, parentAttr.flags & CAF_JSON, parent, checkCustomAttributeInConflict(name, parent));
      }
      else
      {
         parentAttr = getCustomAttributeFromParent(name);
         if (!parentAttr.value.isNull())
            setInheritedCustomAttribute(name, parentAttr.value, parentAttr.flags & CAF_JSON, parentAttr.sourceObject, checkCustomAttributeInConflict(name, parent));
         else
            propagateCustomAttributeRemove(name, m_id); // Attribute no longer exist at parent, delete it from this object
      }
   }
}

/**
 * Update or delete inherited custom attribute after parent object removal
 */
void NObject::updateOrDeleteCustomAttributeOnParentRemove(const TCHAR *name, uint32_t parentId)
{
   CustomAttribute parentAttr = NObject::getCustomAttributeFromParent(name);

   lockCustomAttributes();
   CustomAttribute *ca = m_customAttributes.get(name);
   bool propagateDelete = false;
   bool propagateChange = false;
   if (ca != nullptr)
   {
      if (ca->isRedefined())
      {
         if (parentAttr.sourceObject != 0)
         {
            ca->sourceObject = parentAttr.sourceObject;
         }
         else
         {
            ca->sourceObject = 0;
            ca->flags &= ~CAF_REDEFINED;
         }
         onCustomAttributeChange(name, ca->value);
      }
      else if (ca->isInheritable() && !ca->isConflict())
      {
         m_customAttributes.remove(name);
         propagateDelete = true;
         onCustomAttributeChange(name, nullptr);
      }
      else if (ca->isConflict())
      {
         if (parentId == ca->sourceObject)
         {
            ca->sourceObject = parentAttr.sourceObject;
            ca->copyValue(parentAttr);
            propagateChange = true;
         }
         if (!checkCustomAttributeInConflict(name, 0))
         {
            ca->flags &= ~CAF_CONFLICT;
            if (!propagateChange && parentAttr.sourceObject != 0)
            {
               ca->sourceObject = parentAttr.sourceObject;
               ca->copyValue(parentAttr);
               propagateChange = true;
            }
         }
         onCustomAttributeChange(name, ca->value);
      }
   }
   unlockCustomAttributes();

   if (propagateDelete)
      propagateCustomAttributeRemove(name, parentId);
   if (propagateChange)
      propagateCustomAttributeChange(name, parentAttr.value, parentAttr.flags & CAF_JSON, parentAttr.sourceObject);
}

/**
 * Delete inherited custom attribute
 */
void NObject::deleteInheritedCustomAttribute(const TCHAR *name, uint32_t parentId)
{
   CustomAttribute parentAttr = getCustomAttributeFromParent(name);
   lockCustomAttributes();
   CustomAttribute *ca = m_customAttributes.get(name);
   bool propagateChange = false;
   bool propagateDelete = false;
   if (ca != nullptr && (ca->sourceObject == parentId || (parentAttr.sourceObject != parentId || parentId == 0)))
   {
      if (!ca->isRedefined() && ca->sourceObject != 0) //source check required if node is multiple times under parent container
      {
         if (ca->isConflict())
         {
            if (parentId == ca->sourceObject)
            {
               ca->sourceObject = parentAttr.sourceObject;
               ca->copyValue(parentAttr);
               propagateChange = true;
            }

            if (!checkCustomAttributeInConflict(name, 0))
            {
               ca->flags &= ~CAF_CONFLICT;
            }
         }
         else
         {
            m_customAttributes.remove(name);
            propagateDelete = true;
         }
      }
      else if (ca->isRedefined())
      {
         if (!parentAttr.value.isNull()) //Redefine may come from different node
         {
            ca->sourceObject = parentAttr.sourceObject;
         }
         else
         {
            ca->sourceObject = 0;
            ca->flags &= ~CAF_REDEFINED;
         }
      }

      onCustomAttributeChange(name, nullptr);
   }
   unlockCustomAttributes();
   if (propagateDelete)
      propagateCustomAttributeRemove(name, parentId);
   if (propagateChange)
      propagateCustomAttributeChange(name, parentAttr.value, parentAttr.flags & CAF_JSON, parentAttr.sourceObject);
}

/**
 * Get named object attribute. Allows subclasses to create virtual custom attributes - those not set directly
 * in custom attribute list but provided by class implementation. Returns true if attribute was retrieved.
 * Parameter isAllocated will be set to true if returned value should be freed by MemFree().
 * Returns true if attribute was retrieved. Default implementation always returns false.
 */
bool NObject::getObjectAttribute(const TCHAR *name, TCHAR **value, bool *isAllocated) const
{
   return false;
}

/**
 * Get custom attribute into buffer
 */
TCHAR *NObject::getCustomAttribute(const TCHAR *name, TCHAR *buffer, size_t size) const
{
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(name);
   if (attr != nullptr)
   {
      _tcslcpy(buffer, attr->value, size);
      unlockCustomAttributes();
      return buffer;
   }
   unlockCustomAttributes();

   TCHAR *value, *result;
   bool isAllocated;
   if (getObjectAttribute(name, &value, &isAllocated))
   {
      _tcslcpy(buffer, value, size);
      if (isAllocated)
         MemFree(value);
      result = buffer;
   }
   else
   {
      result = nullptr;
   }
   return result;
}

/**
 * Get custom attribute into buffer
 */
SharedString NObject::getCustomAttribute(const TCHAR *name) const
{
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(name);
   if (attr != nullptr)
   {
      unlockCustomAttributes();
      return SharedString(attr->value);
   }
   unlockCustomAttributes();

   TCHAR *value;
   bool isAllocated;
   if (getObjectAttribute(name, &value, &isAllocated))
   {
      SharedString result(value);
      if (isAllocated)
         MemFree(value);
      return result;
   }

   return SharedString();
}

/**
 * Get custom attribute into buffer
 */
uint32_t NObject::getInheritableCustomAttributeParent(const TCHAR *name) const
{
   uint32_t parent = 0;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(name);
   if (attr != nullptr && attr->isInheritable())
   {
      if (attr->sourceObject == 0 || attr->isRedefined())
         parent = m_id;
      else
         parent = attr->sourceObject;
   }
   unlockCustomAttributes();
   return parent;
}

/**
 * Get custom attribute into buffer
 */
CustomAttribute NObject::getInheritableCustomAttribute(const TCHAR *name) const
{
   CustomAttribute result;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(name);
   if (attr != nullptr && attr->isInheritable())
      result = *attr;
   unlockCustomAttributes();
   return result;
}

/**
 * Get copy of custom attribute. Returned value must be freed by caller
 */
TCHAR *NObject::getCustomAttributeCopy(const TCHAR *name) const
{
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(name);
   if (attr != nullptr)
   {
      TCHAR *result = MemCopyString(attr->value);
      unlockCustomAttributes();
      return result;
   }
   unlockCustomAttributes();

   TCHAR *value;
   bool isAllocated;
   if (getObjectAttribute(name, &value, &isAllocated))
      return isAllocated ? value : MemCopyString(value);

   return nullptr;
}

/**
 * Get custom attribute value by key as INT32
 */
int32_t NObject::getCustomAttributeAsInt32(const TCHAR *key, int32_t defaultValue) const
{
   int32_t result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(key);
   if (attr != nullptr)
      result = _tcstol(attr->value, nullptr, 0);
   unlockCustomAttributes();
   return result;
}

/**
 * Get custom attribute value by key as UINT32
 */
uint32_t NObject::getCustomAttributeAsUInt32(const TCHAR *key, uint32_t defaultValue) const
{
   uint32_t result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(key);
   if (attr != nullptr)
      result = _tcstoul(attr->value, nullptr, 0);
   unlockCustomAttributes();
   return result;
}

/**
 * Get custom attribute value by key as INT64
 */
int64_t NObject::getCustomAttributeAsInt64(const TCHAR *key, int64_t defaultValue) const
{
   int64_t result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(key);
   if (attr != nullptr)
      result = _tcstoll(attr->value, nullptr, 0);
   unlockCustomAttributes();
   return result;

}

/**
 * Get custom attribute value by key as UINT64
 */
uint64_t NObject::getCustomAttributeAsUInt64(const TCHAR *key, uint64_t defaultValue) const
{
   uint64_t result = defaultValue;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(key);
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
   const CustomAttribute *attr = m_customAttributes.get(key);
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
   const CustomAttribute *attr = m_customAttributes.get(key);
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
StringMap *NObject::getCustomAttributes(bool (*filter)(const wchar_t *, const CustomAttribute *, void *), void *context) const
{
   StringMap *attributes = new StringMap();
   lockCustomAttributes();
   StructArray<KeyValuePair<CustomAttribute>> *filtered = m_customAttributes.toArray(filter, context);
   for(int i = 0; i < filtered->size(); i++)
      attributes->set(filtered->get(i)->key, filtered->get(i)->value->value);
   delete filtered;
   unlockCustomAttributes();
   return attributes;
}

/**
 * Callback filter for matching attribute name by regular expression
 */
static bool RegExpAttrFilter(const wchar_t *name, const CustomAttribute *value, PCRE *preg)
{
   int ovector[30];
   return _pcre_exec_w(preg, nullptr, reinterpret_cast<const PCRE_WCHAR*>(name), static_cast<int>(wcslen(name)), 0, 0, ovector, 30) >= 0;
}

/**
 * Get all custom attributes matching given regular expression. Regular expression can be NULL to return all attributes.
 * Filter arguments: attribute name, attribute value, context
 */
StringMap *NObject::getCustomAttributes(const wchar_t *regexp) const
{
   if (regexp == nullptr)
      return getCustomAttributes(nullptr, nullptr);

   const char *eptr;
   int eoffset;
   PCRE *preg = _pcre_compile_w(reinterpret_cast<const PCRE_WCHAR*>(regexp), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (preg == nullptr)
      return new StringMap();

   StringMap *attributes = new StringMap();
   lockCustomAttributes();
   StructArray<KeyValuePair<CustomAttribute>> *filtered = m_customAttributes.toArray(RegExpAttrFilter, preg);
   for(int i = 0; i < filtered->size(); i++)
      attributes->set(filtered->get(i)->key, filtered->get(i)->value->value);
   delete filtered;
   unlockCustomAttributes();
   _pcre_free_t(preg);
   return attributes;
}

/**
 * Get all custom attributes matching given filter as JSON object. Filter can be NULL to return all attributes.
 * Filter arguments: attribute name, attribute value, context
 */
json_t *NObject::getCustomAttributesAsJson(bool (*filter)(const TCHAR *, const CustomAttribute *, void *), void *context) const
{
   json_t *attributes = json_object();
   lockCustomAttributes();
   StructArray<KeyValuePair<CustomAttribute>> *filtered = m_customAttributes.toArray(filter, context);
   for(int i = 0; i < filtered->size(); i++)
   {
      char keyBuffer[256];
      wchar_to_utf8(filtered->get(i)->key, -1, keyBuffer, 256);
      json_object_set_new(attributes, keyBuffer, filtered->get(i)->value->valueToJson());
   }
   delete filtered;
   unlockCustomAttributes();
   return attributes;
}

/**
 * Create NXSL value from structured custom attribute. Returns nullptr if attribute is not structured
 * or its text cannot be parsed as JSON object or array.
 */
static NXSL_Value *StructuredValueToNXSL(NXSL_VM *vm, const CustomAttribute *attr)
{
   if (!attr->isJson())
      return nullptr;

   json_t *json = ParseStructuredValue(attr->value);
   if (json == nullptr)
      return nullptr;

   NXSL_Value *value = vm->createValueFromJson(json);
   json_decref(json);
   return value;
}

/**
 * Get custom attribute as NXSL value
 */
NXSL_Value *NObject::getCustomAttributeForNXSL(NXSL_VM *vm, const TCHAR *name) const
{
   NXSL_Value *value = nullptr;
   lockCustomAttributes();
   const CustomAttribute *attr = m_customAttributes.get(name);
   if (attr != nullptr)
   {
      value = StructuredValueToNXSL(vm, attr);
      if (value == nullptr)
      {
         // Handle "true" and "false" strings as boolean value
         if (!_tcscmp(attr->value.cstr(), _T("true")))
            value = vm->createValue(true);
         else if (!_tcscmp(attr->value.cstr(), _T("false")))
            value = vm->createValue(false);
         else
            value = vm->createValue(attr->value);
      }
   }
   else
   {
      TCHAR *v;
      bool isAllocated;
      if (getObjectAttribute(name, &v, &isAllocated))
      {
         value = vm->createValue(v);
         if (isAllocated)
            MemFree(v);
      }
   }
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
   StructArray<KeyValuePair<CustomAttribute>> *attributes = m_customAttributes.toArray();
   for(int i = 0; i < attributes->size(); i++)
   {
      KeyValuePair<CustomAttribute> *p = attributes->get(i);
      NXSL_Value *value = StructuredValueToNXSL(vm, p->value);
      map->set(p->key, (value != nullptr) ? value : vm->createValue(p->value->value));
   }
   unlockCustomAttributes();
   delete attributes;
   return vm->createValue(map);
}

/**
 * Hook method called after change in custom attributes
 */
void NObject::onCustomAttributeChange(const TCHAR *name, const TCHAR *value)
{
}

/**
 * Will remove custom attributes from current node if it's inherited and duplicate key+value from parent node
 */
void NObject::pruneCustomAttributes()
{
   StringList deletionList;
   lockCustomAttributes();
   StructArray<KeyValuePair<CustomAttribute>> *attributes = m_customAttributes.toArray();
   for(int i = 0; i < attributes->size(); i++)
   {
      KeyValuePair<CustomAttribute> *p = attributes->get(i);
      if (p->value->isRedefined())
      {
         CustomAttribute parentAttr = getCustomAttributeFromParent(p->key, p->value->sourceObject);
         if (parentAttr.value.isNull())
         {
            CustomAttribute *ca = m_customAttributes.get(p->key);
            ca->flags &= ~CAF_REDEFINED;
            onCustomAttributeChange(p->key, ca->value);
         }
         else if (!_tcscmp(p->value->value, parentAttr.value) && (p->value->isJson() == parentAttr.isJson()))
         {
            deletionList.add(p->key);
         }
      }
   }
   delete attributes;
   unlockCustomAttributes();

   for (int i = 0; i < deletionList.size(); i++)
   {
      deleteCustomAttribute(deletionList.get(i));
   }
}
