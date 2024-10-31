/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2024 Victor Kirhenshtein
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

#include "aix_subagent.h"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ndd.h>
#include <sys/kinfo.h>

#define sa2sin(x) ((struct sockaddr_in *)x)
#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/**
 * Function getkerninfo() has not documented, but used in IBM examples.
 * It also doesn't have prototype in headers, so we declare it here.
 */
#if !HAVE_DECL_GETKERNINFO
extern "C" int getkerninfo(int, void *, void *, void *);
#endif

/**
 * Internal interface info structure
 */
struct IF_INFO
{
   char name[IFNAMSIZ];
   BYTE mac[6];
   uint32_t ip;
   uint32_t netmask;
   uint32_t iftype;
};

/**
 * Interface data
 */
static perfstat_netinterface_t *s_ifaceData = nullptr;
static int s_ifaceDataSize = 0;
static time_t s_ifaceDataTimestamp = 0;
static Mutex s_ifaceDataLock;

/**
 * Clear network data
 */
void ClearNetworkData()
{
   s_ifaceDataLock.lock();
   free(s_ifaceData);
   s_ifaceData = 0;
   s_ifaceDataSize = 0;
   s_ifaceDataTimestamp = 0;
   s_ifaceDataLock.unlock();
}

/**
 * Get data for all interfaces via libperfstat
 */
static bool GetInterfaceData()
{
   int ifCount = perfstat_netinterface(nullptr, nullptr, sizeof(perfstat_netinterface_t), 0);
   if (ifCount < 1)
      return false;

   if (ifCount != s_ifaceDataSize)
   {
      s_ifaceDataSize = ifCount;
      s_ifaceData = MemReallocArray(s_ifaceData, ifCount);
      if (s_ifaceData == nullptr)
      {
         s_ifaceDataSize = 0;
	 return false;
      }
   }

   perfstat_id_t firstIface;
   strcpy(firstIface.name, FIRST_NETINTERFACE);
   bool success = perfstat_netinterface(&firstIface, s_ifaceData, sizeof(perfstat_netinterface_t), ifCount) > 0;
   if (success)
      s_ifaceDataTimestamp = time(nullptr);
   return success;
}

/**
 * Get interface data
 */
LONG H_NetInterfaceInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char ifName[IF_NAMESIZE];
   if (!AgentGetParameterArgA(param, 1, ifName, IF_NAMESIZE))
      return SYSINFO_RC_UNSUPPORTED;

   // Check if we have interface name or index
   char *eptr;
   unsigned int ifIndex = (unsigned int)strtoul(ifName, &eptr, 10);
   if (*eptr == 0)
   {
      // Index passed as argument, convert to name
      if (if_indextoname(ifIndex, ifName) == nullptr)
      {
         nxlog_debug_tag(AIX_DEBUG_TAG, 7, _T("Unable to resolve interface index %u"), ifIndex);
         return SYSINFO_RC_ERROR;
      }
   }

   s_ifaceDataLock.lock();

	LONG nRet = SYSINFO_RC_SUCCESS;
	if (time(nullptr) - s_ifaceDataTimestamp > 5)
	{
		if (!GetInterfaceData())
			nRet = SYSINFO_RC_ERROR;
	}

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		int i;
		for(i = 0; i < s_ifaceDataSize; i++)
		{
			if (!strcmp(s_ifaceData[i].name, ifName))
				break;
		}
		if (i < s_ifaceDataSize)
		{
			switch(CAST_FROM_POINTER(arg, int))
			{
				case IF_INFO_DESCRIPTION:
					ret_mbstring(value, s_ifaceData[i].description);
					break;
				case IF_INFO_MTU:
					ret_int(value, s_ifaceData[i].mtu);
					break;
				case IF_INFO_SPEED:
					ret_uint(value, s_ifaceData[i].bitrate);
					break;
				case IF_INFO_BYTES_IN:
					ret_uint(value, s_ifaceData[i].ibytes);
					break;
				case IF_INFO_BYTES_OUT:
					ret_uint(value, s_ifaceData[i].obytes);
					break;
				case IF_INFO_PACKETS_IN:
					ret_uint(value, s_ifaceData[i].ipackets);
					break;
				case IF_INFO_PACKETS_OUT:
					ret_uint(value, s_ifaceData[i].opackets);
					break;
				case IF_INFO_IN_ERRORS:
					ret_uint(value, s_ifaceData[i].ierrors);
					break;
				case IF_INFO_OUT_ERRORS:
					ret_uint(value, s_ifaceData[i].oerrors);
					break;
				default:
					nRet = SYSINFO_RC_UNSUPPORTED;
					break;
			}
		}
		else
		{
			nRet = SYSINFO_RC_UNSUPPORTED;
		}
	}

   s_ifaceDataLock.unlock();
   return nRet;
}

