/* 
** NetXMS subagent for IPSO
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
**/

#ifndef __IPSO_H__
#define __IPSO_H__

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>


//
// IPSO system functions and structures
//

struct ipsctl_value
{
	WORD wSize;
	WORD wType;
	union
	{
		LONG nInt;
		DWORD dwUInt;
		INT64 nInt64;
		QWORD qwUInt64;
		char szStr[1];
		struct
		{
			DWORD dwAddr;
			BYTE nMaskBits;
		} ipAddr;
	} data;
};

#ifdef __cplusplus
extern "C" {
#endif

int ipsctl_open(void);
int ipsctl_close(int nHandle);
int ipsctl_get(int nHandle, char *pszName, struct ipsctl_value **ppData);
int ipsctl_iterate(int nHandle, char *pszRoot,
                   void (* pfCallback)(char *, void *), void *pArg);

char *if_indextoname(unsigned int ifindex, char *ifname);
unsigned int if_nametoindex(char *ifname);

#ifdef __cplusplus
}
#endif


//
// Wrappers for ipsctl_get()
//

LONG IPSCTLGetInt(int nCallerHandle, char *pszName, LONG *pnValue);
LONG IPSCTLGetString(int nCallerHandle, char *pszName,
                     char *pszValue, int nSize);


//
// File system stats
//

enum
{
	DISK_AVAIL,
	DISK_AVAIL_PERC,
	DISK_FREE,
	DISK_FREE_PERC,
	DISK_USED,
	DISK_USED_PERC,
	DISK_TOTAL,
};


//
// Memory stats
//

enum
{
	PHYSICAL_FREE,
	PHYSICAL_FREE_PCT,
	PHYSICAL_USED,
	PHYSICAL_USED_PCT,
	PHYSICAL_TOTAL,
	SWAP_FREE,
	SWAP_FREE_PCT,
	SWAP_USED,
	SWAP_USED_PCT,
	SWAP_TOTAL,
	VIRTUAL_FREE,
	VIRTUAL_FREE_PCT,
	VIRTUAL_USED,
	VIRTUAL_USED_PCT,
	VIRTUAL_TOTAL,
};


//
// Network interface stats
//

enum
{
	NET_IF_DESCRIPTION,
	NET_IF_LINK,
	NET_IF_ADMIN_STATUS,
	NET_IF_BYTES_IN,
	NET_IF_BYTES_OUT,
	NET_IF_PACKETS_IN,
	NET_IF_PACKETS_OUT
};


//
// Handlers
//

LONG H_DiskInfo(char *, char *, char *, AbstractCommSession *);
LONG H_ProcessList(char *, char *, StringList *, AbstractCommSession *);
LONG H_Uptime(char *, char *, char *, AbstractCommSession *);
LONG H_Uname(char *, char *, char *, AbstractCommSession *);
LONG H_Hostname(char *, char *, char *, AbstractCommSession *);
LONG H_CpuCount(char *, char *, char *, AbstractCommSession *);
LONG H_CpuLoad(char *, char *, char *, AbstractCommSession *);
LONG H_CpuUsage(char *, char *, char *, AbstractCommSession *);
LONG H_ProcessCount(char *, char *, char *, AbstractCommSession *);
LONG H_MemoryInfo(char *, char *, char *, AbstractCommSession *);
LONG H_NetIpForwarding(char *, char *, char *, AbstractCommSession *);
LONG H_NetIfStats(char *, char *, char *, AbstractCommSession *);
LONG H_NetArpCache(char *, char *, StringList *, AbstractCommSession *);
LONG H_NetIfList(char *, char *, StringList *, AbstractCommSession *);
LONG H_NetRoutingTable(char *, char *, StringList *, AbstractCommSession *);

#endif /* __IPSO_H__ */
