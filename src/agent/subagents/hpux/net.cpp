/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2010-2015 Victor Kirhenshtein
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
#include <net/if.h>
#if HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif

#define __STDC__
extern "C" {
#include <sys/mib.h>
}

#include "net.h"

#ifndef IFT_LOOP
#define IFT_LOOP 24
#endif


//
// Internal interface representation
//

#define MAX_IPADDR_COUNT        256

struct IPADDR
{
	DWORD addr;
	DWORD mask;
};

struct NETIF
{
	char device[MAX_PHYSADDR_LEN];
	int ppa;
	int ifIndex;
	char ifName[MAX_NAME_LENGTH];
	int ifType;
	BYTE macAddr[MAX_PHYSADDR_LEN];
	int ipAddrCount;
	IPADDR ipAddrList[MAX_IPADDR_COUNT];
};

#if HAVE_DECL_SIOCGIFNAME

/**
 * Find interface name by interface index
 */
static char *IfIndexToName(int index, char *buffer)
{
	int fd;
	struct ifreq ifr;
	char *ret = NULL;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd != -1)
	{
		ifr.ifr_index = index;
		if (ioctl(fd, SIOCGIFNAME, &ifr) == 0)
		{
			strcpy(buffer, ifr.ifr_name);
			ret = buffer;
		}
		close(fd);
	}

	return ret;
}

#endif

#if HAVE_DECL_SIOCGIFINDEX

/**
 * Find interface index by name
 */
static int IfNameToIndex(const char *name)
{
	int fd, index = -1;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd != -1)
	{
		strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = 0;
		if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0)
		{
			index = ifr.ifr_index;
		}
		close(fd);
	}

	return index;
}

#endif

/**
 * Get interface list
 */
static int GetInterfaceList(NETIF **iflist)
{
	nmapi_iftable ift[1024];
	unsigned int size;
	int i, mib, ifcount;
	NETIF *curr;

	size = sizeof(ift);
	if (get_if_table(ift, &size) != 0)
		return -1;

	ifcount = size / sizeof(nmapi_iftable);
	*iflist = (NETIF *)malloc(ifcount * sizeof(NETIF));
	memset(*iflist, 0, ifcount * sizeof(NETIF));

	for(i = 0; i < ifcount; i++)
	{
		mib = open_mib(ift[i].nm_device, O_RDWR, ift[i].nm_ppanum, 0);
		if (mib >= 0)
		{
			struct nmparms np;
			mib_ifEntry iface;
			unsigned int ifeSize = sizeof(mib_ifEntry);

			np.objid = ID_ifEntry;
			np.buffer = &iface;
			np.len = &ifeSize;
			iface.ifIndex = ift[i].nm_ifindex;
			if (get_mib_info(mib, &np) == 0)
			{
				curr = *iflist + i;
				strcpy(curr->device, ift[i].nm_device);
				curr->ppa = ift[i].nm_ppanum;
				curr->ifIndex = iface.ifIndex;
				curr->ifType = iface.ifType;
				memcpy(curr->macAddr, iface.ifPhysAddress.o_bytes, iface.ifPhysAddress.o_length);
#if HAVE_DECL_SIOCGIFNAME
				IfIndexToName(iface.ifIndex, curr->ifName);
#else
				nx_strncpy(curr->ifName, iface.ifDescr, MAX_NAME_LENGTH);
#endif
			}
			close_mib(mib);
		}
	}

	// Remove loopback and unplumbed interfaces
	for(i = 0; i < ifcount; i++)
	{
		if (((*iflist)[i].ifType == IFT_LOOP) ||	// loopback
		    ((*iflist)[i].ifName[0] == 0))		      // unplumbed
		{
			ifcount--;
			memmove(*iflist + i, *iflist + i + 1, (ifcount - i) * sizeof(NETIF));
			i--;
		}
	}

	// Get IP addresses
	mib = open_mib("/dev/ip", O_RDWR, 0, 0);
	if (mib >= 0)
	{
		struct nmparms np;
		unsigned int addrCount;
		unsigned int bufSize = sizeof(unsigned int);

		np.objid = ID_ipAddrNumEnt;
		np.buffer = &addrCount;
		np.len = &bufSize;
		if (get_mib_info(mib, &np) == 0)
		{
			bufSize = addrCount * sizeof(mib_ipAdEnt);
			mib_ipAdEnt *ipList = (mib_ipAdEnt *)malloc(bufSize);
			memset(ipList, 0, bufSize);
			np.objid = ID_ipAddrTable;
			np.buffer = ipList;
			np.len = &bufSize;
			if (get_mib_info(mib, &np) == 0)
			{
				for(i = 0; i < (int)addrCount; i++)
				{
					if (ipList[i].Addr != 0)
					{
						for(int j = 0; j < ifcount; j++)
						{
							if (((*iflist)[j].ifIndex == ipList[i].IfIndex) && ((*iflist)[j].ipAddrCount < MAX_IPADDR_COUNT))
							{
								int pos = (*iflist)[j].ipAddrCount++;
								(*iflist)[j].ipAddrList[pos].addr = ipList[i].Addr;
								(*iflist)[j].ipAddrList[pos].mask = ipList[i].NetMask;
								break;
							}
						}
					}
				}
			}
			free(ipList);
		}
		close_mib(mib);
	}

	return ifcount;
}

