/*
** NetXMS Prometheus remote write receiver subagent
** Copyright (C) 2025 Raden Solutions
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
**/

#ifndef _prometheus_h_
#define _prometheus_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include "generated/remote.pb.h"

/**
 * Metric mapping structure
 */
struct MetricMapping
{
   TCHAR prometheusName[256];
   TCHAR netxmsName[256];
   StringList nameArguments;
   StringList valueArguments;
   bool useMetricValue;

   MetricMapping()
   {
      prometheusName[0] = 0;
      netxmsName[0] = 0;
      useMetricValue = true;
   }
};

bool HandleWriteRequest(const char *data, size_t size);
const ObjectArray<MetricMapping>& GetMetricMappings();

void PrintWriteRequest(const prometheus::WriteRequest& request);

bool StartReceiver(const TCHAR *listenAddress, uint16_t port, const TCHAR *endpoint);
void StopReceiver();

#define DEBUG_TAG _T("prometheus")

#endif
