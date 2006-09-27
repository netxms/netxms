/* $Id: net.cpp,v 1.1 2006-09-27 04:15:25 victor Exp $ */
/*
** NetXMS subagent for AIX
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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

LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
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
				if (!strcmp(ifl[j].name, ifc.ifc_req[i].ifr_name))
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
			NxAddResultString(pValue, szBuffer);
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
