/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
** File: snmptrap.cpp
**
**/

#include "nxcore.h"
#include <nxcore_discovery.h>

#define DEBUG_TAG _T("snmp.trap")

#define BY_OBJECT_ID 0
#define BY_POSITION 1

/**
 * Static data
 */
static Mutex s_trapMappingLock(MutexType::FAST);
static SharedObjectArray<SNMPTrapMapping> s_trapMappings(16, 16);

/**
 * Collects information about all SNMPTraps that are using specified event
 */
void GetSNMPTrapsEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences)
{
   LockGuard lockGuard(s_trapMappingLock);
   for (int i = 0; i < s_trapMappings.size(); i++)
   {
      SNMPTrapMapping *tm = s_trapMappings.get(i);
      if (tm->getEventCode() == eventCode)
      {
         eventReferences->add(new EventReference(EventReferenceType::SNMP_TRAP, tm->getId(), tm->getGuid(), tm->getOid().toString(), tm->getDescription()));
      }
   }
}

/**
 * Create new SNMP trap mapping object
 */
SNMPTrapMapping::SNMPTrapMapping() : m_objectId(), m_mappings(8, 8, Ownership::True)
{
   m_guid = uuid::generate();
   m_id = CreateUniqueId(IDG_SNMP_TRAP);
   m_eventCode = EVENT_SNMP_UNMATCHED_TRAP;
   m_eventTag = nullptr;
   m_description = nullptr;
   m_scriptSource = nullptr;
   m_script = nullptr;
}

/**
 * Create SNMP trap mapping object from database
 */
SNMPTrapMapping::SNMPTrapMapping(DB_RESULT trapResult, DB_HANDLE hdb, DB_STATEMENT stmt, int row) : m_mappings(8, 8, Ownership::True)
{
   m_id = DBGetFieldULong(trapResult, row, 0);
   TCHAR buffer[MAX_OID_LENGTH];
   m_objectId = SNMP_ObjectId::parse(DBGetField(trapResult, row, 1, buffer, MAX_OID_LENGTH));
   m_eventCode = DBGetFieldULong(trapResult, row, 2);
   m_description = DBGetField(trapResult, row, 3, nullptr, 0);
   m_eventTag = DBGetField(trapResult, row, 4, nullptr, 0);
   m_guid = DBGetFieldGUID(trapResult, row, 5);
   m_scriptSource = DBGetField(trapResult, row, 6, nullptr, 0);
   m_script = nullptr;

   DB_RESULT mapResult;
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_INTEGER, m_id);
      mapResult = DBSelectPrepared(stmt);
   }
   else
   {
      mapResult = ExecuteSelectOnObject(hdb, m_id, _T("SELECT snmp_oid,description,flags FROM snmp_trap_pmap WHERE trap_id={id} ORDER BY parameter"));
   }
   if (mapResult != nullptr)
   {
      int mapCount = DBGetNumRows(mapResult);
      for(int i = 0; i < mapCount; i++)
      {
         SNMPTrapParameterMapping *param = new SNMPTrapParameterMapping(mapResult, i);
         if (!param->isPositional() && !param->getOid()->isValid())
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Invalid trap parameter OID %s for trap mapping %u in trap mapping table"), param->getOid()->toString().cstr(), m_id);
         m_mappings.add(param);
      }
      DBFreeResult(mapResult);
   }

   compileScript();
}

/**
 * Create SNMP trap mapping from JSON
 */
SNMPTrapMapping::SNMPTrapMapping(json_t *json, const uuid& guid, uint32_t id, uint32_t eventCode, bool nxslV5) : m_mappings(8, 8, Ownership::True)
{
   if (id == 0)
      m_id = CreateUniqueId(IDG_SNMP_TRAP);
   else
      m_id = id;

   m_guid = guid;
   
   TCHAR *oidStr = json_object_get_string_t(json, "oid", _T(""));
   m_objectId = SNMP_ObjectId::parse(oidStr);
   MemFree(oidStr);

   m_eventCode = eventCode;
   
   m_description = json_object_get_string_t(json, "description", _T(""));   
   m_eventTag = json_object_get_string_t(json, "eventTag", nullptr);   
   m_scriptSource = json_object_get_string_t(json, "transformationScript", _T(""));
   
   m_script = nullptr;

   json_t *parametersArray = json_object_get(json, "parameters");
   if (json_is_array(parametersArray))
   {
      size_t index;
      json_t *param;
      json_array_foreach(parametersArray, index, param)
      {
         if (json_is_object(param))
         {
            SNMPTrapParameterMapping *paramMapping = new SNMPTrapParameterMapping(param);
            if (!paramMapping->isPositional() && !paramMapping->getOid()->isValid())
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Invalid trap parameter OID for trap %u in trap mapping table"), m_id);
            m_mappings.add(paramMapping);
         }
      }
   }

   compileScript();
}