/**
 * Handler for Net.InterfaceList list
 */
LONG H_NetIfList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
	NETIF *ifList;
	int ifCount = GetInterfaceList(&ifList);
	if (ifCount == -1)
		return SYSINFO_RC_ERROR;

	for(int i = 0; i < ifCount; i++)
	{
	   char macAddr[16], ipAddr[32], buffer[256];
		BinToStrA(ifList[i].macAddr, 6, macAddr);
		if (ifList[i].ipAddrCount == 0)
		{
			snprintf(buffer, 256, "%d 0.0.0.0/0 %d %s %s", ifList[i].ifIndex, ifList[i].ifType, macAddr, ifList[i].ifName);
			value->addMBString(buffer);
		}
		else
		{
			snprintf(buffer, 256, "%d %s/%d %d %s %s", ifList[i].ifIndex, IpToStrA(ifList[i].ipAddrList[0].addr, ipAddr),
			         BitsInMask(ifList[i].ipAddrList[0].mask), ifList[i].ifType, macAddr, ifList[i].ifName);
			value->addMBString(buffer);

			for(int j = 1; j < ifList[i].ipAddrCount; j++)
			{
				snprintf(buffer, 256, "%d %s/%d %d %s %s:%d", ifList[i].ifIndex, IpToStrA(ifList[i].ipAddrList[j].addr, ipAddr),
				         BitsInMask(ifList[i].ipAddrList[j].mask), ifList[i].ifType, macAddr, ifList[i].ifName, j);
				value->addMBString(buffer);
			}
		}
	}
	free(ifList);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.InterfaceNames list
 */
