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
   if (f != NULL)
   {
      char line[256] = "";
      fgets(line, 256, f);
      fclose(f);
      if (sscanf(line, "backend/vif/%u/%u", domId, netId) == 2)
         return true;
      nxlog_debug(6, _T("XEN: NetDeviceToDomId(%hs): cannot parse /sys/class/net/%hs/device/nodename (content is \"%hs\")"), dev, dev, line);
   }
   else
   {
      nxlog_debug(6, _T("XEN: NetDeviceToDomId(%hs): cannot open /sys/class/net/%hs/device/nodename"), dev, dev);
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
   {
      nxlog_debug(6, _T("XEN: ScanNetworkDevices: cannot open /proc/net/dev (%hs)"), strerror(errno));
      return NULL;
   }

   char line[1024];
   NetDevice dev;

   ObjectArray<NetDevice> *devices = new ObjectArray<NetDevice>(32, 32, true);
   while(!feof(f))
   {
      if (fgets(line, 1024, f) == NULL)
         break;
      if (sscanf(line, " %[^:]: %llu %llu %*u %*u %*u %*u %*u %*u %llu %llu %*u %*u %*u %*u %*u %*u",
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
 * Get NIC data
 */
static bool GetNicData(libxl_ctx *ctx, uint32_t domId, uint32_t netId, libxl_device_nic *data)
{
   int count;
   libxl_device_nic *nicList = libxl_device_nic_list(ctx, domId, &count);
   if (nicList == NULL)
      return false;

   bool found = false;
   for(int i = 0; i < count; i++)
   {
      if (nicList[i].devid == netId)
      {
         memcpy(data, &nicList[i], sizeof(libxl_device_nic));
         found = true;
         break;
      }
   }
   return found;
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
            success = true;
         }
      }
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

/**
 * Handler for XEN.VirtualMachines table
 */
LONG H_XenDomainNetIfTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   libxl_ctx *ctx;
   XEN_CONNECT(ctx);

   LONG rc = SYSINFO_RC_ERROR;

   AcquireNetDeviceCache();
   if (s_cache != NULL)
   {
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
      value->addColumn(_T("DOMAIN_ID"), DCI_DT_UINT, _T("Domain ID"));
      value->addColumn(_T("DOMAIN_NAME"), DCI_DT_STRING, _T("Domain name"));
      value->addColumn(_T("NET_ID"), DCI_DT_UINT, _T("Network ID"));
      value->addColumn(_T("BRIDGE"), DCI_DT_STRING, _T("Bridge"));
      value->addColumn(_T("MAC_ADDR"), DCI_DT_STRING, _T("MAC Address"));
      value->addColumn(_T("IP_ADDR"), DCI_DT_STRING, _T("IP Address"));
      value->addColumn(_T("MODEL"), DCI_DT_STRING, _T("Model"));
      value->addColumn(_T("RX_BYTES"), DCI_DT_UINT64, _T("Bytes Rx"));
      value->addColumn(_T("TX_BYTES"), DCI_DT_UINT64, _T("Bytes Tx"));
      value->addColumn(_T("RX_PACKETS"), DCI_DT_UINT64, _T("Packets Rx"));
      value->addColumn(_T("TX_PACKETS"), DCI_DT_UINT64, _T("Packets Tx"));
      for(int i = 0; i < s_cache->size(); i++)
      {
         NetDevice *d = s_cache->get(i);
         value->addRow();
         value->set(0, d->name);
         value->set(1, d->domId);
         value->set(2, libxl_domid_to_name(ctx, d->domId));
         value->set(3, d->netId);

         libxl_device_nic nicData;
         if (GetNicData(ctx, d->domId, d->netId, &nicData))
         {
            value->set(4, nicData.bridge);
            TCHAR buffer[64];
            value->set(5, MACToStr(nicData.mac, buffer));
            value->set(6, nicData.ip);
            value->set(7, nicData.model);
         }

         value->set(8, d->rxBytes);
         value->set(9, d->txBytes);
         value->set(10, d->rxPackets);
         value->set(11, d->txPackets);
      }
      rc = SYSINFO_RC_SUCCESS;
   }

   s_cacheLock.unlock();
   libxl_ctx_free(ctx);
   return rc;
}
