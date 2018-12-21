/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: agent_policy.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor for user-initiated object creation
 */
GenericAgentPolicy::GenericAgentPolicy(uuid guid, UINT32 ownerId)
{
   m_name[0] = 0;
   m_policyType[0] = 0;
   m_guid = guid;
   m_ownerId = ownerId;
   m_fileContent = NULL;
   m_version = 1;
}


GenericAgentPolicy::GenericAgentPolicy(GenericAgentPolicy *policy)
{
   nx_strncpy(m_name, policy->m_name, MAX_OBJECT_NAME);
   nx_strncpy(m_policyType, policy->m_policyType, MAX_OBJECT_NAME);
   m_guid = policy->m_guid;
   m_ownerId = policy->m_ownerId;
   m_fileContent = MemCopyString(policy->m_fileContent);
   m_version = policy->m_version;
}

/**
 * Constructor for user-initiated object creation
 */
GenericAgentPolicy::GenericAgentPolicy(const TCHAR *name, const TCHAR *type, UINT32 ownerId)
{
	nx_strncpy(m_name, name, MAX_OBJECT_NAME);
   nx_strncpy(m_policyType, type, MAX_OBJECT_NAME);
   m_guid = uuid::generate();
   m_ownerId = ownerId;
   m_fileContent = NULL;
   m_version = 1;
}

/**
 * Destructor
 */
GenericAgentPolicy::~GenericAgentPolicy()
{
   MemFree(m_fileContent);
}

/**
 * Save to database
 */
bool GenericAgentPolicy::saveToDatabase(DB_HANDLE hdb)
{
   bool success = false;
   DB_STATEMENT hStmt;
   if (!IsDatabaseRecordExist(hdb, _T("ap_common"), _T("guid"), m_guid)) //Policy can be only created. Policy type can't be changed.
      hStmt = DBPrepare(hdb, _T("INSERT INTO ap_common (policy_name,owner_id,policy_type,file_content,version,guid) VALUES (?,?,?,?,?,?)"));
   else
      hStmt = DBPrepare(hdb, _T("UPDATE ap_common SET policy_name=?,owner_id=?,policy_type=?,file_content=?,version=? WHERE guid=?"));

   if(hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_ownerId);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_policyType, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_fileContent), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_version);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_guid);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   else
      success = false;

   return success;
}

/**
 * Delete from database
 */
bool GenericAgentPolicy::deleteFromDatabase(DB_HANDLE hdb)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM ap_common WHERE guid='%s'"), (const TCHAR *)m_guid.toString());
   return DBQuery(hdb, query);
}

/**
 * Load from database
 */
bool GenericAgentPolicy::loadFromDatabase(DB_HANDLE hdb)
{
   TCHAR query[256];
   bool success = false;
   // Load related nodes list
   _sntprintf(query, 256, _T("SELECT policy_name,owner_id,policy_type,file_content,version FROM ap_common WHERE guid='%s'"), (const TCHAR *)m_guid.toString());
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL && DBGetNumRows(hResult) > 0)
   {
      DBGetField(hResult, 0, 0, m_name, MAX_DB_STRING);
      m_ownerId = DBGetFieldLong(hResult, 0, 1);
      DBGetField(hResult, 0, 2, m_policyType, 32);
      m_fileContent = DBGetField(hResult, 0, 3, NULL, 0);
      m_version = DBGetFieldLong(hResult, 0, 4);
      DBFreeResult(hResult);
      success = true;
   }

	return success;
}

/**
 * Create NXCP message with policy data
 */
void GenericAgentPolicy::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
   msg->setField(baseId, m_guid);
   msg->setField(baseId+1, m_policyType);
   msg->setField(baseId+2, m_name);
   msg->setField(baseId+3, CHECK_NULL_EX(m_fileContent));
}

/**
 * Create NXCP message with policy data for notifications
 */
void GenericAgentPolicy::fillUpdateMessage(NXCPMessage *msg)
{
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_NAME, m_name);
   msg->setField(VID_POLICY_TYPE, m_policyType);
   msg->setField(VID_CONFIG_FILE_DATA, CHECK_NULL_EX(m_fileContent));
}


/**
 * Modify policy from message
 */
UINT32 GenericAgentPolicy::modifyFromMessage(NXCPMessage *msg)
{
   msg->getFieldAsString(VID_NAME, m_name, MAX_DB_STRING);

   if (msg->isFieldExist(VID_CONFIG_FILE_DATA))
   {
      MemFree(m_fileContent);
      m_fileContent = msg->getFieldAsString(VID_CONFIG_FILE_DATA);
   }
   m_version++;
   return RCC_SUCCESS;
}

/**
 * Create deployment message
 */
bool GenericAgentPolicy::createDeploymentMessage(NXCPMessage *msg, bool supportNewTypeFormat)
{
   if (m_fileContent == NULL)
      return false;  // Policy cannot be deployed

#ifdef UNICODE
   char *fd = MBStringFromWideStringSysLocale(m_fileContent);
   msg->setField(VID_CONFIG_FILE_DATA, (BYTE *)fd, (UINT32)strlen(fd));
   free(fd);
#else
   msg->setField(VID_CONFIG_FILE_DATA, (BYTE *)m_fileContent, (UINT32)strlen(m_fileContent));
#endif

   if(supportNewTypeFormat)
   {
      msg->setField(VID_POLICY_TYPE, m_policyType);
   }
   else
   {
      if(!_tcscmp(m_policyType, _T("AgentConfig")))
      {
         msg->setField(VID_POLICY_TYPE, AGENT_POLICY_CONFIG);
      }
      else if(!_tcscmp(m_policyType, _T("LogParserConfig")))
      {
         msg->setField(VID_POLICY_TYPE, AGENT_POLICY_LOG_PARSER);
      }
   }
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_VERSION, m_version);

	return true;
}

/**
 * Serialize object to JSON
 */
json_t *GenericAgentPolicy::toJson()
{
   json_t *root = json_object();
   json_object_set_new(root, "guid", json_string_t((const TCHAR *)m_guid.toString()));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "policyType", json_string_t(m_policyType));
   json_object_set_new(root, "fileContent", json_string_t(CHECK_NULL_EX(m_fileContent)));
   return root;
}

void GenericAgentPolicy::updateFromImport(ConfigEntry *config)
{
   nx_strncpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("unnamed")), MAX_ITEM_NAME);
   nx_strncpy(m_policyType, config->getSubEntryValue(_T("policyType"), 0, _T("none")), MAX_DB_STRING);
   const TCHAR *fileContent=config->getSubEntryValue(_T("fileContent"), 0, _T(""));
   MemFree(m_fileContent);
   m_fileContent = MemCopyString(fileContent);
}

void GenericAgentPolicy::createExportRecord(String &str)
{

   str.appendFormattedString(_T("\t\t\t<agentPolicy id=\"%s\">\n"), (const TCHAR *)m_guid.toString());
   str.append(_T("\t\t\t\t<name>"));
   str.append(m_name);
   str.append(_T("</name>\n"));
   str.append(_T("\t\t\t\t<policyType>"));
   str.append(m_policyType);
   str.append(_T("</policyType>\n"));
   str.append(_T("\t\t\t\t<fileContent>"));
   str.append(m_fileContent);
   str.append(_T("</fileContent>\n"));
   str.append(_T("</agentPolicy>\n"));
}
