/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2008 Mark Ibell
** Copyright (C) 2016-2023 Raden Solutions
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

#include <nms_common.h>
#include <nms_agent.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/ethernet.h>

#if __FreeBSD__ >= 13
#include <net/if_mib.h>
#else
#include <kvm.h>
#include <nlist.h>
#endif

#if HAVE_NET_ISO88025_H
#include <net/iso88025.h>
#endif

#include "freebsd_subagent.h"

#define sa2sin(x) ((struct sockaddr_in *)x)
#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

struct IFLIST
{
   char *name;
   struct ether_addr *mac;
   InetAddress *addr;
   int addrCount;
   int index;
   int type;
   int mtu;
};

/**
 * Handler for Net.IP.Forwarding parameter
 */
LONG H_NetIpForwarding(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	int nVer = CAST_FROM_POINTER(arg, int);
	int nRet = SYSINFO_RC_ERROR;
	int mib[4];
	size_t nSize = sizeof(mib), nValSize;
	int nVal;

	if (nVer == 6)
	{
		return ERR_NOT_IMPLEMENTED;
	}

	if (sysctlnametomib("net.inet.ip.forwarding", mib, &nSize) != 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nValSize = sizeof(nVal);
	if (sysctl(mib, nSize, &nVal, &nValSize, NULL, 0) == 0)
	{
		if (nVal == 0 || nVal == 1)
		{
			ret_int(value, nVal);
			nRet = SYSINFO_RC_SUCCESS;
		}
	}

	return nRet;
}

/**
 * Handler for Net.Interface.AdminStatus parameter
 */
LONG H_NetIfAdminStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szArg[512];

	AgentGetParameterArgA(param, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0)
	{
		if (szArg[0] >= '0' && szArg[0] <= '9')
		{
			// index
			if (if_indextoname(atoi(szArg), szArg) != szArg)
			{
				// not found
				nRet = SYSINFO_RC_ERROR;
			}
		}

		if (nRet == SYSINFO_RC_SUCCESS)
		{
			int nSocket;

			nRet = SYSINFO_RC_ERROR;

			nSocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (nSocket > 0)
			{
				struct ifreq ifr;
				int flags;

				memset(&ifr, 0, sizeof(ifr));
				strncpy(ifr.ifr_name, szArg, sizeof(ifr.ifr_name));
				if (ioctl(nSocket, SIOCGIFFLAGS, (caddr_t)&ifr) >= 0)
				{
					flags = (ifr.ifr_flags & 0xffff) | (ifr.ifr_flagshigh << 16);
					if ((flags & IFF_UP) == IFF_UP)
					{
						// enabled
						ret_int(value, 1);
						nRet = SYSINFO_RC_SUCCESS;
					}
					else
					{
						ret_int(value, 2);
						nRet = SYSINFO_RC_SUCCESS;
					}
				}
				close(nSocket);
			}
		}
	}

	return nRet;
}

/**
 * Handler for Net.Interface.OperStatus parameter
 */
LONG H_NetIfOperStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szArg[512];

	AgentGetParameterArgA(param, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0)
	{
		if (szArg[0] >= '0' && szArg[0] <= '9')
		{
			// index
			if (if_indextoname(atoi(szArg), szArg) != szArg)
			{
				// not found
				nRet = SYSINFO_RC_ERROR;
			}
		}

		if (nRet == SYSINFO_RC_SUCCESS)
		{
			int nSocket;

			nRet = SYSINFO_RC_ERROR;

			nSocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (nSocket > 0)
			{
				struct ifmediareq ifmr;

				memset(&ifmr, 0, sizeof(ifmr));
				strlcpy(ifmr.ifm_name, szArg, sizeof(ifmr.ifm_name));
				if (ioctl(nSocket, SIOCGIFMEDIA, (caddr_t)&ifmr) >= 0)
				{
					if ((ifmr.ifm_status & IFM_AVALID) == IFM_AVALID &&
							(ifmr.ifm_status & IFM_ACTIVE) == IFM_ACTIVE)
					{
						ret_int(value, 1);
						nRet = SYSINFO_RC_SUCCESS;
					}
					else
					{
						ret_int(value, 0);
						nRet = SYSINFO_RC_SUCCESS;
					}
				}
            else if (errno == EINVAL || errno == ENOTTY)
            {
               // ifmedia not supported, assume the status is NORMAL
               ret_int(value, 1);
               nRet = SYSINFO_RC_SUCCESS;
            }
				close(nSocket);
			}
		}
	}

	return nRet;
}

