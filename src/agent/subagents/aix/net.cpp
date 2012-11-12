/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2012 Victor Kirhenshtein
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


//
// Function getkerninfo() has not documented, but used in IBM examples.
// It also doesn't have prototype in headers, so we declare it here.
//

#if !HAVE_DECL_GETKERNINFO
extern "C" int getkerninfo(int, void *, void *, void *);
#endif


//
// Internal interface info structure
//

typedef struct
{
	char name[IFNAMSIZ];
	BYTE mac[6];
	DWORD ip;
	DWORD netmask;
	DWORD iftype;
} IF_INFO;


//
// Static data
//

static perfstat_netinterface_t *s_ifaceData = NULL;
static int s_ifaceDataSize = 0;
static time_t s_ifaceDataTimestamp = 0;
static MUTEX s_ifaceDataLock = INVALID_MUTEX_HANDLE;


//
// Initialize network stuff
//

void InitNetworkDataCollection()
{
	s_ifaceDataLock = MutexCreate();
}


//
// Get data for all interfaces via libperfstat
//

static bool GetInterfaceData()
{
	int ifCount = perfstat_netinterface(NULL, NULL, sizeof(perfstat_netinterface_t), 0);
	if (ifCount < 1)
		return false;

	if (ifCount != s_ifaceDataSize)
	{
		s_ifaceDataSize = ifCount;
		s_ifaceData = (perfstat_netinterface_t *)realloc(s_ifaceData, sizeof(perfstat_netinterface_t) * ifCount);
	}

	perfstat_id_t firstIface;
	strcpy(firstIface.name, FIRST_NETINTERFACE);
	bool success = perfstat_netinterface(&firstIface, s_ifaceData, sizeof(perfstat_netinterface_t), ifCount) > 0;
	if (success)
		s_ifaceDataTimestamp = time(NULL);
	return success;
}


//
// Get interface data
//

LONG H_NetInterfaceInfo(const char *param, const char *arg, char *value)
{
	char ifName[IF_NAMESIZE], *eptr;

	if (!AgentGetParameterArg(param, 1, ifName, IF_NAMESIZE))
	{
		return SYSINFO_RC_ERROR;
	}

	// Check if we have interface name or index
	unsigned int ifIndex = (unsigned int)strtoul(ifName, &eptr, 10);
	if (*eptr == 0)
	{
		// Index passed as argument, convert to name
		if (if_indextoname(ifIndex, ifName) == NULL)
		{
			AgentWriteDebugLog(7, "AIX: unable to resolve interface index %u", ifIndex);
			return SYSINFO_RC_ERROR;
		}
	}

	MutexLock(s_ifaceDataLock);

	LONG nRet = SYSINFO_RC_SUCCESS;
	if (time(NULL) - s_ifaceDataTimestamp > 5)
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
					ret_string(value, s_ifaceData[i].description);
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

	MutexUnlock(s_ifaceDataLock);
	return nRet;
}


//
// Get MAC address and type for interface via getkerninfo()
//

static void GetNDDInfo(char *pszDevice, BYTE *pMacAddr, DWORD *pdwType)
{
	struct kinfo_ndd *nddp;
	int i, size, nrec;

	size = getkerninfo(KINFO_NDD, 0, 0, 0);
	if (size <= 0)
		return;  // getkerninfo() error
	
	nddp = (struct kinfo_ndd *)malloc(size);
	if (getkerninfo(KINFO_NDD, nddp, &size, 0) >= 0)
	{
		nrec = size / sizeof(struct kinfo_ndd);
		for(i = 0; i < nrec; i++)
		{
			if ((!strcmp(pszDevice, nddp[i].ndd_name)) ||
			    (!strcmp(pszDevice, nddp[i].ndd_alias)))
			{
				memcpy(pMacAddr, nddp[i].ndd_addr, 6);
				*pdwType = nddp[i].ndd_type;
				break;
			}
		}
	}
	free(nddp);
}


//
// Handler for Net.InterfaceList enum
//

LONG H_NetInterfaceList(const char *pszParam, const char *pArg, StringList *value)
{
	LONG nRet;
	struct ifconf ifc;
	struct ifreq ifr;
	IF_INFO *ifl;
	char szBuffer[256], szIpAddr[16], szMacAddr[32];
	int i, j, sock, nifs = 0, nrecs = 10;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock <= 0)
		return SYSINFO_RC_ERROR;

retry_ifconf:
	ifc.ifc_len = nrecs * sizeof(struct ifreq);
	ifc.ifc_buf = (caddr_t)malloc(ifc.ifc_len);
#ifdef OSIOCGIFCONF
	if (ioctl(sock, OSIOCGIFCONF, &ifc) == 0)
#else
	if (ioctl(sock, SIOCGIFCONF, &ifc) == 0)
#endif
	{
		if (ifc.ifc_len == nrecs * sizeof(struct ifconf))
		{
			// Assume overlolad, so increase buffer and retry
			free(ifc.ifc_buf);
			nrecs += 10;
			goto retry_ifconf;
		}

		nrecs = ifc.ifc_len / sizeof(struct ifreq);
		ifl = (IF_INFO *)malloc(sizeof(IF_INFO) * nrecs);
		memset(ifl, 0, sizeof(IF_INFO) * nrecs);
		for(i = 0, nifs = 0; i < nrecs; i++)
		{
			// Check if interface already in internal table
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
				strcpy(ifr.ifr_name, ifc.ifc_req[i].ifr_name);
				ifr.ifr_addr.sa_family = AF_INET;
				if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0)
				{
					ifl[j].netmask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
				}
			}
		}

		// Create result list
		for(i = 0; i < nifs; i++)
		{
			sprintf(szBuffer, "%d %s/%d %d %s %s",
			        if_nametoindex(ifl[i].name),
			        IpToStr(ntohl(ifl[i].ip), szIpAddr),
			        BitsInMask(ifl[i].netmask), ifl[i].iftype,
				BinToStr(ifl[i].mac, 6, szMacAddr), ifl[i].name);
			value->add(szBuffer);
		}
		free(ifl);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}

	free(ifc.ifc_buf);
	close(sock);
	return nRet;
}


//
// Handler for Net.Interface.AdminStatus parameter
//

LONG H_NetInterfaceStatus(const char *param, const char *arg, char *value)
{
	int nRet = SYSINFO_RC_ERROR;
	char ifName[IF_NAMESIZE], *eptr;

	AgentGetParameterArg(param, 1, ifName, IF_NAMESIZE);

	// Check if we have interface name or index
	unsigned int ifIndex = (unsigned int)strtoul(ifName, &eptr, 10);
	if (*eptr == 0)
	{
		// Index passed as argument, convert to name
		if (if_indextoname(ifIndex, ifName) == NULL)
		{
			AgentWriteDebugLog(7, "AIX: unable to resolve interface index %u", ifIndex);
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
			AgentWriteDebugLog(7, "AIX: internal error in H_NetIfterfaceStatus (invalid flag requested)");
			return SYSINFO_RC_ERROR;
	}

	int nSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (nSocket > 0)
	{
		struct ifreq ifr;
		int flags;

		memset(&ifr, 0, sizeof(ifr));
		nx_strncpy(ifr.ifr_name, ifName, sizeof(ifr.ifr_name));
		if (ioctl(nSocket, SIOCGIFFLAGS, (caddr_t)&ifr) >= 0)
		{
			if ((ifr.ifr_flags & requestedFlag) == requestedFlag)
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

	return nRet;
}