/**
 * Create SNMP trap mapping object from config entry
 */
SNMPTrapMapping::SNMPTrapMapping(const ConfigEntry& entry, const uuid& guid, uint32_t id, uint32_t eventCode, bool nxslV5) : m_mappings(8, 8, Ownership::True)
{
   if (id == 0)
      m_id = CreateUniqueId(IDG_SNMP_TRAP);
   else
      m_id = id;

   m_guid = guid;
   m_objectId = SNMP_ObjectId::parse(entry.getSubEntryValue(_T("oid"), 0, _T("")));
   m_eventCode = eventCode;
   m_description = MemCopyString(entry.getSubEntryValue(_T("description")));
   m_eventTag = MemCopyString(entry.getSubEntryValue(_T("eventTag"), 0, entry.getSubEntryValue(_T("userTag"))));
   if (nxslV5)
      m_scriptSource = MemCopyString(entry.getSubEntryValue(_T("transformationScript")));
   else
   {
      StringBuffer output = NXSLConvertToV5(NXSLConvertToV5(entry.getSubEntryValue(_T("transformationScript"), 0, _T(""))));
      m_scriptSource = MemCopyString(output);
   }
   m_script = nullptr;

   const ConfigEntry *parametersRoot = entry.findEntry(_T("parameters"));
   if (parametersRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> parameters = parametersRoot->getOrderedSubEntries(_T("parameter#*"));
      if (parameters->size() > 0)
      {
         for(int i = 0; i < parameters->size(); i++)
         {
            SNMPTrapParameterMapping *param = new SNMPTrapParameterMapping(parameters->get(i));
            if (!param->isPositional() && !param->getOid()->isValid())
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Invalid trap parameter OID %s for trap %u in trap mapping table"), param->getOid()->toString().cstr(), m_id);
            m_mappings.add(param);
         }
      }
   }

   compileScript();
}

/**
 * Create SNMP trap mapping object from NXCPMessage
 */
SNMPTrapMapping::SNMPTrapMapping(const NXCPMessage& msg) : m_mappings(8, 8, Ownership::True)
{
   uint32_t buffer[MAX_OID_LENGTH];

   m_id = msg.getFieldAsUInt32(VID_TRAP_ID);
   msg.getFieldAsInt32Array(VID_TRAP_OID, msg.getFieldAsUInt32(VID_TRAP_OID_LEN), buffer);
   m_objectId = SNMP_ObjectId(buffer, msg.getFieldAsUInt32(VID_TRAP_OID_LEN));
   m_eventCode = msg.getFieldAsUInt32(VID_EVENT_CODE);
   m_description = msg.getFieldAsString(VID_DESCRIPTION);
   m_eventTag = msg.getFieldAsString(VID_USER_TAG);
   m_scriptSource = msg.getFieldAsString(VID_TRANSFORMATION_SCRIPT);
   m_script = nullptr;

   // Read new mappings from message
   int count = msg.getFieldAsInt32(VID_TRAP_NUM_MAPS);
   uint32_t base = VID_TRAP_PBASE;
   for(int i = 0; i < count; i++, base += 10)
   {
      m_mappings.add(new SNMPTrapParameterMapping(msg, base));
   }

   compileScript();
}

/**
 * Destructor for SNMP trap mapping object
 */
SNMPTrapMapping::~SNMPTrapMapping()
{
   MemFree(m_description);
   MemFree(m_eventTag);
   MemFree(m_scriptSource);
   delete m_script;
}

