/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: net.cpp
**
**/

#include "sunos_subagent.h"
#include <net/if.h>
#include <sys/sockio.h>


//
// Determine interface type by it's name
//

static int InterfaceTypeFromName(char *pszName)
{
   int iType = 0;

   switch(pszName[0])
   {
      case 'l':   // le / lo / lane (ATM LAN Emulation)
         switch(pszName[1])
         {
            case 'o':
               iType = 24;
               break;
            case 'e':
               iType = 6;
               break;
            case 'a':
               iType = 37;
               break;
         }
         break;
      case 'g':   // (gigabit ethernet card)
         iType = 6;
         break;
      case 'h':   // hme (SBus card)
      case 'e':   // eri (PCI card)
      case 'b':   // be
      case 'd':   // dmfe -- found on netra X1
         iType = 6;
         break;
      case 'f':   // fa (Fore ATM)
         iType = 37;
         break;
      case 'q':   // qe (QuadEther)/qa (Fore ATM)/qfe (QuadFastEther)
         switch(pszName[1])
         {
            case 'a':
               iType = 37;
               break;
            case 'e':
            case 'f':
               iType = 6;
               break;
         }
         break;
   }
   return iType;
}


//
// Get interface's hardware address
//

static BOOL GetInterfaceHWAddr(char *pszIfName, char *pszMacAddr)
{
   BYTE macAddr[6];
	int i;

   if (mac_addr_dlpi(pszIfName, macAddr) == 0)
	{
		for(i = 0; i < 6; i++)
			sprintf(&pszMacAddr[i << 1], "%02X", macAddr[i]);
	}
	else
	{
   	strcpy(pszMacAddr, "000000000000");
	}
   return TRUE;
}


//
// Interface list
//

LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct lifnum ln;
	struct lifconf lc;
   struct lifreq rq;
	int i, nFd;

	nFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (nFd >= 0)
	{
		ln.lifn_family = AF_INET;
		ln.lifn_flags = 0;
		if (ioctl(nFd, SIOCGLIFNUM, &ln) == 0)
		{
			lc.lifc_family = AF_INET;
			lc.lifc_flags = 0;
			lc.lifc_len = sizeof(struct lifreq) * ln.lifn_count;
			lc.lifc_buf = (caddr_t)malloc(lc.lifc_len);
			if (ioctl(nFd, SIOCGLIFCONF, &lc) == 0)
			{
				for (i = 0; i < ln.lifn_count; i++)
				{
					char szOut[256];
					char szIpAddr[32];
					char szMacAddr[32];
					int nMask;

					nRet = SYSINFO_RC_SUCCESS;

					strcpy(rq.lifr_name, lc.lifc_req[i].lifr_name);
					if (ioctl(nFd, SIOCGLIFADDR, &rq) == 0)
					{
						strncpy(szIpAddr, inet_ntoa(((struct sockaddr_in *)&rq.lifr_addr)->sin_addr), sizeof(szIpAddr));
					}
					else
					{
						nRet = SYSINFO_RC_ERROR;
						break;
					}

					if (ioctl(nFd, SIOCGLIFNETMASK, &rq) == 0)
					{
						nMask = BitsInMask(htonl(((struct sockaddr_in *)&rq.lifr_addr)->sin_addr.s_addr));
					}
					else
					{
						nRet = SYSINFO_RC_ERROR;
						break;
					}

					if (!GetInterfaceHWAddr(lc.lifc_req[i].lifr_name, szMacAddr))
					{
						nRet = SYSINFO_RC_ERROR;
						break;
					}

					if (ioctl(nFd, SIOCGLIFINDEX, &rq) == 0)
					{
						snprintf(szOut, sizeof(szOut), "%d %s/%d %d %s %s",
								   rq.lifr_index, szIpAddr, nMask,
									InterfaceTypeFromName(lc.lifc_req[i].lifr_name),
									szMacAddr, lc.lifc_req[i].lifr_name);
						NxAddResultString(pValue, szOut);
					}
					else
					{
						nRet = SYSINFO_RC_ERROR;
						break;
					}
				}
			}

			free(lc.lifc_buf);
		}
		close(nFd);
	}

	return nRet;
}
