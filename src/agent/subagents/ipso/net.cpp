/* 
** NetXMS subagent for IPSO
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2007-2010 Victor Kirhenshtein
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

#define MAX_IF_NAME	128

typedef struct t_IfData
{
	char szName[MAX_IF_NAME];
	LONG nIndex;
	LONG nType;
	char szMacAddr[24];
	BOOL isLogical;
	char szPhysIfName[MAX_IF_NAME];
	int nAddrCount;
	char szIpAddr[16];
	int nMask;
} IFDATA;

typedef struct t_IfList
{
	int nSize;
	IFDATA *pData;
	int nHandle;
	int nCurr;
} IFLIST;

typedef struct t_cbp
{
	int nHandle;
	StringList *pList;
} CBP;


//
// Index to name resolution callback parameters
//

typedef struct
{
	int nHandle;
	int nIndex;
	char *pszName;
	char *pszResult;
} IRPARAM;


//
// Callback for index to name resolution
//

static void IndexCallback(char *pszPath, IRPARAM *pArg)
{
	char *ptr, szCtl[256];
	LONG nIndex;

	snprintf(szCtl, 256, "%s:index", pszPath);
	if (IPSCTLGetInt(pArg->nHandle, szCtl, &nIndex) == SYSINFO_RC_SUCCESS)
	{
		if (nIndex == pArg->nIndex)
		{
			ptr = strrchr(pszPath, ':');
			if (ptr != NULL)
				ptr++;
			else
				ptr = pszPath;
			nx_strncpy(pArg->pszName, ptr, MAX_IF_NAME);
			pArg->pszResult = pArg->pszName;
		}
	}
}


//
// Convert logical interface index to name
//

static char *IndexToName(int nIndex, char *pszName, BOOL *pbIsLogical)
{
	IRPARAM data;

	if (nIndex >= 0x01000000)
	{
		*pbIsLogical = TRUE;
		return if_indextoname(nIndex - 0x01000000, pszName);
	}

	*pbIsLogical = FALSE;
	
	data.pszResult = NULL;
	data.nHandle = ipsctl_open();
	if (data.nHandle > 0)
	{
		data.nIndex = nIndex;
		data.pszName = pszName;
		ipsctl_iterate(data.nHandle, "ifphys", IndexCallback, &data);
		ipsctl_close(data.nHandle);
	}
	return data.pszResult;
}


//
// Handler for Net.IP.Forwarding parameter
//

LONG H_NetIpForwarding(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
{
	int nVer = (int)pArg;
	int nRet = SYSINFO_RC_ERROR;
	LONG nVal;

	if (nVer == 6)
	{
		return ERR_NOT_IMPLEMENTED;
	}

	nRet = IPSCTLGetInt(0, "net:ip:forward:noforwarding", &nVal);
	if (nRet == SYSINFO_RC_SUCCESS)
		ret_int(pValue, !nVal);

	return nRet;
}


//
// Handler for Net.Interface.XXX parameters
//

LONG H_NetIfStats(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
{
	LONG nTemp, nRet = SYSINFO_RC_SUCCESS;
	char szName[MAX_IF_NAME], szRequest[256];
	BOOL isLogical;
	static char *pszRqListLog[] =
	{
		"interface:%s:name",
		"interface:%s:flags:link_avail",
		"interface:%s:flags:up",
		"interface:%s:stats:ibytes",
		"interface:%s:stats:obytes",
		"interface:%s:stats:ipackets",
		"interface:%s:stats:opackets"
	};
	static char *pszRqListPhy[] =
	{
		"ifphys:%s:dev_type",
		"ifphys:%s:flags:link",
		"ifphys:%s:flags:up",
		"ifphys:%s:stats:ibytes",
		"ifphys:%s:stats:obytes",
		"ifphys:%s:stats:ipackets",
		"ifphys:%s:stats:opackets"
	};

	if (!AgentGetParameterArg(pszParam, 1, szName, sizeof(szName)))
		return SYSINFO_RC_UNSUPPORTED;

	if (szName[0] != 0)
	{
		// Convert interface index to name if needed
		if (szName[0] >= '0' && szName[0] <= '9')
		{
			// index
			if (IndexToName(atoi(szName), szName, &isLogical) != szName)
			{
				// not found
				nRet = SYSINFO_RC_ERROR;
			}
		}
		else
		{
			// Check if interface is physical or logical
			snprintf(szRequest, sizeof(szRequest), "ifphys:%s:index", szName);
			isLogical = (IPSCTLGetInt(0, szRequest, &nTemp) == SYSINFO_RC_SUCCESS) ? FALSE : TRUE;
		}

		if (nRet == SYSINFO_RC_SUCCESS)
		{
			snprintf(szRequest, sizeof(szRequest),
			         isLogical ? pszRqListLog[(int)pArg] : pszRqListPhy[(int)pArg],
			         szName);
			nRet = IPSCTLGetString(0, szRequest, pValue, MAX_RESULT_LENGTH);
		}
	}
	else
	{
		nRet = SYSINFO_RC_UNSUPPORTED;
	}

	return nRet;
}


//
// Callback for ARP cache enumeration
//

static void ArpCallback(char *pszPath, CBP *pArg)
{
	char *ptr, szBuffer[1024], szMacAddr[32], szIfName[MAX_IF_NAME];

	snprintf(szBuffer, 1024, "%s:macaddr", pszPath);
	if (IPSCTLGetString(pArg->nHandle, szBuffer, szMacAddr, 32) != SYSINFO_RC_SUCCESS)
		return;

	snprintf(szBuffer, 1024, "%s:ifname", pszPath);
	if (IPSCTLGetString(pArg->nHandle, szBuffer, szIfName, MAX_IF_NAME) != SYSINFO_RC_SUCCESS)
		return;

	ptr = strrchr(pszPath, ':');
	if (ptr == NULL)
		return;
	ptr++;

	TranslateStr(szMacAddr, ":", "");
	snprintf(szBuffer, 1024, "%s %s %d", szMacAddr, ptr,
	         if_nametoindex(szIfName) + 0x01000000);
	pArg->pList->add(szBuffer);
}


//
// Handler for Net.ArpCache enum
//

LONG H_NetArpCache(char *pszParam, char *pArg, StringList *pValue, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_ERROR;
	CBP data;

	data.nHandle = ipsctl_open();
	if (data.nHandle > 0)
	{
		data.pList = pValue;
		if (ipsctl_iterate(data.nHandle, "net:ip:arp:entry", ArpCallback, &data) == 0)
			nRet = SYSINFO_RC_SUCCESS;
		ipsctl_close(data.nHandle);
	}

	return nRet;
}

LONG H_NetRoutingTable(char *pszParam, char *pArg, StringList *pValue, AbstractCommSession *session)
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

				pValue->add(szOut);
			}
		}

		free(pRT);
	}

#undef ROUNDUP
#undef sa2sin

	return nRet;
}


//
// Callback for physical interface enumeration
//

static void PhysicalIfCallback(char *pszPath, IFLIST *pList)
{
	char *ptr, szCtl[256], szBuffer[256];
	IFDATA *pCurr;

	// Extract interface name from path
	ptr = strrchr(pszPath, ':');
	if (ptr == NULL)	// Should never happen
		ptr = pszPath;
	else
		ptr++;
	
	// Allocate new interface data structure
	pList->pData = (IFDATA *)realloc(pList->pData, sizeof(IFDATA) * (pList->nSize + 1));
	pCurr = &pList->pData[pList->nSize];
	memset(pCurr, 0, sizeof(IFDATA));
	pList->nSize++;

	// Fill interface data
	nx_strncpy(pCurr->szName, ptr, MAX_IF_NAME);
	strcpy(pCurr->szIpAddr, "0.0.0.0");

	// Interface index
	snprintf(szCtl, 256, "%s:index", pszPath);
	IPSCTLGetInt(pList->nHandle, szCtl, &pCurr->nIndex);

	// Interface type
	snprintf(szCtl, 256, "%s:type", pszPath);
	if (IPSCTLGetString(pList->nHandle, szCtl, szBuffer, 256) == SYSINFO_RC_SUCCESS)
	{
		if (!strcmp(szBuffer, "ethernet"))
			pCurr->nType = IFTYPE_ETHERNET_CSMACD;
		else if (!strcmp(szBuffer, "loopback"))
			pCurr->nType = IFTYPE_SOFTWARE_LOOPBACK;
		else if (!strcmp(szBuffer, "pppoe"))
			pCurr->nType = IFTYPE_EON;
		else
			pCurr->nType = IFTYPE_OTHER;
	}
	else
	{
		pCurr->nType = IFTYPE_OTHER;
	}

	// MAC address
	snprintf(szCtl, 256, "%s:macaddr", pszPath);
	IPSCTLGetString(pList->nHandle, szCtl, pCurr->szMacAddr, 24);
	TranslateStr(pCurr->szMacAddr, ":", "");
}


//
// Callback for logical interface enumeration
//

static void LogicalIfCallback(char *pszPath, IFLIST *pList)
{
	char *ptr, szCtl[256], szBuffer[256];
	IFDATA *pCurr;

	// Extract interface name from path
	ptr = strrchr(pszPath, ':');
	if (ptr == NULL)	// Should never happen
		ptr = pszPath;
	else
		ptr++;
	
	// Allocate new interface data structure
	pList->pData = (IFDATA *)realloc(pList->pData, sizeof(IFDATA) * (pList->nSize + 1));
	pCurr = &pList->pData[pList->nSize];
	memset(pCurr, 0, sizeof(IFDATA));
	pList->nSize++;

	// Fill interface data
	nx_strncpy(pCurr->szName, ptr, MAX_IF_NAME);
	pCurr->isLogical = TRUE;

	// Interface index
	snprintf(szCtl, 256, "%s:index", pszPath);
	IPSCTLGetInt(pList->nHandle, szCtl, &pCurr->nIndex);
	pCurr->nIndex += 0x01000000;

	// Get underlaying physical interface
	snprintf(szCtl, 256, "%s:phys", pszPath);
	IPSCTLGetString(pList->nHandle, szCtl, pCurr->szPhysIfName, MAX_IF_NAME);

	// Interface type
	snprintf(szCtl, 256, "ifphys:%s:type", pCurr->szPhysIfName);
	if (IPSCTLGetString(pList->nHandle, szCtl, szBuffer, 256) == SYSINFO_RC_SUCCESS)
	{
		if (!strcmp(szBuffer, "ethernet"))
			pCurr->nType = IFTYPE_ETHERNET_CSMACD;
		else if (!strcmp(szBuffer, "loopback"))
			pCurr->nType = IFTYPE_SOFTWARE_LOOPBACK;
		else if (!strcmp(szBuffer, "pppoe"))
			pCurr->nType = IFTYPE_EON;
		else
			pCurr->nType = IFTYPE_OTHER;
	}
	else
	{
		pCurr->nType = IFTYPE_OTHER;
	}

	// MAC address
	snprintf(szCtl, 256, "ifphys:%s:macaddr", pCurr->szPhysIfName);
	IPSCTLGetString(pList->nHandle, szCtl, pCurr->szMacAddr, 24);
	TranslateStr(pCurr->szMacAddr, ":", "");
}


//
// Callback for IP address enumeration
//

static void IpAddrCallback(char *pszPath, IFLIST *pList)
{
	IFDATA *pCurr;
	struct ipsctl_value *pValue;
	char *ptr, szCtl[256];

	if (pList->pData[pList->nCurr].szIpAddr[0] != 0)
	{
		// Interface already has IP address, create additional entry
		pList->pData = (IFDATA *)realloc(pList->pData, sizeof(IFDATA) * (pList->nSize + 1));
		pCurr = &pList->pData[pList->nSize];
		memcpy(pCurr, &pList->pData[pList->nCurr], sizeof(IFDATA));
		pList->nSize++;
	}
	else
	{
		pCurr = &pList->pData[pList->nCurr];
	}

	ptr = strrchr(pszPath, ':');
	if (ptr != NULL)
		ptr++;
	else
		ptr = pszPath;
	nx_strncpy(pCurr->szIpAddr, ptr, 16);

	snprintf(szCtl, 256, "%s:dest", pszPath);
	if (ipsctl_get(pList->nHandle, szCtl, &pValue) == 0)
	{
		pCurr->nMask = pValue->data.ipAddr.nMaskBits;
	}
}


//
// Handler for Net.InterfaceList enum
//

LONG H_NetIfList(char *pszParam, char *pArg, StringList *pValue, AbstractCommSession *session)
{
	int i, nSize, nHandle;
	IFLIST ifList;
	char szBuffer[1024];
	LONG nRet = SYSINFO_RC_ERROR;

	nHandle = ipsctl_open();
	if (nHandle > 0)
	{
		ifList.nSize = 0;
		ifList.pData = NULL;
		ifList.nHandle = nHandle;
		if ((ipsctl_iterate(nHandle, "ifphys", 
		                    PhysicalIfCallback, &ifList) == 0) &&
		    (ipsctl_iterate(nHandle, "interface",
		                    LogicalIfCallback, &ifList) == 0))
		{
			nRet = SYSINFO_RC_SUCCESS;
			
			// Gather IP addresses
			nSize = ifList.nSize;
			for(i = 0; i < nSize; i++)
			{
				if (ifList.pData[i].isLogical)
				{
					sprintf(szBuffer, "interface:%s:family:inet:addr",
					        ifList.pData[i].szName);
					ifList.nCurr = i;
					ipsctl_iterate(nHandle, szBuffer, IpAddrCallback, &ifList);
					if (ifList.pData[i].szIpAddr[0] == 0)
						strcpy(ifList.pData[i].szIpAddr, "0.0.0.0");
				}
			}

			for(i = 0; i < ifList.nSize; i++)
			{
				sprintf(szBuffer, "%d %s/%d %d %s %s", ifList.pData[i].nIndex,
				        ifList.pData[i].szIpAddr, ifList.pData[i].nMask,
				        ifList.pData[i].nType, ifList.pData[i].szMacAddr,
				        ifList.pData[i].szName);
				pValue->add(szBuffer);
			}
		}
		ipsctl_close(nHandle);
	}

	return nRet;
}