/**
 * Get MAC address and type for interface via getkerninfo()
 */
static void GetNDDInfo(char *pszDevice, BYTE *pMacAddr, DWORD *pdwType)
{
   int size = getkerninfo(KINFO_NDD, 0, 0, 0);
   if (size <= 0)
      return;  // getkerninfo() error

   struct kinfo_ndd *nddp = (struct kinfo_ndd *)MemAlloc(size);
   if (getkerninfo(KINFO_NDD, nddp, &size, 0) >= 0)
   {
      int nrec = size / sizeof(struct kinfo_ndd);
      for(int i = 0; i < nrec; i++)
      {
         if (!strcmp(pszDevice, nddp[i].ndd_name) ||
             !strcmp(pszDevice, nddp[i].ndd_alias))
         {
            memcpy(pMacAddr, nddp[i].ndd_addr, 6);
            *pdwType = nddp[i].ndd_type;
            break;
         }
      }
   }
   MemFree(nddp);
}

/**
 * Handler for Net.InterfaceNames enum
 */
LONG H_NetInterfaceNames(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
   int sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock <= 0)
      return SYSINFO_RC_ERROR;

   LONG nRet;
   int nrecs = 32;
   struct ifconf ifc;

retry_ifconf:
   ifc.ifc_len = nrecs * sizeof(struct ifreq);
   ifc.ifc_buf = (caddr_t)MemAlloc(ifc.ifc_len);
#ifdef OSIOCGIFCONF
   if (ioctl(sock, OSIOCGIFCONF, &ifc) == 0)
#else
   if (ioctl(sock, SIOCGIFCONF, &ifc) == 0)
#endif
   {
      if (ifc.ifc_len == nrecs * sizeof(struct ifconf))
      {
         // Assume overlolad, so increase buffer and retry
         MemFree(ifc.ifc_buf);
         nrecs += 32;
         goto retry_ifconf;
      }

      StringSet ifnames;
      nrecs = ifc.ifc_len / sizeof(struct ifreq);
      for(int i = 0; i < nrecs; i++)
      {
#ifdef UNICODE
         ifnames.addPreallocated(WideStringFromMBString(ifc.ifc_req[i].ifr_name));
#else
         ifnames.add(ifc.ifc_req[i].ifr_name);
#endif
      }

      for(const TCHAR* n : ifnames)
         value->add(n);

      nRet = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }

   MemFree(ifc.ifc_buf);
   close(sock);
   return nRet;
}

/**
 * Handler for Net.InterfaceList enum
 */
LONG H_NetInterfaceList(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
   int sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock <= 0)
      return SYSINFO_RC_ERROR;

   LONG nRet;
   int nrecs = 32;
   struct ifconf ifc;

retry_ifconf:
   ifc.ifc_len = nrecs * sizeof(struct ifreq);
   ifc.ifc_buf = (caddr_t)MemAlloc(ifc.ifc_len);
#ifdef OSIOCGIFCONF
   if (ioctl(sock, OSIOCGIFCONF, &ifc) == 0)
#else
   if (ioctl(sock, SIOCGIFCONF, &ifc) == 0)
