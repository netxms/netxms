/* $Id: net.cpp,v 1.4 2005-01-05 12:21:24 victor Exp $ */

/* 
** NetXMS subagent for GNU/Linux
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

#include <linux/sysctl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

LONG H_NetIpForwarding(char *pszParam, char *pArg, char *pValue)
{
	int nVer = (int)pArg;
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
	char *pFileName = NULL;

	switch (nVer)
	{
	case 4:
		pFileName = "/proc/sys/net/ipv4/conf/all/forwarding";
		break;
	case 6:
		pFileName = "/proc/sys/net/ipv6/conf/all/forwarding";
		break;
	}

	if (pFileName != NULL)
	{
		hFile = fopen(pFileName, "r");
		if (hFile != NULL)
		{
			char szBuff[4];

			if (fgets(szBuff, sizeof(szBuff), hFile) != NULL)
			{
				nRet = SYSINFO_RC_SUCCESS;
				switch (szBuff[0])
				{
				case '0':
					ret_int(pValue, 0);
					break;
				case '1':
					ret_int(pValue, 1);
					break;
				default:
					nRet = SYSINFO_RC_ERROR;
					break;
				}
			}
			fclose(hFile);
		}
	}

	return nRet;
}

LONG H_NetArpCache(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;

	hFile = fopen("/proc/net/arp", "r");
	if (hFile != NULL)
	{
		char szBuff[256];
		int nFd;

		nFd = socket(AF_INET, SOCK_DGRAM, 0);
		if (nFd > 0)
		{
			nRet = SYSINFO_RC_SUCCESS;

			fgets(szBuff, sizeof(szBuff), hFile); // skip first line

			while (fgets(szBuff, sizeof(szBuff), hFile) != NULL)
			{
				int nIP1, nIP2, nIP3, nIP4;
				int nMAC1, nMAC2, nMAC3, nMAC4, nMAC5, nMAC6;
				char szTmp1[256];
				char szTmp2[256];
				char szTmp3[256];
				char szIf[256];

				if (sscanf(szBuff,
						"%d.%d.%d.%d %s %s %02X:%02X:%02X:%02X:%02X:%02X %s %s",
						&nIP1, &nIP2, &nIP3, &nIP4,
						szTmp1, szTmp2,
						&nMAC1, &nMAC2, &nMAC3, &nMAC4, &nMAC5, &nMAC6,
						szTmp3, szIf) == 14)
				{
					int nIndex;
					struct ifreq irq;

					if (nMAC1 == 0 && nMAC2 == 0 &&
						nMAC3 == 0 && nMAC4 == 0 &&
						nMAC5 == 0 && nMAC6 == 0)
					{
						// incomplete
						continue;
					}

					strncpy(irq.ifr_name, szIf, IFNAMSIZ);
					if (ioctl(nFd, SIOCGIFINDEX, &irq) != 0)
					{
						perror("ioctl()");
						nIndex = 0;
					}
					else
					{
						nIndex = irq.ifr_ifindex;
					}
					
					snprintf(szBuff, sizeof(szBuff),
							"%02X%02X%02X%02X%02X%02X %d.%d.%d.%d %d",
							nMAC1, nMAC2, nMAC3, nMAC4, nMAC5, nMAC6,
							nIP1, nIP2, nIP3, nIP4,
							nIndex);

					NxAddResultString(pValue, szBuff);
				}
			}

			close(nFd);
		}
		
		fclose(hFile);
	}

	return nRet;
}

LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
   struct if_nameindex *pIndex;
   struct ifreq irq;
   struct sockaddr_in *sa;
	int nFd;

	pIndex = if_nameindex();
	if (pIndex != NULL)
	{
		nFd = socket(AF_INET, SOCK_DGRAM, 0);
		if (nFd >= 0)
		{
			for (int i = 0; pIndex[i].if_index != 0; i++)
			{
				char szOut[256];
				struct sockaddr_in *sa;
				struct in_addr in;
				char szIpAddr[32];
				char szMacAddr[32];
				int nMask;

				nRet = SYSINFO_RC_SUCCESS;

				strcpy(irq.ifr_name, pIndex[i].if_name);
				if (ioctl(nFd, SIOCGIFADDR, &irq) == 0)
				{
					sa = (struct sockaddr_in *)&irq.ifr_addr;
					strncpy(szIpAddr, inet_ntoa(sa->sin_addr), sizeof(szIpAddr));
				}
				else
				{
					nRet = SYSINFO_RC_ERROR;
				}

				if (nRet == SYSINFO_RC_SUCCESS)
				{
					if (ioctl(nFd, SIOCGIFNETMASK, &irq) == 0)
					{
						sa = (struct sockaddr_in *)&irq.ifr_addr;
						nMask = BitsInMask(htonl(sa->sin_addr.s_addr));
					}
				}
				else
				{
					nRet = SYSINFO_RC_ERROR;
				}

				if (nRet == SYSINFO_RC_SUCCESS)
				{
					if (ioctl(nFd, SIOCGIFHWADDR, &irq) == 0)
					{
						for (int z = 0; z < 6; z++)
						{
							sprintf(&szMacAddr[z << 1], "%02X",
									(unsigned char)irq.ifr_hwaddr.sa_data[z]);
						}
					}
               else
               {
					   nRet = SYSINFO_RC_ERROR;
               }
				}
				else
				{
					nRet = SYSINFO_RC_ERROR;
				}

				if (nRet == SYSINFO_RC_SUCCESS)
				{
					snprintf(szOut, sizeof(szOut), "%d %s/%d %d %s %s",
							pIndex[i].if_index,
							szIpAddr,
							nMask,
							IFTYPE_OTHER,
							szMacAddr,
							pIndex[i].if_name);
					NxAddResultString(pValue, szOut);
				}
            else
            {
               break;   // Stop enumerating interfaces on error
            }
			}

			close(nFd);
		}
      if_freenameindex(pIndex);
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
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