/**
 * Handler for Net.ArpCache list
 */
LONG H_NetArpCache(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	int mib[6] = { CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO };
	size_t nNeeded;
	char *pNext, *pBuff;
	struct rt_msghdr *pRtm = NULL;
	struct sockaddr_inarp *pSin;
	struct sockaddr_dl *pSdl;
	char *pLim;

	if (sysctl(mib, 6, NULL, &nNeeded, NULL, 0) != 0)
	{
		return SYSINFO_RC_ERROR;
	}
	if ((pBuff = (char *)malloc(nNeeded)) == NULL)
	{
		return SYSINFO_RC_ERROR;
	}
	if (sysctl(mib, 6, pBuff, &nNeeded, NULL, 0) != 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nRet = SYSINFO_RC_SUCCESS;
	pLim = pBuff + nNeeded;
	for (pNext = pBuff; pNext < pLim; pNext += pRtm->rtm_msglen)
	{
		char szBuff[256];
		struct ether_addr *pEa;

		pRtm = (struct rt_msghdr *)pNext;
		pSin = (struct sockaddr_inarp *)(pRtm + 1);

		pSdl = (struct sockaddr_dl *)((char *)pSin + (pSin->sin_len > 0
					?
						(1 + ((pSin->sin_len - 1) | (sizeof(long) - 1)))
					:
						sizeof(long))
				);

		if (pSdl->sdl_alen != 6)
		{
			continue;
		}

		pEa = (struct ether_addr *)LLADDR(pSdl);

		if (memcmp(pEa->octet, "\377\377\377\377\377\377", 6) == 0)
		{
			// broadcast
			continue;
		}

		snprintf(szBuff, sizeof(szBuff),
				"%02X%02X%02X%02X%02X%02X %s %d",
				pEa->octet[0], pEa->octet[1],
				pEa->octet[2], pEa->octet[3],
				pEa->octet[4], pEa->octet[5],
				inet_ntoa(pSin->sin_addr),
				pSdl->sdl_index);
		value->addMBString(szBuff);
	}

	return nRet;
}

/**
 * Handler for Net.IP.RoutingTable list
 */
LONG H_NetRoutingTable(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int mib[6] = { CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_DUMP, 0 };
   char *rt = nullptr;
   size_t reqSize = 0;
   if (sysctl(mib, 6, nullptr, &reqSize, nullptr, 0) == 0)
   {
      if (reqSize > 0)
      {
         rt = static_cast<char*>(MemAlloc(reqSize));
         if ((rt != nullptr) && (sysctl(mib, 6, rt, &reqSize, nullptr, 0) < 0))
         {
            MemFreeAndNull(rt);
         }
      }
   }

   if (rt == nullptr)
   {
      nxlog_debug_tag(SUBAGENT_DEBUG_TAG, 5, _T("H_NetRoutingTable: call to sysctl() failed (%s)"), _tcserror(errno));
      return SYSINFO_RC_ERROR;
   }

   struct rt_msghdr *rtm;
   for (char *next = rt; next < rt + reqSize; next += rtm->rtm_msglen)
   {
      rtm = (struct rt_msghdr *)next;
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

      if ((rti_info[RTAX_DST] != NULL)
#if HAVE_DECL_RTF_WASCLONED
          && !(rtm->rtm_flags & RTF_WASCLONED)
#endif
          )
      {
         char route[1024];

         if (sa2sin(rti_info[RTAX_DST])->sin_addr.s_addr == INADDR_ANY)
         {
            strcpy(route, "0.0.0.0/0 ");
         }
         else
         {
            if ((rtm->rtm_flags & RTF_HOST) || (rti_info[RTAX_NETMASK] == nullptr))
            {
               // host
               strcpy(route, inet_ntoa(sa2sin(rti_info[RTAX_DST])->sin_addr));
               strcat(route, "/32 ");
            }
            else
            {
               // net
               int mask = (rti_info[RTAX_NETMASK] != nullptr) ? ntohl(sa2sin(rti_info[RTAX_NETMASK])->sin_addr.s_addr) : 0;
               sprintf(route, "%s/%d ", inet_ntoa(sa2sin(rti_info[RTAX_DST])->sin_addr), (mask != 0) ? 33 - ffs(mask) : 0);
            }
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
         if (rtm->rtm_flags & RTF_STATIC)
            routeType = 3; // netmgmt
         else if (rtm->rtm_flags & RTF_LOCAL)
            routeType = 2; // local
         else if (rtm->rtm_flags & RTF_DYNAMIC)
            routeType = 4; // icmp
         else
            routeType = 1; // other

         char tmp[64];
         snprintf(tmp, sizeof(tmp), "%d %d %d %d", rtm->rtm_index, (rtm->rtm_flags & RTF_GATEWAY) == 0 ? 3 : 4, static_cast<int>(rtm->rtm_rmx.rmx_weight), routeType);
         strcat(route, tmp);

         value->addMBString(route);
      }
   }

   MemFree(rt);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Dump full interface info to string list
 */
static void DumpInterfaceInfo(IFLIST *pList, int nIfCount, StringList *value)
{
   char szOut[1024], macAddr[32], ipAddrText[64];

   for(int i = 0; i < nIfCount; i++)
   {
      if (pList[i].addrCount == 0)
      {
         snprintf(szOut, sizeof(szOut), "%d 0.0.0.0/0 %d(%d) %s %s",
               pList[i].index, pList[i].type, pList[i].mtu,
               BinToStrA((BYTE *)pList[i].mac, 6, macAddr),
               pList[i].name);
         value->addMBString(szOut);
      }
      else
      {
         for(int j = 0; j < pList[i].addrCount; j++)
         {
            snprintf(szOut, sizeof(szOut), "%d %s/%d %d(%d) %s %s",
                  pList[i].index, pList[i].addr[j].toStringA(ipAddrText),
                  pList[i].addr[j].getMaskBits(), pList[i].type, pList[i].mtu,
                  BinToStrA((BYTE *)pList[i].mac, 6, macAddr),
                  pList[i].name);
            value->addMBString(szOut);
         }
      }
   }
}

/**
 * Dump only interface names to string list
 */
static void DumpInterfaceNames(IFLIST *pList, int nIfCount, StringList *value)
{
   for(int i = 0; i < nIfCount; i++)
   {
      value->addMBString(pList[i].name);
   }
}

/**
 * Common handler for Net.InterfaceList and Net.InterfaceNames lists
 */
static LONG GetInterfaceList(StringList *value, bool namesOnly)
{
   struct ifaddrs *pIfAddr;
   if (getifaddrs(&pIfAddr) != 0)
   {
      nxlog_debug_tag(SUBAGENT_DEBUG_TAG, 5, _T("Call to getifaddrs() failed (%s)"), _tcserror(errno));
      return SYSINFO_RC_ERROR;
   }

   char *pName = nullptr;
   int nIfCount = 0;
   IFLIST *ifList = nullptr;

   LONG nRet = SYSINFO_RC_SUCCESS;

   struct ifaddrs *pNext = pIfAddr;
   while (pNext != nullptr)
   {
      if (pName != pNext->ifa_name)
      {
         nIfCount++;
         IFLIST *tmp = MemRealloc(ifList, nIfCount * sizeof(IFLIST));
         if (tmp == nullptr)
         {
            // out of memoty
            nIfCount--;
            nRet = SYSINFO_RC_ERROR;
            break;
         }
         ifList = tmp;

         memset(&ifList[nIfCount - 1], 0, sizeof(IFLIST));
         ifList[nIfCount - 1].name = pNext->ifa_name;
         pName = pNext->ifa_name;
      }

      switch(pNext->ifa_addr->sa_family)
      {
         case AF_INET:
         case AF_INET6:
            ifList[nIfCount - 1].addrCount++;
	    {
               InetAddress *tmp = (InetAddress *)MemRealloc(ifList[nIfCount - 1].addr, ifList[nIfCount - 1].addrCount * sizeof(InetAddress));
               if (tmp == nullptr)
               {
                  ifList[nIfCount-1].addrCount--;
                  nRet = SYSINFO_RC_ERROR;
                  break;
               }
               ifList[nIfCount - 1].addr = tmp;
            }
            if (pNext->ifa_addr->sa_family == AF_INET)
            {
               uint32_t addr = htonl(reinterpret_cast<struct sockaddr_in*>(pNext->ifa_addr)->sin_addr.s_addr);
               uint32_t mask = htonl(reinterpret_cast<struct sockaddr_in*>(pNext->ifa_netmask)->sin_addr.s_addr);
               ifList[nIfCount - 1].addr[ifList[nIfCount - 1].addrCount - 1] = InetAddress(addr, mask);
            }
            else
            {
               ifList[nIfCount - 1].addr[ifList[nIfCount - 1].addrCount - 1] =
                     InetAddress(reinterpret_cast<struct sockaddr_in6*>(pNext->ifa_addr)->sin6_addr.s6_addr,
                           BitsInMask(reinterpret_cast<struct sockaddr_in6*>(pNext->ifa_netmask)->sin6_addr.s6_addr, 16));
            }
            break;
         case AF_LINK:
            struct sockaddr_dl *pSdl = (struct sockaddr_dl *)pNext->ifa_addr;
            ifList[nIfCount - 1].mac = (struct ether_addr *)LLADDR(pSdl);
            ifList[nIfCount - 1].index = pSdl->sdl_index;
            ifList[nIfCount - 1].type = static_cast<struct if_data*>(pNext->ifa_data)->ifi_type;
            ifList[nIfCount - 1].mtu = static_cast<struct if_data*>(pNext->ifa_data)->ifi_mtu;
            break;
      }

      if (nRet == SYSINFO_RC_ERROR)
         break;

      pNext = pNext->ifa_next;
   }

   if (nRet == SYSINFO_RC_SUCCESS)
   {
      if (namesOnly)
         DumpInterfaceNames(ifList, nIfCount, value);
      else
         DumpInterfaceInfo(ifList, nIfCount, value);
   }

   for (int i = 0; i < nIfCount; i++)
      MemFree(ifList[i].addr);
   MemFree(ifList);
   freeifaddrs(pIfAddr);

   return nRet;
}

/**
 * Handler for Net.InterfaceList list
 */
LONG H_NetIfList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   return GetInterfaceList(value, false);
}

/**
 * Handler for Net.InterfaceNames list
 */
LONG H_NetIfNames(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   return GetInterfaceList(value, true);
}

#if __FreeBSD__ >= 13

/**
 * Handler for interface statistics parameters (retrieved via sysctl)
 */
LONG H_NetIfInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char ifName[256];
   if (!AgentGetParameterArgA(param, 1, ifName, sizeof(ifName)))
      return SYSINFO_RC_UNSUPPORTED;

   if (ifName[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   int ifIndex;
   if ((ifName[0] >= '0') && (ifName[0] <= '9'))
   {
      ifIndex = atoi(ifName);
   }
   else
   {
      ifIndex = if_nametoindex(ifName);
      if (ifIndex == 0)
      {
         nxlog_debug(7, _T("H_NetIfInfo: cannot find interface index for name %hs"), ifName);
         return SYSINFO_RC_UNSUPPORTED;
      }
   }

   struct ifmibdata ifData;
   size_t len = sizeof(ifData);
   int name[] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, ifIndex, IFDATA_GENERAL };
   if (sysctl(name, 6, &ifData, &len, nullptr, 0) != 0)
   {
      nxlog_debug(7, _T("H_NetIfInfo: sysctl error for interface %hs index %d"), ifName, ifIndex);
      return SYSINFO_RC_UNSUPPORTED;
   }

   switch(CAST_FROM_POINTER(arg, int))
   {
      case IF_INFO_BYTES_IN:
         ret_uint(value, static_cast<uint32_t>(ifData.ifmd_data.ifi_ibytes));
         break;
      case IF_INFO_BYTES_IN_64:
         ret_uint64(value, ifData.ifmd_data.ifi_ibytes);
         break;
      case IF_INFO_BYTES_OUT:
         ret_uint(value, static_cast<uint32_t>(ifData.ifmd_data.ifi_obytes));
         break;
      case IF_INFO_BYTES_OUT_64:
         ret_uint64(value, ifData.ifmd_data.ifi_obytes);
         break;
      case IF_INFO_IN_ERRORS:
         ret_uint(value, static_cast<uint32_t>(ifData.ifmd_data.ifi_ierrors));
         break;
      case IF_INFO_IN_ERRORS_64:
         ret_uint64(value, ifData.ifmd_data.ifi_ierrors);
         break;
      case IF_INFO_OUT_ERRORS:
         ret_uint(value, static_cast<uint32_t>(ifData.ifmd_data.ifi_oerrors));
         break;
      case IF_INFO_OUT_ERRORS_64:
         ret_uint64(value, ifData.ifmd_data.ifi_oerrors);
         break;
      case IF_INFO_PACKETS_IN:
         ret_uint(value, static_cast<uint32_t>(ifData.ifmd_data.ifi_ipackets));
         break;
      case IF_INFO_PACKETS_IN_64:
         ret_uint64(value, ifData.ifmd_data.ifi_ipackets);
         break;
      case IF_INFO_PACKETS_OUT:
         ret_uint(value, static_cast<uint32_t>(ifData.ifmd_data.ifi_opackets));
         break;
      case IF_INFO_PACKETS_OUT_64:
         ret_uint64(value, ifData.ifmd_data.ifi_opackets);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}

#else /* not __FreeBSD__ >= 13 */

/**
 * KVM name list
 */
struct nlist s_nl[] = 
{
   { (char *)"_ifnet" },
   { nullptr }
};

/**
 * KVM handle
 */
static kvm_t *s_kvmd = nullptr;

/**
 * KVM lock
 */
static Mutex s_kvmLock;

#if __FreeBSD__ >= 10

/**
 * Read kernel counter
 */
inline uint64_t ReadKernelCounter64(counter_u64_t cnt)
{
   uint64_t value;
   if (kvm_read(s_kvmd, (u_long)cnt, &value, sizeof(uint64_t)) != sizeof(uint64_t))
   {
      nxlog_debug(7, _T("ReadKernelCounter64: kvm_read failed (%hs) at address %p"), kvm_geterr(s_kvmd), cnt);
      return 0;
   }
   return value;
}

#endif

/**
 * Handler for interface statistics parameters (retrieved via KVM)
 */
LONG H_NetIfInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	char ifName[256];
	if (!AgentGetParameterArgA(param, 1, ifName, sizeof(ifName)))
		return SYSINFO_RC_UNSUPPORTED;

	if (ifName[0] == 0)		
		return SYSINFO_RC_UNSUPPORTED;

	if ((ifName[0] >= '0') && (ifName[0] <= '9'))
	{
		int ifIndex = atoi(ifName);
		if (if_indextoname(ifIndex, ifName) != ifName)
		{
			nxlog_debug(7, _T("H_NetIfInfo: cannot find interface name for index %d"), ifIndex);
			return SYSINFO_RC_UNSUPPORTED;
		}
	}

	s_kvmLock.lock();
	if (s_kvmd == NULL)
	{
		char errmsg[_POSIX2_LINE_MAX];
      s_kvmd = kvm_openfiles(NULL, "/dev/null", NULL, O_RDONLY, errmsg);
      if (s_kvmd == NULL)
		{
			nxlog_debug(7, _T("H_NetIfInfo: kvm_openfiles failed (%hs)"), errmsg);
			s_kvmLock.unlock();
			return SYSINFO_RC_ERROR;
		}
		if (kvm_nlist(s_kvmd, s_nl) < 0)
		{
			nxlog_debug(7, _T("H_NetIfInfo: kvm_nlist failed (%hs)"), kvm_geterr(s_kvmd));
			kvm_close(s_kvmd);
			s_kvmd = NULL;
			s_kvmLock.unlock();
			return SYSINFO_RC_UNSUPPORTED;
		}
		if (s_nl[0].n_type == 0)
		{
			nxlog_debug(7, _T("H_NetIfInfo: symbol %hs not found in kernel symbol table"), s_nl[0].n_name);
			kvm_close(s_kvmd);
			s_kvmd = NULL;
			s_kvmLock.unlock();
			return SYSINFO_RC_UNSUPPORTED;
		}
	}

	int rc = SYSINFO_RC_UNSUPPORTED;

	u_long curr = s_nl[0].n_value;
	struct ifnethead head;
	if (kvm_read(s_kvmd, curr, &head, sizeof(head)) != sizeof(head))
	{
		nxlog_debug(7, _T("H_NetIfInfo: kvm_read failed (%hs)"), kvm_geterr(s_kvmd));
		s_kvmLock.unlock();
		return SYSINFO_RC_ERROR;
	}
#if __FreeBSD__ >= 12
	curr = (u_long)STAILQ_FIRST(&head);
#else
	curr = (u_long)TAILQ_FIRST(&head);
#endif
	while(curr != 0)
	{
		struct ifnet ifnet;
		if (kvm_read(s_kvmd, curr, &ifnet, sizeof(ifnet)) != sizeof(ifnet))
		{
			nxlog_debug(7, _T("H_NetIfInfo: kvm_read failed (%hs)"), kvm_geterr(s_kvmd));
			rc = SYSINFO_RC_ERROR;
			break;
		}
#if __FreeBSD__ >= 12
		curr = (u_long)STAILQ_NEXT(&ifnet, if_link);
#else
		curr = (u_long)TAILQ_NEXT(&ifnet, if_link);
#endif

#if __FreeBSD__ >= 5
		const char *currName = ifnet.if_xname;
#else
		char currName[IFNAMSIZ];
		if (kvm_read(s_kvmd, ifnet.if_name, currName, sizeof(currName)) != sizeof(currName))
		{
			nxlog_debug(7, _T("H_NetIfInfo: kvm_read failed (%hs)"), kvm_geterr(s_kvmd));
			rc = SYSINFO_RC_ERROR;
			break;
		}
		currName[sizeof(currName) - 2] = 0;
		size_t len = strlen(currName); 
		snprintf(&currName[len], sizeof(currName) - len, "%d", ifnet.if_unit);
#endif
		if (!strcmp(currName, ifName))
		{
			rc = SYSINFO_RC_SUCCESS;
#if __FreeBSD__ >= 11
			switch(CAST_FROM_POINTER(arg, int))
			{
				case IF_INFO_BYTES_IN:
					ret_uint(value, (UINT32)(ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_IBYTES]) & _ULL(0xFFFFFFFF)));
					break;
				case IF_INFO_BYTES_IN_64:
					ret_uint64(value, ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_IBYTES]));
					break;
				case IF_INFO_BYTES_OUT:
					ret_uint(value, (UINT32)(ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_OBYTES]) & _ULL(0xFFFFFFFF)));
					break;
				case IF_INFO_BYTES_OUT_64:
					ret_uint64(value, ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_OBYTES]));
					break;
				case IF_INFO_IN_ERRORS:
					ret_uint(value, (UINT32)(ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_IERRORS]) & _ULL(0xFFFFFFFF)));
					break;
				case IF_INFO_IN_ERRORS_64:
					ret_uint64(value, ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_IERRORS]));
					break;
				case IF_INFO_OUT_ERRORS:
					ret_uint(value, (UINT32)(ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_OERRORS]) & _ULL(0xFFFFFFFF)));
					break;
				case IF_INFO_OUT_ERRORS_64:
					ret_uint64(value, ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_OERRORS]));
					break;
				case IF_INFO_PACKETS_IN:
					ret_uint(value, (UINT32)(ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_IPACKETS]) & _ULL(0xFFFFFFFF)));
					break;
				case IF_INFO_PACKETS_IN_64:
					ret_uint64(value, ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_IPACKETS]));
					break;
				case IF_INFO_PACKETS_OUT:
					ret_uint(value, (UINT32)(ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_OPACKETS]) & _ULL(0xFFFFFFFF)));
					break;
				case IF_INFO_PACKETS_OUT_64:
					ret_uint64(value, ReadKernelCounter64(ifnet.if_counters[IFCOUNTER_OPACKETS]));
					break;
				default:
					rc = SYSINFO_RC_UNSUPPORTED;
					break;
			}
