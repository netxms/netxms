/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
** File: nxcore_websvc.h
**
**/

#ifndef _nxcore_websvc_h_
#define _nxcore_websvc_h_

#include <nms_objects.h>

/**
 * Web service flags
 */
#define WSF_VERIFY_CERTIFICATE      0x0001
#define WSF_VERIFY_HOST             0x0002
#define WSF_FORCE_PLAIN_TEXT_PARSER 0x0004
#define WSF_FOLLOW_LOCATION         0x0008

/**
 * Web service definition
 */
class WebServiceDefinition
{
private:
   uint32_t m_id;
   uuid m_guid;
   TCHAR *m_name;
   TCHAR *m_description;
   TCHAR *m_url;
   HttpRequestMethod m_httpRequestMethod;
   TCHAR *m_requestData;
   WebServiceAuthType m_authType;
   TCHAR *m_login;   // Or token for "bearer" auth
   TCHAR *m_password;
   uint32_t m_cacheRetentionTime;  // milliseconds
   uint32_t m_requestTimeout;      // milliseconds
   StringMap m_headers;
   uint32_t m_flags;

public:
   WebServiceDefinition(const NXCPMessage& msg);
   WebServiceDefinition(const ConfigEntry& config, uint32_t id);
   WebServiceDefinition(DB_HANDLE hdb, DB_RESULT hResult, int row);
   ~WebServiceDefinition();

   uint32_t query(DataCollectionTarget *object, WebServiceRequestType requestType, const TCHAR *path,
            const StringList& args, AgentConnection *conn, void *result) const;
   WebServiceCallResult *makeCustomRequest(shared_ptr<Node> node, const HttpRequestMethod requestMethod,
         const StringList& args, const TCHAR *data, const TCHAR *contentType, bool acceptCached) const;
   void fillMessage(NXCPMessage *msg) const;
   void createExportRecord(TextFileWriter &xml) const;
   json_t *toJson() const;

   uint32_t getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getUrl() const { return m_url; }
   HttpRequestMethod getHttpRequestMethod() const { return m_httpRequestMethod; }
   const TCHAR *getRequestData() const { return m_requestData; }
   const TCHAR *getDescription() const { return m_description; }
   WebServiceAuthType getAuthType() const { return m_authType; }
   const TCHAR *getLogin() const { return m_login; }
   const TCHAR *getPassword() const { return m_password; }
   uint32_t getCacheRetentionTime() const { return m_cacheRetentionTime; }
   uint32_t getRequestTimeout() const { return m_requestTimeout; }
   const StringMap& getHeaders() const { return m_headers; }
   bool isVerifyCertificate() const { return (m_flags & WSF_VERIFY_CERTIFICATE) != 0; }
   bool isVerifyHost() const { return (m_flags & WSF_VERIFY_HOST) != 0; }
   bool isFollowLocation() const { return (m_flags & WSF_FOLLOW_LOCATION) != 0; }
   bool isForcePlainTextParser() const { return (m_flags & WSF_FORCE_PLAIN_TEXT_PARSER) != 0; }
   uint32_t getFlags() const { return m_flags; }
};

void LoadWebServiceDefinitions();
SharedObjectArray<WebServiceDefinition> *GetWebServiceDefinitions();
shared_ptr<WebServiceDefinition> FindWebServiceDefinition(const TCHAR *name);
uint32_t ModifyWebServiceDefinition(shared_ptr<WebServiceDefinition> definition);
uint32_t DeleteWebServiceDefinition(uint32_t id);
void CreateWebServiceDefinitionExportRecord(TextFileWriter &xml, uint32_t count, uint32_t *list);
bool ImportWebServiceDefinition(const ConfigEntry& config, bool overwrite, ImportContext *context);

#endif
