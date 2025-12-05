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
** File: agent_policy.cpp
**
**/

#include "nxcore.h"
#include <nxlpapi.h>
#include <zlib.h>

#define DEBUG_TAG _T("agent.policy")

/**
 * Constructor for user-initiated object creation
 */
GenericAgentPolicy::GenericAgentPolicy(const uuid& guid, const TCHAR *type, uint32_t ownerId) : m_contentLock(MutexType::FAST)
{
   m_name[0] = 0;
   _tcslcpy(m_type, type, MAX_POLICY_TYPE_LEN);
   m_guid = guid;
   m_ownerId = ownerId;
   m_content = nullptr;
   m_version = 1;
   m_flags = 0;
}

/**
 * Constructor for user-initiated object creation
 */
GenericAgentPolicy::GenericAgentPolicy(const TCHAR *name, const TCHAR *type, uint32_t ownerId) : m_contentLock(MutexType::FAST)
{
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   _tcslcpy(m_type, type, MAX_POLICY_TYPE_LEN);
   m_guid = uuid::generate();
   m_ownerId = ownerId;
   m_content = nullptr;
   m_version = 1;
   m_flags = 0;
}

/**
 * Destructor
 */
GenericAgentPolicy::~GenericAgentPolicy()
{
   MemFree(m_content);
}

/**
 * Save to database
 */
bool GenericAgentPolicy::saveToDatabase(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt;
   if (!IsDatabaseRecordExist(hdb, _T("ap_common"), _T("guid"), m_guid)) //Policy can be only created. Policy type can't be changed.
      hStmt = DBPrepare(hdb, _T("INSERT INTO ap_common (policy_name,owner_id,policy_type,file_content,version,flags,guid) VALUES (?,?,?,?,?,?,?)"));
   else
      hStmt = DBPrepare(hdb, _T("UPDATE ap_common SET policy_name=?,owner_id=?,policy_type=?,file_content=?,version=?,flags=? WHERE guid=?"));

   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_ownerId);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_type, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_content, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_version);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_guid);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);

   return success;
}

/**
 * Delete from database
 */
bool GenericAgentPolicy::deleteFromDatabase(DB_HANDLE hdb)
{
   TCHAR query[256], guid[64];
   _sntprintf(query, 256, _T("DELETE FROM ap_common WHERE guid='%s'"), m_guid.toString(guid));
   return DBQuery(hdb, query);
}

/**
 * Load from database
 */
bool GenericAgentPolicy::loadFromDatabase(DB_HANDLE hdb)
{
   bool success = false;

   TCHAR query[256], guid[64];
   _sntprintf(query, 256, _T("SELECT policy_name,owner_id,policy_type,file_content,version,flags FROM ap_common WHERE guid='%s'"), m_guid.toString(guid));
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, m_name, MAX_OBJECT_NAME);
         m_ownerId = DBGetFieldLong(hResult, 0, 1);
         DBGetField(hResult, 0, 2, m_type, MAX_POLICY_TYPE_LEN);
         m_content = DBGetFieldUTF8(hResult, 0, 3, nullptr, 0);
         m_version = DBGetFieldLong(hResult, 0, 4);
         m_flags = DBGetFieldLong(hResult, 0, 5);
         success = true;
      }
      DBFreeResult(hResult);
   }

   return success;
}

/**
 * Create NXCP message with policy data
 */
void GenericAgentPolicy::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_guid);
   msg->setField(baseId + 1, m_type);
   msg->setField(baseId + 2, m_name);
   msg->setFieldFromUtf8String(baseId + 3, CHECK_NULL_EX_A(m_content));
   msg->setField(baseId + 4, m_flags);
}

/**
 * Create NXCP message with policy data for notifications
 */
void GenericAgentPolicy::fillUpdateMessage(NXCPMessage *msg) const
{
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_NAME, m_name);
   msg->setField(VID_POLICY_TYPE, m_type);
   msg->setFieldFromUtf8String(VID_CONFIG_FILE_DATA, CHECK_NULL_EX_A(m_content));
   msg->setField(VID_FLAGS, m_flags);
}

/**
 * Modify policy from message
 */
uint32_t GenericAgentPolicy::modifyFromMessage(const NXCPMessage& msg)
{
   m_contentLock.lock();
   msg.getFieldAsString(VID_NAME, m_name, MAX_DB_STRING);
   if (msg.isFieldExist(VID_CONFIG_FILE_DATA))
   {
      MemFree(m_content);
      m_content = msg.getFieldAsUtf8String(VID_CONFIG_FILE_DATA);
   }
   if (msg.isFieldExist(VID_FLAGS))
   {
      m_flags = msg.getFieldAsUInt32(VID_FLAGS);
   }
   m_version++;
   m_contentLock.unlock();
   return RCC_SUCCESS;
}