#else
			switch(CAST_FROM_POINTER(arg, int))
			{
				case IF_INFO_BYTES_IN:
					ret_uint(value, ifnet.if_ibytes);
					break;
				case IF_INFO_BYTES_OUT:
					ret_uint(value, ifnet.if_obytes);
					break;
				case IF_INFO_IN_ERRORS:
					ret_uint(value, ifnet.if_ierrors);
					break;
				case IF_INFO_OUT_ERRORS:
					ret_uint(value, ifnet.if_oerrors);
					break;
				case IF_INFO_PACKETS_IN:
					ret_uint(value, ifnet.if_ipackets);
					break;
				case IF_INFO_PACKETS_OUT:
					ret_uint(value, ifnet.if_opackets);
					break;
				default:
					rc = SYSINFO_RC_UNSUPPORTED;
					break;
			}
#endif
			break;
		}
	}
	s_kvmLock.unlock();
	return rc;
}

#endif /* __FreeBSD__ >= 13 */

/**
 * Handler for Net.Interface.64BitCounters
 */
LONG H_NetInterface64bitSupport(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
#if __FreeBSD__ >= 10
	ret_int(value, 1);
#else
	ret_int(value, 0);
#endif	
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.Interface.Speed(*) parameter
 */
LONG H_NetIfInfoSpeed(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   static ifmedia_baudrate baudrateDescriptions[] = IFM_BAUDRATE_DESCRIPTIONS;
   LONG ret = SYSINFO_RC_ERROR;
   char buffer[256];

   if (!AgentGetParameterArgA(param, 1, buffer, 256))
      return SYSINFO_RC_UNSUPPORTED;

   // Check if we have interface name or index
   char *p, name[IFNAMSIZ];
   uint index = strtol(buffer, &p, 10);
   if (*p == 0)
   {
      // Index passed as argument, convert to name
      if (if_indextoname(index, name) == nullptr)
         return SYSINFO_RC_ERROR;
   }
   else
   {
      // Name passed as argument
      strlcpy(name, buffer, IFNAMSIZ);
   }

   int sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock == -1)
      return SYSINFO_RC_ERROR;

   struct ifmediareq ifmr;
   memset(&ifmr, 0, sizeof(ifmr));
   strlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));
   if (ioctl(sock, SIOCGIFMEDIA, (caddr_t)&ifmr) >= 0)
   {
      ifmedia_baudrate* bdr = baudrateDescriptions;
      while (bdr->ifmb_word != 0)
      {
         if (bdr->ifmb_word == (IFM_TYPE(ifmr.ifm_active) | IFM_SUBTYPE(ifmr.ifm_active)))
         {
            ret_uint64(value, bdr->ifmb_baudrate);
            ret = SYSINFO_RC_SUCCESS;
            break;
         }
         bdr++;
      }
   }
   close(sock);

   return ret;
}
