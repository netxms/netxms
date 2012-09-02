/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2008 Mark Ibell
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
#include <net/iso88025.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <kvm.h>
#include <nlist.h>

#include "net.h"

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

struct nlist nl[] = {
#define N_IFNET 0
	{ (char *)"_ifnet" },
	{ NULL },
};

kvm_t *kvmd = NULL;

LONG H_NetIpForwarding(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nVer = CAST_FROM_POINTER(pArg, int);
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

LONG H_NetIfAdmStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szArg[512];

	AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));

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

LONG H_NetIfLink(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szArg[512];

	AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));

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
            else if (errno == EINVAL || errno == ENOTTY)
            {
               // ifmedia not supported, assume the status is NORMAL
               ret_int(pValue, 1);
               nRet = SYSINFO_RC_SUCCESS;
            }
				close(nSocket);
			}
		}
	}

	return nRet;
}

LONG H_NetArpCache(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
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

#ifdef UNICODE
		pValue->addPreallocated(WideStringFromMBString(szBuff));
#else
		pValue->add(szBuff);
#endif
	}

	return nRet;
}

LONG H_NetRoutingTable(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
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

			if (rti_info[RTAX_DST] != NULL
#if HAVE_DECL_RTF_WASCLONED
			    && !(rtm->rtm_flags & RTF_WASCLONED))
#else
                            )
#endif
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

#ifdef UNICODE
				pValue->addPreallocated(WideStringFromMBString(szOut));
#else
				pValue->add(szOut);
#endif
			}
		}

		free(pRT);
	}

#undef ROUNDUP
#undef sa2sin

	return nRet;
}

LONG H_NetIfList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
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
			int j;
			char szOut[1024], macAddr[32];

			for (i = 0; i < nIfCount; i++)
			{
				if (pList[i].addrCount == 0)
				{
					snprintf(szOut, sizeof(szOut), "%d 0.0.0.0/0 %d %s %s",
							pList[i].index,
							IFTYPE_OTHER,
							BinToStrA((BYTE *)pList[i].mac, 6, macAddr),
							pList[i].name);
#ifdef UNICODE
					pValue->addPreallocated(WideStringFromMBString(szOut));
#else
					pValue->add(szOut);
#endif
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
							                BinToStrA((BYTE *)pList[i].mac, 6, macAddr),
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
							                BinToStrA((BYTE *)pList[i].mac, 6, macAddr),
									pList[i].name);
						}
#ifdef UNICODE
						pValue->addPreallocated(WideStringFromMBString(szOut));
#else
						pValue->add(szOut);
#endif
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

LONG H_NetIfInfoFromKVM(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szArg[512];
	u_long ifnetaddr;
	struct ifnet ifnet;
	struct ifnethead ifnethead;
#if __FreeBSD__ < 5
	char szTName[IFNAMSIZ];
#endif
	char szName[IFNAMSIZ];

	AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));

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
			nRet = SYSINFO_RC_ERROR;

			if (kvmd == NULL) {
				kvmd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
				if (kvmd == NULL)
					return SYSINFO_RC_ERROR;
				if (kvm_nlist(kvmd, nl) < 0)
					return SYSINFO_RC_ERROR;
				if (nl[0].n_type == 0)
					return SYSINFO_RC_ERROR;
			}
			ifnetaddr = nl[N_IFNET].n_value;
			if (kvm_read(kvmd, ifnetaddr, &ifnethead, sizeof(ifnethead)) != sizeof(ifnethead))
				return SYSINFO_RC_ERROR;
			ifnetaddr = (u_long)TAILQ_FIRST(&ifnethead);
			while (ifnetaddr) {
				if (kvm_read(kvmd, ifnetaddr, &ifnet, sizeof(ifnet)) != sizeof(ifnet))
					return SYSINFO_RC_ERROR;
				ifnetaddr = (u_long)TAILQ_NEXT(&ifnet, if_link);
#if __FreeBSD__ >= 5
				strlcpy(szName, ifnet.if_xname, sizeof(szName));
#else
				if (kvm_read(kvmd, ifnet.if_name, szTName, sizeof(szTName)) != sizeof(szTName))
					return SYSINFO_RC_ERROR;
				szTName[sizeof(szTName) - 1] = '\0';
				snprintf(szName, sizeof(szName), "%s%d", szTName, ifnet.if_unit);
#endif
				if (strcmp(szName, szArg) == 0) {
					nRet = SYSINFO_RC_SUCCESS;
					switch((long)pArg)
					{
						case IF_INFO_BYTES_IN:
							ret_uint(pValue, ifnet.if_ibytes);
							break;
						case IF_INFO_BYTES_OUT:
							ret_uint(pValue, ifnet.if_obytes);
							break;
						case IF_INFO_IN_ERRORS:
							ret_uint(pValue, ifnet.if_ierrors);
							break;
						case IF_INFO_OUT_ERRORS:
							ret_uint(pValue, ifnet.if_oerrors);
							break;
						case IF_INFO_PACKETS_IN:
							ret_uint(pValue, ifnet.if_ipackets);
							break;
						case IF_INFO_PACKETS_OUT:
							ret_uint(pValue, ifnet.if_opackets);
							break;
						default:
							nRet = SYSINFO_RC_UNSUPPORTED;
							break;
					}
					break;
				}
				else
					continue;
			}
		}
	}

	return nRet;
}