/**
 * Create deployment message
 */
bool GenericAgentPolicy::createDeploymentMessage(NXCPMessage *msg, char *content, bool newTypeFormatSupported)
{
   if (content == nullptr)
      return false;  // Policy cannot be deployed

   msg->setField(VID_CONFIG_FILE_DATA, reinterpret_cast<BYTE*>(content), strlen(content));

   if (newTypeFormatSupported)
   {
      msg->setField(VID_POLICY_TYPE, m_type);
   }
   else
   {
      if (!_tcscmp(m_type, _T("AgentConfig")))
      {
         msg->setField(VID_POLICY_TYPE, static_cast<uint16_t>(AGENT_POLICY_CONFIG));
      }
      else if (!_tcscmp(m_type, _T("LogParserConfig")))
      {
         msg->setField(VID_POLICY_TYPE, static_cast<uint16_t>(AGENT_POLICY_LOG_PARSER));
      }
      else
      {
         return false;  // This policy type is not supported by old agents
      }
   }
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_VERSION, m_version);

   return true;
}

/**
 * Policy validation
 */
void GenericAgentPolicy::validate()
{
}

/**
 * Deploy policy to agent. Default implementation calls connector's deployPolicy() method
 */
void GenericAgentPolicy::deploy(shared_ptr<AgentPolicyDeploymentData> data)
{
   shared_ptr<AgentConnectionEx> conn = data->node->getAgentConnection();
   if (conn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("GenericAgentPolicy::deploy(%s): failed - no connection to agent"), data->debugId);
      return;
   }

   bool sendUpdate = true;
   m_contentLock.lock();

   if (!data->forceInstall && data->currVersion >= m_version && (m_flags & EXPAND_MACRO) == 0)
   {
      sendUpdate = false;
   }

   char *content = nullptr;
   if (m_flags & EXPAND_MACRO)
   {
      wchar_t *tmp = WideStringFromUTF8String(m_content);
      StringBuffer expanded = data->node->expandText(tmp);
      content = UTF8StringFromWideString(expanded.cstr());
      MemFree(tmp);
      BYTE newHash[MD5_DIGEST_SIZE];
      if (!data->forceInstall)
      {
         CalculateMD5Hash(reinterpret_cast<BYTE*>(content), strlen(content), newHash);
         if (!memcmp(newHash, data->currHash, MD5_DIGEST_SIZE))
            sendUpdate = false;
      }
   }
   else
   {
      content = MemCopyStringA(m_content);
   }
   m_contentLock.unlock();

   if (sendUpdate)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Calling GenericAgentPolicy::deploy at %s (type=%s, newTypeFormat=%d)"), data->debugId, m_type, data->newTypeFormat);

      NXCPMessage msg(conn->getProtocolVersion());
      if (createDeploymentMessage(&msg, content, data->newTypeFormat))
      {
         uint32_t rcc = conn->deployPolicy(&msg);
         if (rcc == RCC_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("GenericAgentPolicy::deploy(%s): policy successfully deployed"), data->debugId);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("GenericAgentPolicy::deploy(%s) policy deploy failed: %s"), data->debugId, AgentErrorCodeToText(rcc));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("GenericAgentPolicy::deploy(%s): failed to create policy deployment message"), data->debugId);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GenericAgentPolicy::deploy(%s): policy not changed - nothing to install"), data->debugId);
   }

   MemFree(content);
}

/**
 * Serialize object to JSON
 */
json_t *GenericAgentPolicy::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "guid", json_string_t(m_guid.toString()));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "type", json_string_t(m_type));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "content", json_string(CHECK_NULL_EX_A(m_content)));
   return root;
}

/**
 * Create configuration export record
 */
json_t *GenericAgentPolicy::createExportRecord() const
{
   json_t *root = toJson();
   exportAdditionalData(root);
   return root;
}

/**
 * Update policy from imported configuration
 */
void GenericAgentPolicy::updateFromImport(const ConfigEntry *config, ImportContext *context)
{
   _tcslcpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("Unnamed")), MAX_OBJECT_NAME);
   _tcslcpy(m_type, config->getSubEntryValue(_T("type"), 0, _T("Unknown")), MAX_POLICY_TYPE_LEN);
   m_flags = config->getSubEntryValueAsUInt(_T("flags"));
   const TCHAR *content = config->getSubEntryValue(_T("content"), 0, _T(""));
   MemFree(m_content);
   m_content = UTF8StringFromTString(content);
   importAdditionalData(config, context);
}

/**
 * Update policy from imported JSON configuration
 */
