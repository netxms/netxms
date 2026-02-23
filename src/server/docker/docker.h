/*
** NetXMS Docker cloud connector module
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
** File: docker.h
**/

#ifndef _docker_h_
#define _docker_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <cloud-connector.h>
#include <nxlibcurl.h>

#define DEBUG_TAG L"docker"

/**
 * Docker Engine API client
 */
class DockerClient
{
private:
   char m_baseUrl[512];
   char m_socketPath[256];
   bool m_useTLS;
   char *m_caCert;
   char *m_clientCert;
   char *m_clientKey;

public:
   DockerClient();
   ~DockerClient();

   bool configure(json_t *credentials);
   json_t *httpGet(const char *path);
};

/**
 * Map Docker container state string to RESOURCE_STATE_* value
 */
int16_t DockerContainerStateToResourceState(const char *dockerState);

/**
 * Cloud connector interface functions
 */
ResourceDescriptor *DockerDiscover(json_t *credentials, const TCHAR *filter);

ObjectArray<MetricDefinition> *DockerGetMetricDefinitions(const TCHAR *resourceType, const TCHAR *resourceId,
   json_t *credentials);

DataCollectionError DockerCollect(const TCHAR *resourceId, const TCHAR *metric, int16_t aggregation,
   TCHAR *value, size_t bufLen, json_t *credentials);

DataCollectionError DockerCollectTable(const TCHAR *resourceId, const TCHAR *metric, int16_t aggregation,
   shared_ptr<Table> *value, json_t *credentials);

int16_t DockerQueryState(const TCHAR *resourceId, char *providerState, size_t bufLen, json_t *credentials);

#endif