/**
 * Compile transformation script
 */
void SNMPTrapMapping::compileScript()
{
   delete m_script;
   if ((m_scriptSource != nullptr) && (*m_scriptSource != 0))
   {
      m_script = CompileServerScript(m_scriptSource, SCRIPT_CONTEXT_SNMP_TRAP, nullptr, 0, _T("SNMPTrap::%u"), m_id);
      if (m_script == nullptr)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Failed to compile SNMP trap transformation script for trap mapping [%u]"), m_id);
      }
   }
   else
   {
      m_script = nullptr;
   }
}

/**
 * Fill NXCP message with trap mapping data
 */
void SNMPTrapMapping::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_TRAP_ID, m_id);
   msg->setField(VID_TRAP_OID_LEN, (UINT32)m_objectId.length());
   msg->setFieldFromInt32Array(VID_TRAP_OID, m_objectId.length(), m_objectId.value());
   msg->setField(VID_EVENT_CODE, m_eventCode);
   msg->setField(VID_DESCRIPTION, CHECK_NULL_EX(m_description));
   msg->setField(VID_USER_TAG, CHECK_NULL_EX(m_eventTag));
   msg->setField(VID_TRANSFORMATION_SCRIPT, CHECK_NULL_EX(m_scriptSource));

   msg->setField(VID_TRAP_NUM_MAPS, (UINT32)m_mappings.size());
   UINT32 base = VID_TRAP_PBASE;
   for(int i = 0; i < m_mappings.size(); i++, base += 10)
   {
      m_mappings.get(i)->fillMessage(msg, base);
   }
}

/**
 * Fill NXCP message with trap mapping for list
 */
void SNMPTrapMapping::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base, m_id);
   msg->setField(base + 1, CHECK_NULL_EX(m_description));
   msg->setFieldFromInt32Array(base + 2, m_objectId.length(), m_objectId.value());
   msg->setField(base + 3, m_eventCode);
}

/**
 * Save parameter mapping to database
 */
bool SNMPTrapMapping::saveParameterMapping(DB_HANDLE hdb)
{
   if (!ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=?")))
      return false;

   if (m_mappings.isEmpty())
      return true;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description,flags) VALUES (?,?,?,?,?)"), true);
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

   bool success = true;
   TCHAR oid[1024];
   for(int i = 0; (i < m_mappings.size()) && success; i++)
   {
      const SNMPTrapParameterMapping *pm = m_mappings.get(i);
      if (!pm->isPositional())
         pm->getOid()->toString(oid, 1024);
      else
         _sntprintf(oid, 1024, _T("POS:%d"), pm->getPosition());

      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pm->getDescription(), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pm->getFlags());

      success = DBExecute(hStmt);
   }
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Notify clients about trap mapping change
 */
static void NotifyOnTrapMappingChangeCB(ClientSession *session, NXCPMessage *msg)
{
   if (session->isAuthenticated())
      session->postMessage(msg);
}

/**
 * Notify clients of trap cfg change
 */
void SNMPTrapMapping::notifyOnTrapCfgChange(uint32_t code)
{
   NXCPMessage msg(CMD_TRAP_CFG_UPDATE, 0);
   msg.setField(VID_NOTIFICATION_CODE, code);
   fillMessage(&msg);
   EnumerateClientSessions(NotifyOnTrapMappingChangeCB, &msg);
}

/**
 * Create configuration export record
 */