void GenericAgentPolicy::updateFromImport(json_t *data, ImportContext *context)
{
   String name = json_object_get_string(data, "name", _T("Unnamed"));
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   String type = json_object_get_string(data, "type", _T("Unknown"));
   _tcslcpy(m_type, type, MAX_POLICY_TYPE_LEN);
   m_flags = json_object_get_uint32(data, "flags", 0);
   MemFree(m_content);
   m_content = MemCopyStringA(json_object_get_string_utf8(data, "content", ""));
   importAdditionalData(data, context);
}

/**
 * Import additional data.
 */
void GenericAgentPolicy::importAdditionalData(const ConfigEntry *config, ImportContext *context)
{
}

/**
 * Import additional data from JSON.
 */
void GenericAgentPolicy::importAdditionalData(json_t *data, ImportContext *context)
{
}

/**
 * Export additional data to JSON.
 */
void GenericAgentPolicy::exportAdditionalData(json_t *root) const
{
}

/**
 * File information
 */
struct FileInfo
{
   uuid guid;
   uuid newGuid;
   TCHAR *path;
   uint32_t permissions;
   TCHAR *owner;
   TCHAR *group;

   FileInfo(const uuid& _guid, const TCHAR *_path, uint32_t _permissions, const TCHAR *_owner, const TCHAR *_group) : guid(_guid)
   {
      path = MemCopyString(_path);
      permissions = _permissions;
      owner = ((_owner != nullptr) && (*_owner != 0)) ? MemCopyString(_owner) : nullptr;
      group = ((_group != nullptr) && (*_group != 0)) ? MemCopyString(_group) : nullptr;
   }

   ~FileInfo()
   {
      MemFree(path);
      MemFree(owner);
      MemFree(group);
   }
};

/**
 * Build file list from path element
 */
static void BuildFileList(ConfigEntry *currEntry, StringBuffer *currPath, ObjectArray<FileInfo> *files, bool updateGuid, bool includeDirectories)
{
   ConfigEntry *children = currEntry->findEntry(_T("children"));
   if (children == nullptr)
      return;

   size_t pathPos = currPath->length();

   currPath->append(currEntry->getAttribute(_T("name")));

   if (includeDirectories)
   {
      files->add(new FileInfo(uuid::NULL_UUID, currPath->cstr(), currEntry->getSubEntryValueAsUInt(_T("permissions")),
               currEntry->getSubEntryValue(_T("owner")), currEntry->getSubEntryValue(_T("ownerGroup"))));
   }

   currPath->append(_T("/"));

   unique_ptr<ObjectArray<ConfigEntry>> elements = children->getSubEntries(_T("*"));
   if (elements != nullptr)
   {
      for(int i = 0; i < elements->size(); i++)
      {
         ConfigEntry *e = elements->get(i);
         uuid guid = e->getSubEntryValueAsUUID(_T("guid"));
         if (!guid.isNull())
         {
            size_t pos = currPath->length();
            currPath->append(e->getAttribute(_T("name")));
            auto f = new FileInfo(guid, currPath->cstr(), e->getSubEntryValueAsUInt(_T("permissions")),
                     e->getSubEntryValue(_T("owner")), e->getSubEntryValue(_T("ownerGroup")));
            files->add(f);
            currPath->shrink(currPath->length() - pos);
            if (updateGuid)
            {
               f->newGuid = uuid::generate();
               e->getSubEntries(_T("guid"))->get(0)->setValue(f->newGuid.toString());
            }
         }
         else
         {
            BuildFileList(e, currPath, files, updateGuid, includeDirectories);
         }
      }
   }

   currPath->shrink(currPath->length() - pathPos);
}

/**
 * Get file list from file policy configuration
 */
static unique_ptr<ObjectArray<FileInfo>> GetFilesFromConfig(const char* content)
{
   auto files = make_unique<ObjectArray<FileInfo>>(64, 64, Ownership::True);
   Config data;
   data.loadXmlConfigFromMemory(content, static_cast<int>(strlen(content)), nullptr, "FileDeliveryPolicy", false);
   unique_ptr<ObjectArray<ConfigEntry>> rootElements = data.getSubEntries(_T("/elements"), _T("*"));
   if (rootElements != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Found elements"));
      for(int i = 0; i < rootElements->size(); i++)
      {
         StringBuffer path;
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Building file list"));
         BuildFileList(rootElements->get(i), &path, files.get(), true, false);
      }
   }
   return files;
}

/**
 * Loads files in base64 coding from imported config
 */