#endif
   {
      if (ifc.ifc_len == nrecs * sizeof(struct ifconf))
      {
         // Assume overlolad, so increase buffer and retry
         MemFree(ifc.ifc_buf);
         nrecs += 32;
         goto retry_ifconf;
      }

      nrecs = ifc.ifc_len / sizeof(struct ifreq);
      IF_INFO *ifl = MemAllocArray<IF_INFO>(nrecs);
      int nifs = 0;
      for(int i = 0; i < nrecs; i++)
      {
         // Check if interface already in internal table
         int j;
         for(j = 0; j < nifs; j++)
				if (!strcmp(ifl[j].name, ifc.ifc_req[i].ifr_name) &&
				    (((ifc.ifc_req[i].ifr_addr.sa_family == AF_INET) && 
				      (((struct sockaddr_in *)&ifc.ifc_req[i].ifr_addr)->sin_addr.s_addr == ifl[j].ip)) ||
				     (ifc.ifc_req[i].ifr_addr.sa_family != AF_INET) ||
				     (ifl[j].ip == 0)))
					break;
			if (j == nifs)
			{
				strcpy(ifl[j].name, ifc.ifc_req[i].ifr_name);
				GetNDDInfo(ifc.ifc_req[i].ifr_name, ifl[j].mac, &ifl[j].iftype);
				nifs++;
			}

			// Copy IP address
			if (ifc.ifc_req[i].ifr_addr.sa_family == AF_INET)
			{
				ifl[j].ip = ((struct sockaddr_in *)&ifc.ifc_req[i].ifr_addr)->sin_addr.s_addr;
	         
            struct ifreq ifr;
				strcpy(ifr.ifr_name, ifc.ifc_req[i].ifr_name);
				ifr.ifr_addr.sa_family = AF_INET;
				if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0)
				{
					ifl[j].netmask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
				}
			}
		}

		// Create result list
		for(int i = 0; i < nifs; i++)
		{
	      char szBuffer[256], szIpAddr[16], szMacAddr[32];
			sprintf(szBuffer, "%d %s/%d %d %s %s",
			        if_nametoindex(ifl[i].name),
			        IpToStrA(ntohl(ifl[i].ip), szIpAddr),
			        BitsInMask(ifl[i].netmask), ifl[i].iftype,
				BinToStrA(ifl[i].mac, 6, szMacAddr), ifl[i].name);
#ifdef UNICODE
			value->addPreallocated(WideStringFromMBString(szBuffer));
#else
			value->add(szBuffer);
#endif
      }
      MemFree(ifl);
      nRet = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }

   MemFree(ifc.ifc_buf);
   close(sock);
   return nRet;
}

/**
 * Handler for Net.Interface.AdminStatus parameter
 */
LONG H_NetInterfaceStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char ifName[IF_NAMESIZE];
   if (!AgentGetParameterArgA(param, 1, ifName, IF_NAMESIZE))
      return SYSINFO_RC_UNSUPPORTED;

   // Check if we have interface name or index
   char *eptr;
   unsigned int ifIndex = (unsigned int)strtoul(ifName, &eptr, 10);
   if (*eptr == 0)
   {
      // Index passed as argument, convert to name
      if (if_indextoname(ifIndex, ifName) == nullptr)
      {
         nxlog_debug_tag(AIX_DEBUG_TAG, 7, _T("Unable to resolve interface index %u"), ifIndex);
         return SYSINFO_RC_ERROR;
      }
   }

   int requestedFlag = 0;
   switch(CAST_FROM_POINTER(arg, int))
   {
      case IF_INFO_ADMIN_STATUS:
         requestedFlag = IFF_UP;
         break;
      case IF_INFO_OPER_STATUS:
         requestedFlag = IFF_RUNNING;
         break;
      default:
         nxlog_debug_tag(AIX_DEBUG_TAG, 7, _T("Internal error in H_NetIfterfaceStatus (invalid flag requested)"));
         return SYSINFO_RC_UNSUPPORTED;
   }

   int nSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (nSocket < 0)
   {
      nxlog_debug_tag(AIX_DEBUG_TAG, 7, _T("Cannot create socket (%hs)"), strerror(errno));
      return SYSINFO_RC_ERROR;
   }

   int nRet = SYSINFO_RC_ERROR;
   struct ifreq ifr;
   memset(&ifr, 0, sizeof(ifr));
   strlcpy(ifr.ifr_name, ifName, sizeof(ifr.ifr_name));
   if (ioctl(nSocket, SIOCGIFFLAGS, (caddr_t)&ifr) >= 0)
   {
      if ((ifr.ifr_flags & requestedFlag) == requestedFlag)
      {
         // enabled
         ret_int(value, 1);
      }
      else
      {
         ret_int(value, 2);
      }
      nRet = SYSINFO_RC_SUCCESS;
   }

   close(nSocket);
   return nRet;
}