json_t *SNMPTrapMapping::createExportRecord() const
{
   json_t *trap = json_object();
   TCHAR szBuffer[1024];
   
   json_object_set_new(trap, "id", json_integer(m_id));
   json_object_set_new(trap, "guid", json_string_t(m_guid.toString()));
   json_object_set_new(trap, "oid", json_string_t(m_objectId.toString()));
   json_object_set_new(trap, "description", json_string_t(m_description));
   json_object_set_new(trap, "eventTag", json_string_t(m_eventTag));
   
   EventNameFromCode(m_eventCode, szBuffer);
   json_object_set_new(trap, "event", json_string_t(szBuffer));
   
   json_object_set_new(trap, "transformationScript", json_string_t(m_scriptSource));
   
   if (m_mappings.size() > 0)
   {
      json_t *parameters = json_array();
      for(int i = 0; i < m_mappings.size(); i++)
      {
         const SNMPTrapParameterMapping *pm = m_mappings.get(i);
         json_t *parameter = json_object();
         
         json_object_set_new(parameter, "id", json_integer(i + 1));
         json_object_set_new(parameter, "flags", json_integer(pm->getFlags()));
         json_object_set_new(parameter, "description", json_string_t(pm->getDescription()));
         
         if (!pm->isPositional())
         {
            json_object_set_new(parameter, "oid", json_string_t(pm->getOid()->toString(szBuffer, 1024)));
         }
         else
         {
            json_object_set_new(parameter, "position", json_integer(pm->getPosition()));
         }
         
         json_array_append_new(parameters, parameter);
      }
      json_object_set_new(trap, "parameters", parameters);
   }
   
   return trap;
}

/**
 * Serialize object to JSON (for audit purposes)
 */
json_t *SNMPTrapMapping::toJson() const
{
   json_t *trap = json_object();
   TCHAR szBuffer[1024];
   
   json_object_set_new(trap, "id", json_integer(m_id));
   json_object_set_new(trap, "guid", json_string_t(m_guid.toString()));
   json_object_set_new(trap, "oid", json_string_t(m_objectId.toString()));
   json_object_set_new(trap, "description", json_string_t(m_description));
   json_object_set_new(trap, "eventTag", json_string_t(m_eventTag));
   json_object_set_new(trap, "event", json_integer(m_eventCode));   
   json_object_set_new(trap, "transformationScript", json_string_t(m_scriptSource));
   
   if (m_mappings.size() > 0)
   {
      json_t *parameters = json_array();
      for(int i = 0; i < m_mappings.size(); i++)
      {
         const SNMPTrapParameterMapping *pm = m_mappings.get(i);
         json_t *parameter = json_object();
         
         json_object_set_new(parameter, "id", json_integer(i + 1));
         json_object_set_new(parameter, "flags", json_integer(pm->getFlags()));
         json_object_set_new(parameter, "description", json_string_t(pm->getDescription()));
         
         if (!pm->isPositional())
         {
            json_object_set_new(parameter, "oid", json_string_t(pm->getOid()->toString(szBuffer, 1024)));
         }
         else
         {
            json_object_set_new(parameter, "position", json_integer(pm->getPosition()));
         }
         
         json_array_append_new(parameters, parameter);
      }
      json_object_set_new(trap, "parameters", parameters);
   }
   
   return trap;
}

/**
 * SNMP trap parameter map default constructor
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping()
{
   m_objectId = new SNMP_ObjectId();
   m_position = 0;
   m_flags = 0;
   m_description = nullptr;
}

/**
 * Create SNMP trap parameter map object from database
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(DB_RESULT mapResult, int row)
{
   TCHAR oid[MAX_DB_STRING];
   DBGetField(mapResult, row, 0, oid, MAX_DB_STRING);

   if (!_tcsncmp(oid, _T("POS:"), 4))
   {
      m_objectId = nullptr;
      m_position = _tcstoul(&oid[4], nullptr, 10);
   }
   else
   {
      m_objectId = new SNMP_ObjectId(SNMP_ObjectId::parse(oid));
      m_position = 0;
   }

   m_description = DBGetField(mapResult, row, 1, nullptr, 0);
   m_flags = DBGetFieldULong(mapResult, row, 2);
}

/**
 * Create SNMP trap parameter mapping from JSON
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(json_t *json)
{
   json_t *positionObj = json_object_get(json, "position");
   if (json_is_integer(positionObj) && (json_integer_value(positionObj) > 0))
   {
      m_objectId = nullptr;
      m_position = (uint32_t)json_integer_value(positionObj);
   }
   else
   {
      TCHAR *oidStr = json_object_get_string_t(json, "oid", _T(""));
      m_objectId = new SNMP_ObjectId(SNMP_ObjectId::parse(oidStr));
      m_position = 0;
      MemFree(oidStr);
   }

   m_description = json_object_get_string_t(json, "description", _T(""));

   json_t *flagsObj = json_object_get(json, "flags");
   m_flags = json_is_integer(flagsObj) ? (uint32_t)json_integer_value(flagsObj) : 0;
}

/**
 * Create SNMP trap parameter map object from ConfigEntry
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(ConfigEntry *entry)
{
   int position = entry->getSubEntryValueAsInt(_T("position"), 0, -1);
   if (position > 0)
   {
      m_objectId = nullptr;
      m_position = position; // Positional parameter
   }
   else
   {
      m_objectId = new SNMP_ObjectId(SNMP_ObjectId::parse(entry->getSubEntryValue(_T("oid"), 0, _T("")))); // OID parameter
      m_position = 0;
   }

   m_description = MemCopyString(entry->getSubEntryValue(_T("description")));
   m_flags = entry->getSubEntryValueAsUInt(_T("flags"), 0, 0);
}

/**
 * Create SNMP trap parameter map object from NXCPMessage
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(const NXCPMessage& msg, uint32_t base)
{
   m_flags = msg.getFieldAsUInt32(base);
   m_description = msg.getFieldAsString(base + 1);
   if (msg.getFieldAsUInt32(base + 2) == BY_POSITION)
   {
      m_objectId = nullptr;
      m_position = msg.getFieldAsUInt32(base + 3);
   }
   else
   {
      uint32_t buffer[MAX_OID_LENGTH];
      msg.getFieldAsInt32Array(base + 3, msg.getFieldAsUInt32(base + 4), buffer);
      m_objectId = new SNMP_ObjectId(buffer, msg.getFieldAsUInt32(base + 4));
   }
}

/**
 * Destructor for SNMP trap parameter map object
 */