void FileDeliveryPolicy::importAdditionalData(const ConfigEntry *config, ImportContext *context)
{
   unique_ptr<ObjectArray<FileInfo>> configFiles = GetFilesFromConfig(m_content);
   ConfigEntry *fileRoot = config->findEntry(_T("files"));
   if (fileRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> files = fileRoot->getSubEntries(_T("file"));
      for (int i = 0; i < files->size(); i++)
      {
         ConfigEntry* file = files->get(i);
         String fileName(file->getSubEntryValue(_T("name")));
         StringBuffer fullPath = g_netxmsdDataDir;
         fullPath.append(DDIR_FILES FS_PATH_SEPARATOR);
         fullPath.append(fileName);
         int fd = _topen(fullPath.cstr(), O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
         if (fd != -1)
         {
            char *base64Data = MBStringFromWideString(file->getSubEntryValue(L"data"));
            uLongf fileSize = static_cast<uLongf>(file->getSubEntryValueAsUInt64(L"size"));
            char* fileContent;
            size_t decodedFileSize;
            base64_decode_alloc(base64Data, strlen(base64Data), &fileContent, &decodedFileSize);
            MemFree(base64Data);

            if (file->getSubEntryValueAsBoolean(_T("compression")))
            {
               auto buffer = MemAllocArrayNoInit<char>(fileSize);
               uncompress(reinterpret_cast<BYTE*>(buffer), &fileSize, (BYTE *)fileContent, static_cast<uLong>(decodedFileSize));
               MemFree(fileContent);
               fileContent = buffer;
            }

            bool success = (_write(fd, fileContent, fileSize) == static_cast<ssize_t>(fileSize));
            _close(fd);
            MemFree(fileContent);

            if (success)
            {
               BYTE originalHash[MD5_DIGEST_SIZE];
               memset(originalHash, 0, sizeof(originalHash));
               StrToBin(file->getSubEntryValue(_T("hash"), 0, _T("")), originalHash, MD5_DIGEST_SIZE);
               BYTE calculatedHash[MD5_DIGEST_SIZE];
               CalculateFileMD5Hash(fullPath, calculatedHash);
               if (memcmp(originalHash, calculatedHash, MD5_DIGEST_SIZE))
               {
                  context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("Hash mismatch for file %s while importing policy %s in template %s [%u]"),
                     fileName.cstr(), m_name, GetObjectName(m_ownerId, _T("Unknown")), m_ownerId);
                  _tremove(fullPath.cstr());
               }
            }
            else
            {
               context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("Cannot write to file %s while importing policy %s in template %s [%u] (%s)"),
                  fileName.cstr(), m_name, GetObjectName(m_ownerId, _T("Unknown")), m_ownerId, _tcserror(errno));
               _tremove(fullPath.cstr());
            }
         }
         String guid = fileName.substring(13, -1);
         for (int i = 0; i < configFiles->size(); i++)
         {
            if (configFiles->get(i)->guid.equals(uuid::parse(guid)))
            {
               configFiles->remove(i);
               break;
            }
         }
      }
   }

   for (int i = 0; i < configFiles->size(); i++)
   {
      context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("File with GUID %s missing from policy %s"), configFiles->get(i)->guid.toString().cstr(), m_name);
   }
}

/**
 * Export additional data for FileDeliveryPolicy (files)
 */
