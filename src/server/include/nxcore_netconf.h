/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: nxcore_netconf.h
**
**/

#ifndef _nxcore_netconf_h_
#define _nxcore_netconf_h_

#include <nms_objects.h>

/**
 * NETCONF datastore selection in query definition (0 = get operation covering state and
 * configuration data, others = get-config from given datastore)
 */
#define NETCONF_DATASTORE_OPERATIONAL  0
#define NETCONF_DATASTORE_RUNNING      1
#define NETCONF_DATASTORE_CANDIDATE    2
#define NETCONF_DATASTORE_STARTUP      3

/**
 * NETCONF filter types in query definition
 */
#define NETCONF_FILTER_NONE      0
#define NETCONF_FILTER_SUBTREE   1
#define NETCONF_FILTER_XPATH     2

/**
 * NETCONF query definition - named get/get-config request shared by multiple DCIs,
 * each DCI extracting a value from the cached result document by XPath
 */
class NXCORE_EXPORTABLE NetconfQueryDefinition
{
private:
   uint32_t m_id;
   uuid m_guid;
   TCHAR *m_name;
   TCHAR *m_description;
   int16_t m_datastore;
   int16_t m_filterType;
   TCHAR *m_filter;
   uint32_t m_cacheRetentionTime;  // seconds
   uint32_t m_requestTimeout;      // milliseconds
   uint32_t m_flags;

public:
   NetconfQueryDefinition(const NXCPMessage& msg);
   NetconfQueryDefinition(DB_HANDLE hdb, DB_RESULT hResult, int row);
   ~NetconfQueryDefinition();

   void fillMessage(NXCPMessage *msg) const;

   uint32_t getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDescription() const { return m_description; }
   int16_t getDatastore() const { return m_datastore; }
   int16_t getFilterType() const { return m_filterType; }
   const TCHAR *getFilter() const { return m_filter; }
   uint32_t getCacheRetentionTime() const { return m_cacheRetentionTime; }
   uint32_t getRequestTimeout() const { return m_requestTimeout; }
   uint32_t getFlags() const { return m_flags; }
};

void LoadNetconfQueryDefinitions();
SharedObjectArray<NetconfQueryDefinition> *GetNetconfQueryDefinitions();
shared_ptr<NetconfQueryDefinition> NXCORE_EXPORTABLE FindNetconfQueryDefinition(uint32_t id);
shared_ptr<NetconfQueryDefinition> NXCORE_EXPORTABLE FindNetconfQueryDefinition(const TCHAR *name);
uint32_t NXCORE_EXPORTABLE ModifyNetconfQueryDefinition(shared_ptr<NetconfQueryDefinition> definition);
uint32_t NXCORE_EXPORTABLE DeleteNetconfQueryDefinition(uint32_t id);

#endif   /* _nxcore_netconf_h_ */