SNMPTrapParameterMapping::~SNMPTrapParameterMapping()
{
   delete m_objectId;
   MemFree(m_description);
}

/**
 * Fill NXCP message with trap parameter map configuration data
 */
void SNMPTrapParameterMapping::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base, isPositional());
   if (isPositional())
      msg->setField(base + 1, m_position);
   else
      msg->setFieldFromInt32Array(base + 1, m_objectId->length(), m_objectId->value());
   msg->setField(base + 2, CHECK_NULL_EX(m_description));
   msg->setField(base + 3, m_flags);

}

/**
 * Load trap mappings from database
 */
void LoadTrapMappings()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load traps
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT trap_id,snmp_oid,event_code,description,user_tag,guid,transformation_script FROM snmp_trap_cfg"));
   if (hResult != nullptr)
   {
      DB_STATEMENT hStmt = (g_dbSyntax == DB_SYNTAX_ORACLE) ?
               DBPrepare(hdb, _T("SELECT snmp_oid,description,flags FROM snmp_trap_pmap WHERE trap_id=? ORDER BY parameter"), true)
               : nullptr;
      if ((g_dbSyntax != DB_SYNTAX_ORACLE) || (hStmt != nullptr))
      {
         int numRows = DBGetNumRows(hResult);
         for(int i = 0; i < numRows; i++)
         {
            auto tm = new SNMPTrapMapping(hResult, hdb, hStmt, i);
            if (!tm->getOid().isValid())
            {
               TCHAR buffer[MAX_DB_STRING];
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Invalid trap enterprise ID %s in trap mapping table"), DBGetField(hResult, i, 1, buffer, MAX_DB_STRING));
            }
            s_trapMappings.add(tm);
         }
         if (hStmt != nullptr)
            DBFreeStatement(hStmt);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Send all trap mapping records to client
 */
void SendTrapMappingsToClient(ClientSession *session, uint32_t requestId)
{
   NXCPMessage msg(CMD_TRAP_CFG_RECORD, requestId);

   s_trapMappingLock.lock();
   for(int i = 0; i < s_trapMappings.size(); i++)
   {
      s_trapMappings.get(i)->fillMessage(&msg);
      session->sendMessage(msg);
      msg.deleteAllFields();
   }
   s_trapMappingLock.unlock();

   msg.setField(VID_TRAP_ID, static_cast<uint32_t>(0));
   session->sendMessage(msg);
}

/**
 * Prepare single message with all trap mapping records
 */
void CreateTrapMappingMessage(NXCPMessage *msg)
{
   LockGuard lockGuard(s_trapMappingLock);
	msg->setField(VID_NUM_TRAPS, s_trapMappings.size());
   for(int i = 0, id = VID_TRAP_INFO_BASE; i < s_trapMappings.size(); i++, id += 10)
      s_trapMappings.get(i)->fillMessage(msg, id);
}

/**
 * Notify clients on trap mapping delete
 */
static void NotifyOnTrapMappingDelete(uint32_t id)
{
	NXCPMessage msg(CMD_TRAP_CFG_UPDATE, 0);
	msg.setField(VID_NOTIFICATION_CODE, static_cast<uint32_t>(NX_NOTIFY_TRAPCFG_DELETED));
	msg.setField(VID_TRAP_ID, id);
	EnumerateClientSessions(NotifyOnTrapMappingChangeCB, &msg);
}

/**
 * Delete trap configuration record
 */
uint32_t DeleteTrapMapping(uint32_t id)
{
   uint32_t rcc = RCC_INVALID_TRAP_ID;

   LockGuard lockGuard(s_trapMappingLock);

   for(int i = 0; i < s_trapMappings.size(); i++)
   {
      if (s_trapMappings.get(i)->getId() == id)
      {
         // Remove trap entry from database
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmtCfg = DBPrepare(hdb, _T("DELETE FROM snmp_trap_cfg WHERE trap_id=?"));
         DB_STATEMENT hStmtMap = DBPrepare(hdb, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=?"));

         if (hStmtCfg != nullptr && hStmtMap != nullptr)
         {
            DBBind(hStmtCfg, 1, DB_SQLTYPE_INTEGER, id);
            DBBind(hStmtMap, 1, DB_SQLTYPE_INTEGER, id);

            if (DBBegin(hdb))
            {
               if (DBExecute(hStmtCfg) && DBExecute(hStmtMap))
               {
                  s_trapMappings.remove(i);
                  NotifyOnTrapMappingDelete(id);
                  rcc = RCC_SUCCESS;
                  DBCommit(hdb);
               }
               else
               {
                  DBRollback(hdb);
               }
               DBFreeStatement(hStmtCfg);
               DBFreeStatement(hStmtMap);
            }
         }
         DBConnectionPoolReleaseConnection(hdb);
         break;
      }
   }

   return rcc;
}

/**
 * Create new trap configuration record
 */
uint32_t CreateNewTrapMapping(uint32_t *trapId)
{
   uint32_t rcc = RCC_DB_FAILURE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_trap_cfg (guid,trap_id,snmp_oid,event_code,description,user_tag) VALUES (?,?,'',?,'','')"));
   if (hStmt != nullptr)
   {
      auto tm = make_shared<SNMPTrapMapping>();
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, tm->getGuid());
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, tm->getId());
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, tm->getEventCode());

      if (DBExecute(hStmt))
      {
         AddTrapMappingToList(tm);
         tm->notifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_CREATED);
         *trapId = tm->getId();
         rcc = RCC_SUCCESS;
      }

      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Update trap configuration record from message
 */
uint32_t UpdateTrapMappingFromMsg(const NXCPMessage& msg)
{
   uint32_t rcc = RCC_INVALID_TRAP_ID;
   TCHAR oid[1024];

   uint32_t id = msg.getFieldAsUInt32(VID_TRAP_ID);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   for(int i = 0; i < s_trapMappings.size(); i++)
   {
      if (s_trapMappings.get(i)->getId() == id)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE snmp_trap_cfg SET snmp_oid=?,event_code=?,description=?,user_tag=?,transformation_script=? WHERE trap_id=?"));
         if (hStmt != nullptr)
         {
            auto tm = make_shared<SNMPTrapMapping>(msg);
            tm->getOid().toString(oid, 1024);
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, tm->getEventCode());
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, tm->getDescription(), DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, tm->getEventTag(), DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_TEXT, tm->getScriptSource(), DB_BIND_STATIC);
            DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, tm->getId());

            if (DBBegin(hdb))
            {
               if (DBExecute(hStmt) && tm->saveParameterMapping(hdb))
               {
                  AddTrapMappingToList(tm);
                  tm->notifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_MODIFIED);
                  rcc = RCC_SUCCESS;
                  DBCommit(hdb);
               }
               else
               {
                  DBRollback(hdb);
                  rcc = RCC_DB_FAILURE;
               }
            }
            else
            {
               rcc = RCC_DB_FAILURE;
            }
            DBFreeStatement(hStmt);
         }

			DBConnectionPoolReleaseConnection(hdb);
         break;
      }
   }

   return rcc;
}