void FileDeliveryPolicy::exportAdditionalData(json_t *root) const
{
   unique_ptr<ObjectArray<FileInfo>> configFiles = GetFilesFromConfig(m_content);
   if ((configFiles != nullptr) && (configFiles->size() > 0))
   {
      json_t *files = json_array();
      
      for (int i = 0; i < configFiles->size(); i++)
      {
         FileInfo *file = configFiles->get(i);
         
         StringBuffer fileName = _T("FileDelivery-");
         fileName.append(file->guid.toString());
         
         StringBuffer fullPath = g_netxmsdDataDir;
         fullPath.append(DDIR_FILES FS_PATH_SEPARATOR);
         fullPath.append(fileName.cstr());
         
         uint64_t fileSize = FileSize(fullPath.cstr());
         if (fileSize != 0)
         {
            uint64_t maxFileSize = ConfigReadULong(_T("AgentPolicy.MaxFileSize"), 128 * 1024 * 1024);
            if (fileSize < maxFileSize)
            {               
               // Read file content
               int fd = _topen(fullPath.cstr(), O_RDONLY | O_BINARY);
               if (fd != -1)
               {
                  BYTE *fileContent = MemAllocArrayNoInit<BYTE>(fileSize);
                  if (_read(fd, fileContent, static_cast<unsigned int>(fileSize)) == static_cast<ssize_t>(fileSize))
                  {
                     json_t *fileData = json_object();               
                     json_object_set_new(fileData, "name", json_string_t(fileName.cstr()));
                     json_object_set_new(fileData, "size", json_integer(fileSize));

                     uLong compressedFileSize = compressBound(static_cast<uLong>(fileSize));
                     auto compressedFileContent = MemAllocArrayNoInit<BYTE>(compressedFileSize);
                     BYTE *encodedInput;
                     uint64_t encodedInputSize;

                     if (compress(compressedFileContent, &compressedFileSize, fileContent, static_cast<uLong>(fileSize)) == Z_OK)
                     {
                        if (compressedFileSize < fileSize) 
                        {
                           json_object_set_new(fileData, "compression", json_true());
                           encodedInput = compressedFileContent;
                           encodedInputSize = compressedFileSize;
                           MemFree(fileContent);
                        }
                        else
                        {
                           encodedInput = fileContent;
                           encodedInputSize = fileSize;
                           MemFree(compressedFileContent);
                        }
                     }
                     else
                     {
                        encodedInput = fileContent;
                        encodedInputSize = fileSize;
                        MemFree(compressedFileContent);
                     }

                     json_object_set_new(fileData, "data", json_base64_string(encodedInput, encodedInputSize));                    
                     // Add MD5 hash
                     BYTE hash[MD5_DIGEST_SIZE];
                     CalculateFileMD5Hash(fullPath, hash);
                     json_object_set_new(fileData, "hash", json_base64_string(hash, MD5_DIGEST_SIZE));
                        
                     json_array_append_new(files, fileData);                     
                     MemFree(encodedInput);
                  }
                  else
                  {
                     nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot read file %s for export of policy %s in template %s [%u] (%s)"),
                        fileName.cstr(), m_name, GetObjectName(m_ownerId, _T("Unknown")), m_ownerId, _tcserror(errno));
                  }                 
                  MemFree(fileContent);
                  _close(fd);
               }
               else
               {
                  nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot open file %s for export of policy %s in template %s [%u] (%s)"),
                     fileName.cstr(), m_name, GetObjectName(m_ownerId, _T("Unknown")), m_ownerId, _tcserror(errno));
               }
            }
            else
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("File %s in policy %s in template %s [%u] is too big (%llu bytes with limit %llu bytes) and will not be exported"),
                  fileName.cstr(), m_name, GetObjectName(m_ownerId, _T("Unknown")), m_ownerId, 
                  static_cast<unsigned long long>(fileSize), static_cast<unsigned long long>(maxFileSize));
            }
         }
      }
      
      if (json_array_size(files) > 0)
      {
         json_object_set_new(root, "files", files);
      }
      else
      {
         json_decref(files);
      }
   }
}

/**
 * Loads files in base64 coding from imported config
 */
void FileDeliveryPolicy::importAdditionalData(json_t *data, ImportContext *context)
{
   json_t *files = json_object_get(data, "files");
   if (json_is_array(files))
   {
      unique_ptr<ObjectArray<FileInfo>> configFiles = GetFilesFromConfig(m_content);
      
      size_t arraySize = json_array_size(files);
      for (size_t i = 0; i < arraySize; i++)
      {
         json_t *file = json_array_get(files, i);
         if (!json_is_object(file))
            continue;
            
         const char *fileName = json_object_get_string_utf8(file, "name", "");
         if (*fileName == 0)
            continue;
            
         StringBuffer fullPath = g_netxmsdDataDir;
         fullPath.append(DDIR_FILES FS_PATH_SEPARATOR);
         fullPath.append(fileName);
         
         int fd = _topen(fullPath.cstr(), O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
         if (fd != -1)
         {
            const char *base64Data = json_object_get_string_utf8(file, "data", "");
            uint64_t fileSize = json_object_get_uint64(file, "size", 0);
            bool compressed = json_object_get_boolean(file, "compression", false);
            
            char* fileContent;
            size_t decodedFileSize;
            if (base64_decode_alloc(base64Data, strlen(base64Data), &fileContent, &decodedFileSize) == 0)
            {
               if (compressed && fileSize > 0)
               {
                  auto buffer = MemAllocArrayNoInit<char>(fileSize);
                  uLongf uncompressedSize = static_cast<uLongf>(fileSize);
                  if (uncompress(reinterpret_cast<BYTE*>(buffer), &uncompressedSize, reinterpret_cast<BYTE*>(fileContent), static_cast<uLong>(decodedFileSize)) == Z_OK)
                  {
                     MemFree(fileContent);
                     fileContent = buffer;
                     decodedFileSize = uncompressedSize;
                  }
                  else
                  {
                     MemFree(buffer);
                     MemFree(fileContent);
                     context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("Failed to decompress file %hs while importing policy %s"), fileName, m_name);
                     continue;  // Skip to next file since this one failed
                  }
               }
               
               bool success = (_write(fd, fileContent, static_cast<unsigned int>(decodedFileSize)) == static_cast<ssize_t>(decodedFileSize));
               _close(fd);
               
               if (success)
               {
                  // Verify hash if provided
                  const char *expectedHash = json_object_get_string_utf8(file, "hash", nullptr);
                  if (expectedHash != nullptr && strlen(expectedHash) == MD5_DIGEST_SIZE * 2)
                  {
                     BYTE originalHash[MD5_DIGEST_SIZE];
                     StrToBin(TStringFromUTF8String(expectedHash), originalHash, MD5_DIGEST_SIZE);
                     BYTE calculatedHash[MD5_DIGEST_SIZE];
                     CalculateFileMD5Hash(fullPath, calculatedHash);
                     if (memcmp(originalHash, calculatedHash, MD5_DIGEST_SIZE))
                     {
                        context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("Hash mismatch for file %hs while importing policy %s"), fileName, m_name);
                        _tremove(fullPath.cstr());
                     }
                  }
               }
               else
               {
                  context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("Cannot write to file %hs while importing policy %s (%s)"), fileName, m_name, _tcserror(errno));
                  _tremove(fullPath.cstr());
               }
               
               MemFree(fileContent);
            }
            else
            {
               context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("Cannot decode base64 data for file %hs while importing policy %s"), fileName, m_name);
               _close(fd);
               _tremove(fullPath.cstr());
            }
         }
         else
         {
            context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("Cannot create file %hs while importing policy %s (%s)"), fileName, m_name, _tcserror(errno));
         }
         
         // Remove from expected files list
         if (strlen(fileName) > 13) // "FileDelivery-" prefix
         {
            String guid = TStringFromUTF8String(&fileName[13]);
            for (int j = 0; j < configFiles->size(); j++)
            {
               if (configFiles->get(j)->guid.equals(uuid::parse(guid)))
               {
                  configFiles->remove(j);
                  break;
               }
            }
         }
      }
      
      // Warn about missing files
      for (int i = 0; i < configFiles->size(); i++)
      {
         context->log(NXLOG_WARNING, _T("ImportFileData()"), _T("File with GUID %s missing from policy %s"), configFiles->get(i)->guid.toString().cstr(), m_name);
      }
   }
}