/**
 * Handler for Net.IP.RoutingTable list
 */
LONG H_NetRoutingTable(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int reqSize = getkerninfo(KINFO_RT_DUMP, 0, 0, 0);
   if (reqSize <= 0)
   {
      nxlog_debug_tag(AIX_DEBUG_TAG, 6, _T("H_NetRoutingTable: call to getkerninfo() failed (%hs)"), strerror(errno));
      return SYSINFO_RC_ERROR;
   }

   char *rt = static_cast<char*>(MemAlloc(reqSize));
   if (getkerninfo(KINFO_RT_DUMP, rt, &reqSize, 0) < 0)
   {
      MemFreeAndNull(rt);
      nxlog_debug_tag(AIX_DEBUG_TAG, 6, _T("H_NetRoutingTable: call to getkerninfo() failed (%hs)"), strerror(errno));
      return SYSINFO_RC_ERROR;
   }

   struct rt_msghdr *rtm, *prtm = nullptr;
   for (char *next = rt; next < rt + reqSize; next += rtm->rtm_msglen)
   {
      rtm = (struct rt_msghdr *)next;
      if ((prtm != nullptr) && (prtm->rtm_msglen == rtm->rtm_msglen) && !memcmp(prtm, rtm, rtm->rtm_msglen))
         continue;

      prtm = rtm;
      auto sa = reinterpret_cast<struct sockaddr*>(rtm + 1);
      if (sa->sa_family != AF_INET)
         continue;

      struct sockaddr *rti_info[RTAX_MAX];
      for (int i = 0; i < RTAX_MAX; i++)
      {
         if (rtm->rtm_addrs & (1 << i))
         {
            rti_info[i] = sa;
            sa = reinterpret_cast<struct sockaddr*>(reinterpret_cast<char*>(sa) + ROUNDUP(sa->sa_len));
         }
         else
         {
            rti_info[i] = nullptr;
         }
      }

      if ((rti_info[RTAX_DST] != nullptr) && !(rtm->rtm_flags & (RTF_CLONED | RTF_REJECT)))
      {
         char route[1024];

         if ((rtm->rtm_flags & RTF_HOST) || (rti_info[RTAX_NETMASK] == nullptr))
         {
            // host
            strcpy(route, inet_ntoa(sa2sin(rti_info[RTAX_DST])->sin_addr));
            strcat(route, "/32 ");
         }
	 else if (sa2sin(rti_info[RTAX_DST])->sin_addr.s_addr == INADDR_ANY)
         {
            strcpy(route, "0.0.0.0/0 ");
         }
         else
         {
            // net
            int mask = (rti_info[RTAX_NETMASK] != nullptr) ? ntohl(sa2sin(rti_info[RTAX_NETMASK])->sin_addr.s_addr) : 0;
            sprintf(route, "%s/%d ", inet_ntoa(sa2sin(rti_info[RTAX_DST])->sin_addr), BitsInMask(mask));
         }

         if (rti_info[RTAX_GATEWAY]->sa_family != AF_INET)
         {
            strcat(route, "0.0.0.0 ");
         }
         else
         {
            strcat(route, inet_ntoa(sa2sin(rti_info[RTAX_GATEWAY])->sin_addr));
            strcat(route, " ");
         }

         int routeType;
         if (rtm->rtm_flags & RTF_LOCAL)
            routeType = 2; // local
	 else if (rtm->rtm_flags & RTF_STATIC)
            routeType = 3; // netmgmt
         else if (rtm->rtm_flags & RTF_DYNAMIC)
            routeType = 4; // icmp
         else
            routeType = 1; // other

         char tmp[64];
         snprintf(tmp, sizeof(tmp), "%d %d %u %d", rtm->rtm_index, (rtm->rtm_flags & RTF_GATEWAY) == 0 ? 3 : 4, rtm->rtm_rmx.rmx_hopcount, routeType);
         strcat(route, tmp);

         value->addMBString(route);
      }
   }

   MemFree(rt);
   return SYSINFO_RC_SUCCESS;
}
