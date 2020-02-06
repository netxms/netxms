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
   UINT32 m_id;
   uuid m_guid;
   TCHAR *m_name;
   TCHAR *m_url;
   WebServiceAuthType m_authType;
   TCHAR *m_login;   // Or token for "bearer" auth
   TCHAR *m_password;
   UINT32 m_cacheRetentionTime;  // milliseconds
   UINT32 m_requestTimeout;      // milliseconds
   StringMap m_headers;

public:
   WebServiceDefinition(const NXCPMessage *msg);
   WebServiceDefinition(DB_HANDLE hdb, DB_RESULT hResult, int row);
   ~WebServiceDefinition();

   UINT32 query(DataCollectionTarget *object, const TCHAR *path, const StringList *args,
            AgentConnection *conn, TCHAR *result) const;
   void fillMessage(NXCPMessage *msg) const;
};

void LoadWebServiceDefinitions();
shared_ptr<WebServiceDefinition> GetWebServiceDefinition(const TCHAR *name);

#endif
