/* $Id: net.cpp,v 1.3 2005-02-14 17:03:37 alk Exp $ */

/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
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
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/iso88025.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/ethernet.h>


LONG H_NetIpForwarding(char *pszParam, char *pArg, char *pValue)
{
	int nVer = (int)pArg;
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
			ret_int(pValue, nVal);
			nRet = SYSINFO_RC_SUCCESS;
		}
	}

	return nRet;
}

LONG H_NetArpCache(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
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

		NxAddResultString(pValue, szBuff);
	}

	return nRet;
}

LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct ifaddrs *pIfAddr, *pNext;

	if (getifaddrs(&pIfAddr) == 0)
	{
		char *pIp, *pMac, *pName = NULL;
		int nIndex, nMask, i;
		unsigned long nDiv = 0x80000000;

		nRet = SYSINFO_RC_SUCCESS;

		pNext = pIfAddr;
		while (pNext != NULL)
		{
			if (pName != pNext->ifa_name)
			{
				if (pName != NULL)
				{
					char szOut[1024];

					snprintf(szOut, sizeof(szOut), "%d %s/%d %d %s %s",
							nIndex,
							pIp,
							nMask,
							IFTYPE_OTHER,
							pMac,
							pName);
					NxAddResultString(pValue, szOut);
				}
				pName = pNext->ifa_name;
				pIp = "0.0.0.0";
				nMask = 0;
				pMac = "00:00:00:00:00:00";
				nIndex = 0;
				nDiv = 0x80000000;
			}

			switch (pNext->ifa_addr->sa_family)
			{
			case AF_INET:
				pIp = inet_ntoa(((struct sockaddr_in *)(pNext->ifa_addr))->sin_addr);
				nMask = BitsInMask(htonl(
							((struct sockaddr_in *)
								(pNext->ifa_netmask))->sin_addr.s_addr
							)
						);
				break;
			case AF_LINK:
				{
					struct sockaddr_dl *pSdl;

					pSdl = (struct sockaddr_dl *)pNext->ifa_addr;
					pMac = ether_ntoa((struct ether_addr *)LLADDR(pSdl));
					nIndex = pSdl->sdl_index;
				}
				break;
			}
			pNext = pNext->ifa_next;
		}
		
		freeifaddrs(pIfAddr);
	}
	else
	{
		perror("getifaddrs()");
	}
	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2005/01/23 05:08:06  alk
+ System.CPU.Count
+ System.Memory.Physical.*
+ System.ProcessCount
+ System.ProcessList

Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)

Revision 1.4  2005/01/05 12:21:24  victor
- Added wrappers for new and delete from gcc2 libraries
- sys/stat.h and fcntl.h included in nms_common.h

Revision 1.3  2004/11/25 08:01:27  victor
Processing of interface list will be stopped on error

Revision 1.2  2004/10/23 22:53:23  alk
ArpCache: ignore incomplete entries

Revision 1.1  2004/10/22 22:08:34  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