/**
 * Create trap record as JSON object
 */
json_t *CreateTrapMappingExportRecord(uint32_t id)
{
   json_t *trap = nullptr;
   TCHAR szBuffer[1024];

   LockGuard lockGuard(s_trapMappingLock);

   for(int i = 0; i < s_trapMappings.size(); i++)
   {
      SNMPTrapMapping *tm = s_trapMappings.get(i);
      if (tm->getId() == id)
      {
         trap = json_object();
         json_object_set_new(trap, "id", json_integer(id));
         json_object_set_new(trap, "guid", json_string_t(tm->getGuid().toString()));
         json_object_set_new(trap, "oid", json_string_t(tm->getOid().toString()));
         json_object_set_new(trap, "description", json_string_t(tm->getDescription()));
         json_object_set_new(trap, "eventTag", json_string_t(tm->getEventTag()));
         
         EventNameFromCode(tm->getEventCode(), szBuffer);
         json_object_set_new(trap, "event", json_string_t(szBuffer));
         
         json_object_set_new(trap, "transformationScript", json_string_t(tm->getScriptSource()));
         
         if (tm->getParameterMappingCount() > 0)
         {
            json_t *parameters = json_array();
            for(int j = 0; j < tm->getParameterMappingCount(); j++)
            {
               const SNMPTrapParameterMapping *pm = tm->getParameterMapping(j);
               json_t *parameter = json_object();
               
               json_object_set_new(parameter, "flags", json_integer(pm->getFlags()));
               json_object_set_new(parameter, "description", json_string_t(pm->getDescription()));
               
               if (!pm->isPositional())
               {
                  json_object_set_new(parameter, "oid", json_string_t(pm->getOid()->toString(szBuffer, 1024)));
               }
               else
               {
                  json_object_set_new(parameter, "position", json_integer(pm->getPosition()));
               }
               
               json_array_append_new(parameters, parameter);
            }
            json_object_set_new(trap, "parameters", parameters);
         }
         break;
      }
   }
   
   return trap;
}