LONG H_NetIfNames(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
	NETIF *ifList;
	int ifCount = GetInterfaceList(&ifList);
	if (ifCount == -1)
		return SYSINFO_RC_ERROR;

	for(int i = 0; i < ifCount; i++)
	{
		value->addMBString(ifList[i].ifName);
	}
	free(ifList);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.ArpCache enum
 */
LONG H_NetArpCache(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
	int i, mib;
	struct nmparms np;
	mib_ipNetToMediaEnt arpCache[8192];
	unsigned int size, count;
	char buffer[256], mac[MAX_PHYSADDR_LEN * 2 + 1], ip[32];
	LONG nRet = SYSINFO_RC_ERROR;

	mib = open_mib("/dev/ip", O_RDWR, 0, 0);
	if (mib >= 0)
	{
		size = sizeof(arpCache);
		np.objid = ID_ipNetToMediaTable;
		np.buffer = arpCache;
		np.len = &size;
		if (get_mib_info(mib, &np) == 0)
		{
			count = size / sizeof(mib_AtEntry);
			for(i = 0; i < count; i++)
			{
				if (((arpCache[i].Type == INTM_DYNAMIC) || (arpCache[i].Type == INTM_STATIC)) &&
				    (arpCache[i].PhysAddr.o_length > 0))
				{
					snprintf(buffer, 256, "%s %s %d", BinToStrA(arpCache[i].PhysAddr.o_bytes, arpCache[i].PhysAddr.o_length, mac),
					         IpToStrA(arpCache[i].NetAddr, ip), arpCache[i].IfIndex);
					pValue->addMBString(buffer);
				}
			}
			nRet = SYSINFO_RC_SUCCESS;
		}
		close_mib(mib);
	}
	return nRet;
}

/**
 * Handler for Net.IP.Forwarding parameter
 */
LONG H_NetIpForwarding(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	int ipVer = CAST_FROM_POINTER(pArg, int);
	int nRet = SYSINFO_RC_ERROR;
	int mib;
	unsigned int size, fwFlag;
	struct nmparms np;
	
	mib = open_mib((ipVer == 4) ? "/dev/ip" : "/dev/ip6", O_RDWR, 0, 0);
	if (mib >= 0)
	{
		size = sizeof(unsigned int);
		np.objid = (ipVer == 4) ? ID_ipForwarding : ID_ipv6Forwarding;
		np.buffer = &fwFlag;
		np.len = &size;
		if (get_mib_info(mib, &np) == 0)
		{
			ret_int(pValue, (fwFlag == 1) ? 1 : 0);
			nRet = SYSINFO_RC_SUCCESS;
		}
		close_mib(mib);
	}

	return nRet;
}


LONG H_NetRoutingTable(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;

	// TODO
	
	return nRet;
}

/**
 * Handler for Net.Interface.* parameters
 */
LONG H_NetIfInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	char *eptr, buffer[256];
	nmapi_iftable ift[1024];
	int i, mib, ifCount, ifIndex;
	unsigned int size;
	LONG nRet = SYSINFO_RC_SUCCESS;

	if (!AgentGetParameterArgA(pszParam, 1, buffer, 256))
	{
		return SYSINFO_RC_ERROR;
	}

	// Check if we have interface name or index
	ifIndex = strtol(buffer, &eptr, 10);
	if (*eptr != 0)
	{
		// Name passed as argument, convert to index
#if HAVE_DECL_SIOCGIFINDEX
		ifIndex = IfNameToIndex(buffer);
#else
		return SYSINFO_RC_UNSUPPORTED;	// Unable to convert interface name to index
#endif
	}
	if (ifIndex <= 0)
		return SYSINFO_RC_UNSUPPORTED;

	// Find driver for interface
	size = sizeof(ift);
	if (get_if_table(ift, &size) != 0)
		return SYSINFO_RC_ERROR;
	ifCount = size / sizeof(nmapi_iftable);
	for(i = 0; i < ifCount; i++)
		if (ift[i].nm_ifindex == ifIndex)
			break;
	if (i == ifCount)
		return SYSINFO_RC_UNSUPPORTED;	// Interface not found

	mib = open_mib(ift[i].nm_device, O_RDWR, ift[i].nm_ppanum, 0);
	if (mib >= 0)
	{
		struct nmparms np;
		mib_ifEntry iface;
		unsigned int ifeSize = sizeof(mib_ifEntry);

		np.objid = ID_ifEntry;
		np.buffer = &iface;
		np.len = &ifeSize;
		iface.ifIndex = ift[i].nm_ifindex;
		if (get_mib_info(mib, &np) == 0)
		{
			nRet = SYSINFO_RC_SUCCESS;

			// Get interface information
			switch((long)pArg)
			{
				case IF_INFO_ADMIN_STATUS:
					ret_int(pValue, iface.ifAdmin);
					break;
				case IF_INFO_OPER_STATUS:
					ret_int(pValue, (iface.ifOper == 1) ? 1 : 0);
					break;
				case IF_INFO_DESCRIPTION:
					ret_mbstring(pValue, iface.ifDescr);
					break;
				case IF_INFO_MTU:
					ret_int(pValue, iface.ifMtu);
					break;
				case IF_INFO_SPEED:
					ret_uint(pValue, iface.ifSpeed);
					break;
				case IF_INFO_BYTES_IN:
					ret_uint(pValue, iface.ifInOctets);
					break;
				case IF_INFO_BYTES_OUT:
					ret_uint(pValue, iface.ifOutOctets);
					break;
				case IF_INFO_PACKETS_IN:
					ret_uint(pValue, iface.ifInUcastPkts + iface.ifInNUcastPkts);
					break;
				case IF_INFO_PACKETS_OUT:
					ret_uint(pValue, iface.ifOutUcastPkts + iface.ifOutNUcastPkts);
					break;
				case IF_INFO_IN_ERRORS:
					ret_uint(pValue, iface.ifInErrors);
					break;
				case IF_INFO_OUT_ERRORS:
					ret_uint(pValue, iface.ifOutErrors);
					break;
				default:
					nRet = SYSINFO_RC_UNSUPPORTED;
					break;
			}
		}
		close_mib(mib);
	}

	return nRet;
}
