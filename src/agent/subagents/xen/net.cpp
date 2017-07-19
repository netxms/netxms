/*
** NetXMS XEN hypervisor subagent
** Copyright (C) 2017 Raden Solutions
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
** File: net.cpp
**
**/

#include "xen.h"

/**
 * Network device information
 */
struct NetDevice
{
   char name[64];
   uint32_t domId;
   uint32_t netId;
   UINT64 rxBytes;
   UINT64 txBytes;
   UINT64 rxPackets;
   UINT64 txPackets;
};

/**
 * Get domaind and network ID from network device name
 */
static bool NetDeviceToDomId(const char *dev, uint32_t *domId, uint32_t *netId)
{
   if (sscanf(dev, "vif%u.%u", domId, netId) == 2)
      return true;

   char path[128];
   snprintf(path, 128, "/sys/class/net/%s/device/nodename", dev);
   FILE *f = fopen(path, "r");
   if (path != NULL)
   {
      char line[256];
      fgets(line, 256, f);
      fclose(f);
      if (sscanf(line, "backend/vif/%u/%u", domId, netId) == 2)
         return true;
   }

   return false;
}

/**
 * Scan network devices
 */
ObjectArray<NetDevice> *ScanNetworkDevices()
{
   FILE *f = fopen("/proc/net/dev", "r");
   if (f == NULL)
      return NULL;

   char line[1024];
   NetDevice dev;

   ObjectArray<NetDevice> *devices = new ObjectArray<NetDevice>(32, 32, true);
   while(!feof(f))
   {
      fgets(line, 1024, f);
      if (sscanf(line, "%s: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu %*u %*u %*u %*u %*u %*u",
               dev.name, &dev.rxBytes, &dev.rxPackets, &dev.txBytes, &dev.txPackets) == 5)
      {
         if (NetDeviceToDomId(dev.name, &dev.domId, &dev.netId))
         {
            devices->add(new NetDevice(dev));
         }
      }
   }
   fclose(f);
   return devices;
}

/**
 * Network device data cache
 */
static ObjectArray<NetDevice> *s_cache = NULL;
static time_t s_cacheTimestamp = 0;
static Mutex s_cacheLock;

/**
 * Acquire device data cache and update if needed
 */
inline void AcquireNetDeviceCache()
{
   s_cacheLock.lock();
   time_t now = time(NULL);
   if (now - s_cacheTimestamp > 1)
   {
      delete s_cache;
      s_cache = ScanNetworkDevices();
      if (s_cache != NULL)
         s_cacheTimestamp = now;
   }
}

/**
 * Query network traffic for domain
 */
bool XenQueryDomainNetworkTraffic(uint32_t domId, UINT64 *rxBytes, UINT64 *txBytes, UINT64 *rxPackets, UINT64 *txPackets)
{
   bool success = false;

   *rxBytes = 0;
   *txBytes = 0;
   *rxPackets = 0;
   *txPackets = 0;

   AcquireNetDeviceCache();
   if (s_cache != NULL)
   {
      for(int i = 0; i < s_cache->size(); i++)
      {
         NetDevice *d = s_cache->get(i);
         if (d->domId == domId)
         {
            *rxBytes += d->rxBytes;
            *txBytes += d->txBytes;
            *rxPackets += d->rxPackets;
            *txPackets += d->txPackets;
         }
      }
      success = true;
   }
   s_cacheLock.unlock();
   return success;
}

/**
 * Handler for XEN.Domain.Net.* parameters
 */
LONG H_XenDomainNetStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char domName[256];
   if (!AgentGetParameterArgA(param, 1, domName, 256))
      return SYSINFO_RC_UNSUPPORTED;

   char *eptr;
   uint32_t domId = strtoul(domName, &eptr, 0);
   if (*eptr != 0)
   {
      LONG rc = XenResolveDomainName(domName, &domId);
      if (rc != SYSINFO_RC_SUCCESS)
         return rc;
   }

   LONG rc = SYSINFO_RC_ERROR;
   UINT64 rxBytes, txBytes, rxPackets, txPackets;
   if (XenQueryDomainNetworkTraffic(domId, &rxBytes, &txBytes, &rxPackets, &txPackets))
   {
      if (arg[0] == 'R')
      {
         ret_uint64(value, arg[1] == 'B' ? rxBytes : rxPackets);
      }
      else
      {
         ret_uint64(value, arg[1] == 'B' ? txBytes : txPackets);
      }
      rc = SYSINFO_RC_SUCCESS;
   }
   return rc;
}