/**
 * Modify from message and in case of duplicate - duplicate all physical files and update GUID
 */
uint32_t FileDeliveryPolicy::modifyFromMessage(const NXCPMessage& request)
{
   uint32_t result = GenericAgentPolicy::modifyFromMessage(request);
   if (result != RCC_SUCCESS)
      return result;

   if (request.getFieldAsBoolean(VID_DUPLICATE))
   {
      m_contentLock.lock();
      ObjectArray<FileInfo> files(64, 64, Ownership::True);
      Config data;
      data.loadXmlConfigFromMemory(m_content, static_cast<int>(strlen(m_content)), nullptr, "FileDeliveryPolicy", false);
      unique_ptr<ObjectArray<ConfigEntry>> rootElements = data.getSubEntries(_T("/elements"), _T("*"));
      if (rootElements != nullptr)
      {
         for (int i = 0; i < rootElements->size(); i++)
         {
            StringBuffer path;
            BuildFileList(rootElements->get(i), &path, &files, true, false);
         }
      }
      MemFree(m_content);
      data.setTopLevelTag(_T("FileDeliveryPolicy"));
      m_content = data.createXml().getUTF8String();
      m_contentLock.unlock();

      for (int i = 0; i < files.size(); i++)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::modifyFromMessage(): copy file and update GUID from %s to %s"),
                  files.get(i)->guid.toString().cstr(), files.get(i)->newGuid.toString().cstr());

         StringBuffer sourceFile = g_netxmsdDataDir;
         sourceFile.append(DDIR_FILES FS_PATH_SEPARATOR _T("FileDelivery-"));
         sourceFile.append(files.get(i)->guid.toString());

         StringBuffer destinationFile = g_netxmsdDataDir;
         destinationFile.append(DDIR_FILES FS_PATH_SEPARATOR _T("FileDelivery-"));
         destinationFile.append(files.get(i)->newGuid.toString());

         CopyFileOrDirectory(sourceFile, destinationFile);
      }
   }
   return result;
}

/**
 * File delivery policy destructor
 * deletes all files on policy deletion
 */
bool FileDeliveryPolicy::deleteFromDatabase(DB_HANDLE hdb)
{
   unique_ptr<ObjectArray<FileInfo>> files = GetFilesFromConfig(m_content);
   for (int i = 0; i < files->size(); i++)
   {
      String guid = files->get(i)->guid.toString();
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::modifyFromMessage(): copy file %s and update guid"), guid.cstr());

      StringBuffer sourceFile = g_netxmsdDataDir;
      sourceFile.append(DDIR_FILES FS_PATH_SEPARATOR _T("FileDelivery-"));
      sourceFile.append(guid);

      _tremove(sourceFile);
   }

   return GenericAgentPolicy::deleteFromDatabase(hdb);
}

/**
 * Policy validation
 */
