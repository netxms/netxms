/* $Id: net.cpp,v 1.3 2006-08-16 22:26:09 victor Exp $ */

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

#include "ipso.h"
#include <sys/file.h>
#include <sys/sysctl.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>


//
// Internal data structures
//

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


//
// Handler for Net.IP.Forwarding parameter
//

LONG H_NetIpForwarding(char *pszParam, char *pArg, char *pValue)
{
	int nVer = (int)pArg;
	int nRet = SYSINFO_RC_ERROR;
	LONG nVal;

	if (nVer == 6)
	{
		return ERR_NOT_IMPLEMENTED;
	}

	nRet = IPSCTLGetInt("net:ip:forward:noforwarding", &nVal);
	if (nRet == SYSINFO_RC_SUCCESS)
		ret_int(pValue, !nVal);

	return nRet;
}


//
// Handler for Net.Interface.XXX parameters
//

LONG H_NetIfStats(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szName[128], szRequest[256];
	static char *pszRqList[] =
	{
		"interface:%s:name",
		"interface:%s:flags:link_avail",
		"interface:%s:flags:up",
		"interface:%s:stats:ibytes",
		"interface:%s:stats:obytes",
		"interface:%s:stats:ipackets",
		"interface:%s:stats:opackets"
	};

	if (!NxGetParameterArg(pszParam, 1, szName, sizeof(szName)))
		return SYSINFO_RC_UNSUPPORTED;

	if (szName[0] != 0)
	{
		// Convert interface index to name if needed
		if (szName[0] >= '0' && szName[0] <= '9')
		{
			// index
			if (if_indextoname(atoi(szName), szName) != szName)
			{
				// not found
				nRet = SYSINFO_RC_ERROR;
			}
		}

		if (nRet == SYSINFO_RC_SUCCESS)
		{
			snprintf(szRequest, sizeof(szRequest), pszRqList[(int)pArg], szName);
			nRet = IPSCTLGetString(szRequest, pValue, MAX_RESULT_LENGTH);
		}
	}
	else
	{
		nRet = SYSINFO_RC_UNSUPPORTED;
	}

	return nRet;
}


//
// Handler for Net.ArpCache enum
//

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

LONG H_NetRoutingTable(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
#define sa2sin(x) ((struct sockaddr_in *)x)
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
	int nRet = SYSINFO_RC_ERROR;
	int mib[6] = { CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_DUMP, 0 };
	char *pRT = NULL, *pNext = NULL;
	size_t nReqSize = 0;
	struct rt_msghdr *rtm;
	struct sockaddr *sa;
	struct sockaddr *rti_info[RTAX_MAX];

	if (sysctl(mib, 6, NULL, &nReqSize, NULL, 0) == 0)
	{
		if (nReqSize > 0)
		{
			pRT = (char *)malloc(nReqSize);
			if (pRT != NULL)
			{
				if (sysctl(mib, 6, pRT, &nReqSize, NULL, 0) < 0)
				{
					free(pRT);
					pRT = NULL;
				}
			}
		}
	}

	if (pRT != NULL)
	{
		nRet = SYSINFO_RC_SUCCESS;

		for (pNext = pRT; pNext < pRT + nReqSize; pNext += rtm->rtm_msglen)
		{
			rtm = (struct rt_msghdr *)pNext;
			sa = (struct sockaddr *)(rtm + 1);

			if (sa->sa_family != AF_INET)
			{
				continue;
			}

			for (int i = 0; i < RTAX_MAX; i++)
			{
				if (rtm->rtm_addrs & (1 << i))
				{
					rti_info[i] = sa;
					sa = (struct sockaddr *)((char *)(sa) + ROUNDUP(sa->sa_len));
				}
				else
				{
					rti_info[i] = NULL;
				}
			}

			if (rti_info[RTAX_DST] != NULL && !(rtm->rtm_flags & RTF_WASCLONED))
			{
				char szOut[1024];
				char szTmp[64];

				if (sa2sin(rti_info[RTAX_DST])->sin_addr.s_addr == INADDR_ANY)
				{
					strcpy(szOut, "0.0.0.0/0 ");
				}
				else
				{
					if ((rtm->rtm_flags & RTF_HOST) || rti_info[RTAX_NETMASK]==NULL)
					{
						// host
						strcpy(szOut,
								inet_ntoa(sa2sin(rti_info[RTAX_DST])->sin_addr));
						strcat(szOut, "/32 ");
					}
					else
					{
						// net
						int nMask =
							rti_info[RTAX_NETMASK] ?
								ntohl(sa2sin(rti_info[RTAX_NETMASK])->sin_addr.s_addr)
								: 0;

						sprintf(szOut, "%s/%d ",
								inet_ntoa(sa2sin(rti_info[RTAX_DST])->sin_addr),
								nMask ? 33 - ffs(nMask) : 0);
					}
				}

				if (rti_info[RTAX_GATEWAY]->sa_family != AF_INET)
				{
					strcat(szOut, "0.0.0.0 ");
				}
				else
				{
					strcat(szOut,
							inet_ntoa(sa2sin(rti_info[RTAX_GATEWAY])->sin_addr));
					strcat(szOut, " ");
				}
				snprintf(szTmp, sizeof(szTmp), "%d %d",
						rtm->rtm_index,
						(rtm->rtm_flags & RTF_GATEWAY) == 0 ? 3 : 4);
				strcat(szOut, szTmp);

				NxAddResultString(pValue, szOut);
			}
		}

		free(pRT);
	}

#undef ROUNDUP
#undef sa2sin

	return nRet;
}


//
// Handler for Net.InterfaceList enum
//

LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2006/07/21 16:22:44  victor
Some parameters are working

Revision 1.1  2006/07/21 11:48:35  victor
Initial commit

*/