/**
 * Find if trap with guid already exists
 */
uint32_t ResolveTrapMappingGuid(const uuid& guid)
{
   uint32_t id = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT trap_id FROM snmp_trap_cfg WHERE guid=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            id = DBGetFieldULong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return id;
}

/**
 * Add SNMP trap configuration to local list
 */
void AddTrapMappingToList(const shared_ptr<SNMPTrapMapping>& tm)
{
   LockGuard lockGuard(s_trapMappingLock);

   for(int i = 0; i < s_trapMappings.size(); i++)
   {
      if (s_trapMappings.get(i)->getId() == tm->getId())
      {
         s_trapMappings.replace(i, tm);
         return;
      }
   }

   s_trapMappings.add(tm);
}

/**
 * Find trap mapping that is best match for given trap OID
 */
shared_ptr<SNMPTrapMapping> FindBestMatchTrapMapping(const SNMP_ObjectId& oid)
{
   LockGuard lockGuard(s_trapMappingLock);

   // Try to find closest match
   size_t matchLen = 0;
   int matchIndex = -1;
   for(int i = 0; i < s_trapMappings.size(); i++)
   {
      const SNMPTrapMapping *tm = s_trapMappings.get(i);
      if (tm->getOid().length() > 0)
      {
         int result = oid.compare(tm->getOid());
         if (result == OID_EQUAL)
         {
            return s_trapMappings.getShared(i); // Found exact match
         }
         if ((result == OID_LONGER) && (tm->getOid().length() > matchLen))
         {
            matchLen = tm->getOid().length();
            matchIndex = i;
         }
      }
   }

   return (matchLen > 0) ? s_trapMappings.getShared(matchIndex) : shared_ptr<SNMPTrapMapping>();
}