void FileDeliveryPolicy::validate()
{
   m_contentLock.lock();
   if (m_content == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::validate(): empty content"));
      m_contentLock.unlock();
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("FileDeliveryPolicy::validate(): preparing file list"));
   unique_ptr<ObjectArray<FileInfo>> files = GetFilesFromConfig(m_content);
   m_contentLock.unlock();

   for(int i = 0; i < files->size(); i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::validate(): processing file path %s"), files->get(i)->path);

      StringBuffer localFile = g_netxmsdDataDir;
      localFile.append(DDIR_FILES FS_PATH_SEPARATOR _T("FileDelivery-"));
      localFile.append(files->get(i)->guid.toString());

      // Check if the file exists
      if (_taccess(localFile, F_OK) != 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::validate(): failed to find file %s"), files->get(i)->path);
         TCHAR description[MAX_PATH] = _T("Missing policy file ");
         _tcslcat(description, localFile, MAX_PATH);
         EventBuilder(EVENT_POLICY_VALIDATION_ERROR, g_dwMgmtNode)
            .param(_T("templateName"), GetObjectName(m_ownerId, _T("UNKNOWN")))
            .param(_T("templateId"), m_ownerId)
            .param(_T("policyName"), m_name)
            .param(_T("policyType"), m_type)
            .param(_T("policyId"), m_guid)
            .param(_T("additionalInfo"), description)
            .post();
      }
   }
}

/**
 * Convert file permissions to text
 */
static TCHAR *FilePermissionsToText(uint32_t bits, TCHAR *buffer)
{
   _tcscpy(buffer, _T("rwxrwxrwx"));
   for(int i = 0; i <= 8; i++)
      if ((bits & (1 << i)) == 0)
         buffer[i] = _T('-');
   return buffer;
}

/**
 * Deploy file delivery policy
 */
void FileDeliveryPolicy::deploy(shared_ptr<AgentPolicyDeploymentData> data)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): start deployment"), data->debugId);

   if (!data->newTypeFormat)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): failed - file delivery policy not supported by agent"), data->debugId);
      return;
   }

   shared_ptr<AgentConnectionEx> conn = data->node->getAgentConnection();
   if (conn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): failed - no connection to agent"), data->debugId);
      return;
   }

   m_contentLock.lock();
   if (m_content == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): empty content)"), data->debugId);
      m_contentLock.unlock();
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("FileDeliveryPolicy::deploy(%s): preparing file list"), data->debugId);
   unique_ptr<ObjectArray<FileInfo>> files = GetFilesFromConfig(m_content);
   m_contentLock.unlock();

   StringList fileRequest;
   for (int i = 0; i < files->size(); i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): processing file path %s"), data->debugId, files->get(i)->path);
      fileRequest.add(files->get(i)->path);
   }
   ObjectArray<RemoteFileInfo> *remoteFiles;
   uint32_t rcc = conn->getFileSetInfo(&fileRequest, true, &remoteFiles);
   if (rcc != RCC_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): call to AgentConnection::getFileSetInfo failed (%s)"), data->debugId, AgentErrorCodeToText(rcc));
      return;
   }
   if (remoteFiles->size() != files->size())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): inconsistent number of elements returned by AgentConnection::getFileSetInfo (expected %d, returned %d)"),
               data->debugId, files->size(), remoteFiles->size());
      delete remoteFiles;
      return;
   }

   for (int i = 0; i < remoteFiles->size(); i++)
   {
      RemoteFileInfo *remoteFile = remoteFiles->get(i);
      if ((remoteFile->status() != ERR_SUCCESS) && (remoteFile->status() != ERR_FILE_STAT_FAILED))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): file %s with status %d skipped"), data->debugId, remoteFile->name(), remoteFile->status());
         continue;
      }

      FileInfo *localFile = files->get(i);
      if (!localFile->guid.isNull())
      {
         // Regular file, check if upload is needed
         StringBuffer localFilePath(g_netxmsdDataDir);
         localFilePath.append(DDIR_FILES FS_PATH_SEPARATOR _T("FileDelivery-"));
         localFilePath.append(localFile->guid.toString());

         BYTE localHash[MD5_DIGEST_SIZE];
         if (CalculateFileMD5Hash(localFilePath, localHash) && ((remoteFile->status() == ERR_FILE_STAT_FAILED) || memcmp(localHash, remoteFile->hash(), MD5_DIGEST_SIZE)))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): uploading %s"), data->debugId, localFile->path);
            rcc = conn->uploadFile(localFilePath, remoteFile->name(), true);
            if (rcc == ERR_SUCCESS)
               nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): upload completed"), data->debugId);
            else
               nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): upload failed (%s)"), data->debugId, AgentErrorCodeToText(rcc));
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): remote file %s and local file %s are the same, synchronization skipped"), data->debugId, remoteFile->name(), localFilePath.cstr());
         }
      }
      else if (remoteFile->status() == ERR_FILE_STAT_FAILED)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): creating remote directory %s"), data->debugId, localFile->path);
         NXCPMessage request(CMD_FILEMGR_CREATE_FOLDER, conn->generateRequestId(), conn->getProtocolVersion());
         request.setField(VID_FILE_NAME, remoteFile->name());
         request.setField(VID_ALLOW_PATH_EXPANSION, true);
         NXCPMessage *response = conn->customRequest(&request);
         if (response != nullptr)
         {
            rcc = response->getFieldAsUInt32(VID_RCC);
            delete response;
         }
         else
         {
            rcc = ERR_REQUEST_TIMEOUT;
         }
         if (rcc == ERR_SUCCESS)
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): remote directory created"), data->debugId);
         else
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): cannot create remote directory (%s)"), data->debugId, AgentErrorCodeToText(rcc));
      }

      if (((remoteFile->status() == ERR_FILE_STAT_FAILED) && (rcc == ERR_SUCCESS) && ((localFile->owner != nullptr) || (localFile->group != nullptr))) ||
          ((localFile->owner != nullptr) && _tcscmp(CHECK_NULL_EX(remoteFile->ownerUser()), localFile->owner)) ||
          ((localFile->group != nullptr) && _tcscmp(CHECK_NULL_EX(remoteFile->ownerGroup()), localFile->group)))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): changing ownership for %s to %s:%s"),
                  data->debugId, localFile->path, CHECK_NULL(localFile->owner), CHECK_NULL(localFile->group));
         rcc = conn->changeFileOwner(remoteFile->name(), true, localFile->owner, localFile->group);
         if (rcc == ERR_SUCCESS)
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): ownership changed"), data->debugId);
         else
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): cannot change file ownership (%s)"), data->debugId, AgentErrorCodeToText(rcc));
      }

      if (localFile->permissions != remoteFile->permissions())
      {
         TCHAR buffer[16];
         nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): changing permissions for %s to %s"),
                  data->debugId, localFile->path, FilePermissionsToText(localFile->permissions, buffer));
         rcc = conn->changeFilePermissions(remoteFile->name(), true, localFile->permissions, localFile->owner, localFile->group);
         if (rcc == ERR_SUCCESS)
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): permissions changed"), data->debugId);
         else
            nxlog_debug_tag(DEBUG_TAG, 5, _T("FileDeliveryPolicy::deploy(%s): cannot change permissions (%s)"), data->debugId, AgentErrorCodeToText(rcc));
      }
   }
   delete remoteFiles;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("FileDeliveryPolicy::deploy(%s): policy successfully deployed"), data->debugId);
}

