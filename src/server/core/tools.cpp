/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: tools.cpp
**
**/

#include "nms_core.h"

#define PING_SIZE    64


//
// ICMP echo request structure
//

struct ECHOREQUEST
{
   ICMPHDR m_icmpHdr;
   BYTE m_cData[PING_SIZE];
};


//
// ICMP echo reply structure
//

struct ECHOREPLY
{
   IPHDR m_ipHdr;
   ICMPHDR m_icmpHdr;
   BYTE m_cData[256];
};


//
// Strip whitespaces and tabs off the string
//

void StrStrip(char *pStr)
{
   int i;

   for(i = 0; (pStr[i] != 0) && ((pStr[i] == ' ') || (pStr[i] == '\t')); i++);
   if (i > 0)
      memmove(pStr, &pStr[i], strlen(&pStr[i]) + 1);
   for(i = strlen(pStr) - 1; (i >= 0) && ((pStr[i] == ' ') || (pStr[i] == '\t')); i--);
   pStr[i+1] = 0;
}


//
// Get system error string by call to FormatMessage
//

#ifdef _WIN32

char *GetSystemErrorText(DWORD error)
{
   char *msgBuf;
   static char staticBuffer[1024];

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_SYSTEM | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL,error,
                     MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
                     (LPSTR)&msgBuf,0,NULL)>0)
   {
      msgBuf[strcspn(msgBuf,"\r\n")]=0;
      strncpy(staticBuffer,msgBuf,1023);
      LocalFree(msgBuf);
   }
   else
   {
      sprintf(staticBuffer,"MSG 0x%08X - Unable to find message text",error);
   }

   return staticBuffer;
}

#endif   /* _WIN32 */


//
// * Checksum routine for Internet Protocol family headers (C Version)
// *
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983
//

static WORD IPChecksum(WORD *addr, int len)
{
	int nleft = len, sum = 0;
	WORD *w = addr;
	WORD answer;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while(nleft > 1)
   {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) 
   {
		WORD u = 0;

		*(BYTE *)(&u) = *(BYTE *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return answer;
}


//
// Do an ICMP ping to specific address
// Return value: TRUE if host is alive and FALSE otherwise
// Parameters: dwAddr - IP address with network byte order
//             iNumRetries - number of retries
//             dwTimeout - Timeout waiting for responce in milliseconds
//

BOOL IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout)
{
   SOCKET sock;
   struct sockaddr_in saDest;
   BOOL bResult = FALSE;
   ECHOREQUEST request;
   ECHOREPLY reply;
   DWORD dwTimeLeft, dwStartTime, dwElapsedTime;

   // Create raw socket
   sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   if (sock == -1)
   {
      WriteLog(MSG_RAW_SOCK_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Setup destination address structure
   saDest.sin_addr.s_addr = dwAddr;
   saDest.sin_family = AF_INET;
   saDest.sin_port = 0;

   // Fill in request structure
   request.m_icmpHdr.m_cType = 8;   // ICMP ECHO REQUEST
   request.m_icmpHdr.m_cCode = 0;
   request.m_icmpHdr.m_wChecksum = 0;
   request.m_icmpHdr.m_wId = 0x1020;
   request.m_icmpHdr.m_wSeq = 0;

   // Do ping
   while(iNumRetries--)
   {
      request.m_icmpHdr.m_wSeq++;
      request.m_icmpHdr.m_wChecksum = IPChecksum((WORD *)&request, sizeof(ECHOREQUEST));
      if (sendto(sock, (char *)&request, sizeof(ECHOREQUEST), 0, (struct sockaddr *)&saDest, sizeof(struct sockaddr_in)) == sizeof(ECHOREQUEST))
      {
         struct timeval timeout;
         fd_set rdfs;
         int iAddrLen;
         struct sockaddr_in saSrc;

         // Wait for responce
         for(dwTimeLeft = dwTimeout; dwTimeLeft > 0;)
         {
	         FD_ZERO(&rdfs);
	         FD_SET(sock, &rdfs);
	         timeout.tv_sec = dwTimeLeft / 1000;
	         timeout.tv_usec = (dwTimeLeft % 1000) * 1000;
            dwStartTime = GetTickCount();
	         if (select(sock + 1, &rdfs, NULL, NULL, &timeout) != 0)
            {
               dwElapsedTime = GetTickCount() - dwStartTime;
               dwTimeLeft -= min(dwElapsedTime, dwTimeLeft);

               // Receive reply
               iAddrLen = sizeof(struct sockaddr_in);
               if (recvfrom(sock, (char *)&reply, sizeof(ECHOREPLY), 0, (struct sockaddr *)&saSrc, &iAddrLen) > 0)
               {
                  // Check responce
                  if ((reply.m_ipHdr.m_iaSrc.s_addr == dwAddr) && 
                      (reply.m_icmpHdr.m_cType == 0) &&
                      (reply.m_icmpHdr.m_wId == request.m_icmpHdr.m_wId) &&
                      (reply.m_icmpHdr.m_wSeq == request.m_icmpHdr.m_wSeq))
                  {
                     bResult = TRUE;   // We succeed
                     goto stop_ping;   // Stop sending packets
                  }

                  // Check for "destination unreacheable" error
                  if ((reply.m_icmpHdr.m_cType == 3) &&
                      (reply.m_icmpHdr.m_cCode == 1))    // code 1 is "host unreacheable"
                  {
                     if (((IPHDR *)reply.m_icmpHdr.m_cData)->m_iaDst.S_un.S_addr == dwAddr)
                        goto stop_ping;      // Host unreacheable, stop trying
                  }
               }
            }
            else     // select() ended on timeout
            {
               dwTimeLeft = 0;
            }
         }
      }

      ThreadSleepMs(500);     // Wait half a second before sending next packet
   }

stop_ping:
   closesocket(sock);
   return bResult;
}


//
// Clean interface list from unneeded entries
//

void CleanInterfaceList(INTERFACE_LIST *pIfList)
{
   int i;

   if (pIfList == NULL)
      return;

   // Delete loopback interface(s) from list
   for(i = 0; i < pIfList->iNumEntries; i++)
      if ((pIfList->pInterfaces[i].dwIpAddr & pIfList->pInterfaces[i].dwIpNetMask) == 0x0000007F)
      {
         pIfList->iNumEntries--;
         memmove(&pIfList->pInterfaces[i], &pIfList->pInterfaces[i + 1],
                 sizeof(INTERFACE_INFO) * (pIfList->iNumEntries - i));
         i--;
      }
}


//
// Destroy interface list created by discovery functions
//

void DestroyInterfaceList(INTERFACE_LIST *pIfList)
{
   if (pIfList != NULL)
   {
      if (pIfList->pInterfaces != NULL)
         free(pIfList->pInterfaces);
      free(pIfList);
   }
}


//
// Destroy ARP cache created by discovery functions
//

void DestroyArpCache(ARP_CACHE *pArpCache)
{
   if (pArpCache != NULL)
   {
      if (pArpCache->pEntries != NULL)
         free(pArpCache->pEntries);
      free(pArpCache);
   }
}
