/* $Id$ */

/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
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
*/

#include <nms_common.h>
#include <nms_agent.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "net.h"

#define SA_LEN(addr) (sizeof (struct sockaddr))

static char *if_index2name(int fd, int index)
{
	char *ret = NULL;
	struct ifreq ifr;

	ifr.ifr_index = index;
	if (ioctl (fd, SIOCGIFNAME, &ifr) == 0)
	{
		ret = strdup(ifr.ifr_name);
	}

	return ret;
}

static int if_name2index(int fd, char *name)
{
	int ret = 0;
	struct ifreq ifr;

	nx_strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl (fd, SIOCGIFINDEX, &ifr) == 0)
	{
		ret = ifr.ifr_index;
	}

	return ret;
}

LONG H_NetIpForwarding(char *pszParam, char *pArg, char *pValue)
{
	int nVer = CAST_FROM_POINTER(pArg, int);
	int nRet = SYSINFO_RC_ERROR;

	// TODO

	return nRet;
}

LONG H_NetArpCache(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	// TODO

	return nRet;
}

LONG H_NetRoutingTable(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	// TODO
	
	return nRet;
}

LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	int fd, n;
	char *pBuff = NULL;
	int nBuffSize;
	struct ifconf ifc;
	struct ifreq *ifr, *ifend, *ifnext;
	struct ifreq ifrflags, ifrnetmask;
	struct sockaddr *pNetMask;
	int nNetMaskSize;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nBuffSize = 8192;
	while(1)
	{
		pBuff = (char *)malloc(nBuffSize);
		if (pBuff == NULL)
		{
			break;
		}

		ifc.ifc_len = nBuffSize;
		ifc.ifc_buf = pBuff;
		memset(pBuff, 0, nBuffSize);
		if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0 && errno != EINVAL)
		{
			close(fd);
			free(pBuff);
			return SYSINFO_RC_ERROR;
		}

		if (ifc.ifc_len < nBuffSize &&
				(nBuffSize - ifc.ifc_len) > sizeof(ifr->ifr_name) + 255)
		{
			break;
		}

		free(pBuff);
		nBuffSize *= 2;
	}

	ifr = (struct ifreq *)pBuff;
	ifend = (struct ifreq *)(pBuff + ifc.ifc_len);

	for (; ifr < ifend; ifr = ifnext)
	{
		n = SA_LEN(&ifr->ifr_addr) + sizeof(ifr->ifr_name);
		if (n < sizeof(*ifr))
		{
			ifnext = ifr + 1;
		}
		else
		{
			ifnext = (struct ifreq *)((char *)ifr + n);
		}

		if (!(*ifr->ifr_name))
		{
			break;
		}

		nx_strncpy(ifrflags.ifr_name, ifr->ifr_name, sizeof(ifrflags.ifr_name));
		if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifrflags) < 0)
		{
			if (errno == ENXIO)
			{
				continue;
			}

			close(fd);
			free(pBuff);
			return SYSINFO_RC_ERROR;
		}

		strncpy(ifrnetmask.ifr_name, ifr->ifr_name, sizeof(ifrnetmask.ifr_name));
		memcpy(&ifrnetmask.ifr_addr, &ifr->ifr_addr, sizeof(ifrnetmask.ifr_addr));
		if (ioctl(fd, SIOCGIFNETMASK, (char *)&ifrnetmask) < 0)
		{
			if (errno == EADDRNOTAVAIL)
			{
				pNetMask = NULL;
				nNetMaskSize = 0;
			}
			else
			{
				close(fd);
				free(pBuff);
				return SYSINFO_RC_ERROR;
			}
		}
		else
		{
			pNetMask = &ifrnetmask.ifr_addr;
			nNetMaskSize = SA_LEN(netmask);
		}

		char szOut[1024];

		snprintf(szOut, sizeof(szOut), "%d %s/%d %d %s %s",
				if_name2index(fd, ifr->ifr_name),
				inet_ntoa(((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr),
				BitsInMask(htonl(((struct sockaddr_in *)pNetMask)->sin_addr.s_addr)),
				IFTYPE_OTHER,
				"000000000000",
				ifr->ifr_name);
		NxAddResultString(pValue, szOut);
	}

	nRet = SYSINFO_RC_SUCCESS;

	free(pBuff);
	close(fd);

	return nRet;
}

LONG H_NetIfInfoFromIOCTL(char *pszParam, char *pArg, char *pValue)
{
	char *eptr, szBuffer[256];
	LONG nRet = SYSINFO_RC_SUCCESS;
	struct ifreq ifr;
	int fd;

	if (!NxGetParameterArg(pszParam, 1, szBuffer, 256))
	{
		return nRet;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
	{
		return nRet;
	}

	// Check if we have interface name or index
	ifr.ifr_index = strtol(szBuffer, &eptr, 10);
	if (*eptr == 0)
	{
		// Index passed as argument, convert to name
		char *tmpName = if_index2name(fd, ifr.ifr_index);
		if (tmpName == NULL)
		{
			nRet = SYSINFO_RC_ERROR;
		}
		else
		{
			nx_strncpy(ifr.ifr_name, tmpName, IFNAMSIZ);
			free(tmpName);
		}
	}
	else
	{
		// Name passed as argument
		nx_strncpy(ifr.ifr_name, szBuffer, IFNAMSIZ);
		nRet = SYSINFO_RC_SUCCESS;
	}

	// Get interface information
	if (nRet == SYSINFO_RC_SUCCESS)
	{
		switch((long)pArg)
		{
			case IF_INFO_ADMIN_STATUS:
				if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
				{
					ret_int(pValue, (ifr.ifr_flags & IFF_UP) ? 1 : 2);
					nRet = SYSINFO_RC_SUCCESS;
				}
				else
				{
					printf("failed\n");
				}
				break;
			case IF_INFO_OPER_STATUS:
				if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
				{
					// IFF_RUNNING should be set only if interface can
					// transmit/receive data, but in fact looks like it
					// always set. I have unverified information that
					// newer kernels set this flag correctly.
					ret_int(pValue, (ifr.ifr_flags & IFF_RUNNING) ? 1 : 0);
					nRet = SYSINFO_RC_SUCCESS;
				}
				break;
			case IF_INFO_DESCRIPTION:
				ret_string(pValue, ifr.ifr_name);
				nRet = SYSINFO_RC_SUCCESS;
				break;
		}
	}

	// Cleanup
	close(fd);

	return nRet;
}

LONG H_NetIfInfoFromProc(char *pszParam, char *pArg, char *pValue)
{
	return SYSINFO_RC_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2006/10/05 00:34:24  alk
HPUX: minor cleanup; added System.LoggedInCount (W(1) | wc -l equivalent)

Revision 1.1  2006/10/04 14:59:14  alk
initial version of HPUX subagent


*/