/**
 * Get event list used by policy
 */
void LogParserPolicy::getEventList(HashSet<uint32_t> *eventList) const
{
   TCHAR error[1024];
   ObjectArray<LogParser> *parsers = LogParser::createFromXml((const char *)m_content, strlen(m_content), error, 1024, EventNameResolver);
   if (parsers != nullptr)
   {
      for (int i = 0; i < parsers->size(); i++)
      {
         LogParser *parser = parsers->get(i);
         parser->getEventList(eventList);
         delete parser;
      }
      delete parsers;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("LogParserPolicy::getEventList: Cannot create parser from %s configuration"), m_name);
   }
}

/**
 * Checks if any of this policy's LogParsers is using specified event
 */
bool LogParserPolicy::isUsingEvent(uint32_t eventCode) const
{
   TCHAR error[1024];
   unique_ptr<ObjectArray<LogParser>> parsers(LogParser::createFromXml((const char*)m_content, strlen(m_content), error, 1024, EventNameResolver));
   if (parsers != nullptr)
   {
      parsers->setOwner(Ownership::True);
      if (parsers->get(0)->isUsingEvent(eventCode))
      {
         return true;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("LogParserPolicy::isUsingEvent: Cannot create parser from %s configuration"), m_name);
   }
   return false;
}

/**
 * Remove policy from agent. Will destroy provided removal data.
 */
void RemoveAgentPolicy(const shared_ptr<AgentPolicyRemovalData>& data)
{
   shared_ptr<AgentConnectionEx> conn = data->node->getAgentConnection();
   if (conn != nullptr)
   {
      uint32_t rcc = conn->uninstallPolicy(data->guid, data->policyType, data->newTypeFormat);
      if (rcc == ERR_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RemoveAgentPolicy(%s): policy successfully removed"), data->debugId);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("RemoveAgentPolicy(%s): policy removal failed: %s"), data->debugId, AgentErrorCodeToText(rcc));
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RemoveAgentPolicy(%s): policy removal failed: no connection to agent"), data->debugId);
   }
}
