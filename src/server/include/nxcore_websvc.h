/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
   WebServiceAuthType m_authType;
   TCHAR *m_login;   // Or token for "bearer" auth
   TCHAR *m_password;
   uint32_t m_cacheRetentionTime;  // milliseconds
   uint32_t m_requestTimeout;      // milliseconds
   StringMap m_headers;

public:
   WebServiceDefinition(const NXCPMessage *msg);
   WebServiceDefinition(DB_HANDLE hdb, DB_RESULT hResult, int row);
   ~WebServiceDefinition();

   uint32_t query(DataCollectionTarget *object, const TCHAR *path, const StringList *args,
            AgentConnection *conn, TCHAR *result) const;
   void fillMessage(NXCPMessage *msg) const;
   void createExportRecord(StringBuffer &xml) const;
   json_t *toJson() const;

   uint32_t getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getUrl() const { return m_url; }
   const TCHAR *getDescription() const { return m_description; }
};

void LoadWebServiceDefinitions();
SharedObjectArray<WebServiceDefinition> *GetWebServiceDefinitions();
shared_ptr<WebServiceDefinition> FindWebServiceDefinition(const TCHAR *name);

#endif
