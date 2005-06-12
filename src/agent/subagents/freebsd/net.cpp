/* $Id: net.cpp,v 1.8 2005-06-12 17:58:36 victor Exp $ */

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
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/iso88025.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/ethernet.h>

typedef struct t_Addr
{
	struct in_addr ip;
	int mask;
} ADDR;

typedef struct t_IfList
{
	char *name;
	struct ether_addr *mac;
	ADDR *addr;
	int addrCount;
	int index;
} IFLIST;

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

LONG H_NetIfAdmStatus(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szArg[512];

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

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
						ret_int(pValue, 1);
						nRet = SYSINFO_RC_SUCCESS;
					}
					else
					{
						ret_int(pValue, 2);
						nRet = SYSINFO_RC_SUCCESS;
					}
				}
				close(nSocket);
			}
		}
	}

	return nRet;
}

LONG H_NetIfLink(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szArg[512];

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

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
				strncpy(ifmr.ifm_name, szArg, sizeof(ifmr.ifm_name));
				if (ioctl(nSocket, SIOCGIFMEDIA, (caddr_t)&ifmr) >= 0)
				{
					if ((ifmr.ifm_status & IFM_AVALID) == IFM_AVALID &&
							(ifmr.ifm_status & IFM_ACTIVE) == IFM_ACTIVE)
					{
						ret_int(pValue, 1);
						nRet = SYSINFO_RC_SUCCESS;
					}
					else
					{
						ret_int(pValue, 0);
						nRet = SYSINFO_RC_SUCCESS;
					}
				}
				close(nSocket);
			}
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
		char *pName = NULL;
		int nIndex, nMask, i;
		int nIfCount = 0;
		IFLIST *pList = NULL;

		nRet = SYSINFO_RC_SUCCESS;

		pNext = pIfAddr;
		while (pNext != NULL)
		{
			if (pName != pNext->ifa_name)
			{
				IFLIST *pTmp;

				nIfCount++;
				pTmp = (IFLIST *)realloc(pList, nIfCount * sizeof(IFLIST));
				if (pTmp == NULL)
				{
					// out of memoty
					nIfCount--;
					nRet = SYSINFO_RC_ERROR;
					break;
				}
				pList = pTmp;

				memset(&(pList[nIfCount-1]), 0, sizeof(IFLIST));
				pList[nIfCount-1].name = pNext->ifa_name;
				pName = pNext->ifa_name;
			}

			switch (pNext->ifa_addr->sa_family)
			{
			case AF_INET:
				{
					ADDR *pTmp;
					pList[nIfCount-1].addrCount++;
					pTmp = (ADDR *)realloc(pList[nIfCount-1].addr,
							pList[nIfCount-1].addrCount * sizeof(ADDR));
					if (pTmp == NULL)
					{
						pList[nIfCount-1].addrCount--;
						nRet = SYSINFO_RC_ERROR;
						break;
					}
					pList[nIfCount-1].addr = pTmp;
					pList[nIfCount-1].addr[pList[nIfCount-1].addrCount-1].ip =
						((struct sockaddr_in *)(pNext->ifa_addr))->sin_addr;
					pList[nIfCount-1].addr[pList[nIfCount-1].addrCount-1].mask = 
						BitsInMask(htonl(((struct sockaddr_in *)
										(pNext->ifa_netmask))->sin_addr.s_addr));
				}
				break;
			case AF_LINK:
				{
					struct sockaddr_dl *pSdl;

					pSdl = (struct sockaddr_dl *)pNext->ifa_addr;
					pList[nIfCount-1].mac = (struct ether_addr *)LLADDR(pSdl);
					pList[nIfCount-1].index = pSdl->sdl_index;
				}
				break;
			}

			if (nRet == SYSINFO_RC_ERROR)
			{
				break;
			}
			pNext = pNext->ifa_next;
		}

		if (nRet == SYSINFO_RC_SUCCESS)
		{
			for (i = 0; i < nIfCount; i++)
			{
				int j;
				char szOut[1024];

				if (pList[i].addrCount == 0)
				{
					snprintf(szOut, sizeof(szOut), "%d 0.0.0.0/0 %d %s %s",
							pList[i].index,
							IFTYPE_OTHER,
							ether_ntoa(pList[i].mac),
							pList[i].name);
					NxAddResultString(pValue, szOut);
				}
				else
				{
					for (j = 0; j < pList[i].addrCount; j++)
					{
						if (j > 0)
						{
							snprintf(szOut, sizeof(szOut), "%d %s/%d %d %s %s:%d",
									pList[i].index,
									inet_ntoa(pList[i].addr[j].ip),
									pList[i].addr[j].mask,
									IFTYPE_OTHER,
									ether_ntoa(pList[i].mac),
									pList[i].name,
									j - 1);
						}
						else
						{
							snprintf(szOut, sizeof(szOut), "%d %s/%d %d %s %s",
									pList[i].index,
									inet_ntoa(pList[i].addr[j].ip),
									pList[i].addr[j].mask,
									IFTYPE_OTHER,
									ether_ntoa(pList[i].mac),
									pList[i].name);
						}
						NxAddResultString(pValue, szOut);
					}
				}
			}
		}

		for (i = 0; i < nIfCount; i++)
		{
			if (pList[i].addr != NULL)
			{
				free(pList[i].addr);
				pList[i].addr = NULL;
				pList[i].addrCount = 0;
			}
		}
		if (pList != NULL)
		{
			free(pList);
			pList = NULL;
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
Revision 1.7  2005/05/30 16:31:58  alk
fix: InterfaceList now return interfaces w/o IP address

Revision 1.6  2005/05/23 20:30:28  alk
! memory allocation for address list now in sizeof * count, fixes "mixing" lists of aliases

Revision 1.5  2005/03/10 19:04:07  alk
implemented:
	Net.Interface.AdminStatus(*)
	Net.Interface.Link(*)

Revision 1.4  2005/03/10 12:23:56  alk
issue #18
alias handling on inet interfaces
status: fixed

Revision 1.3  2005/02/14 17:03:37  alk
issue #9

mask calculation chaged to BitsInMask()

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
