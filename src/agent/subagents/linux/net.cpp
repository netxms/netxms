/* 
 ** NetXMS subagent for GNU/Linux
 ** Copyright (C) 2004-2013 Raden Solutions
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

#include <linux_subagent.h>

LONG H_NetIpForwarding(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
   int nVer = CAST_FROM_POINTER(pArg, int);
   int nRet = SYSINFO_RC_ERROR;
   FILE *hFile;
   const char *pFileName = NULL;

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

LONG H_NetArpCache(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
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

         char *__unused = fgets(szBuff, sizeof(szBuff), hFile); // skip first line

         while(fgets(szBuff, sizeof(szBuff), hFile) != NULL)
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

               TCHAR output[256];
               _sntprintf(output, 256,
                  _T("%02X%02X%02X%02X%02X%02X %d.%d.%d.%d %d"),
                  nMAC1, nMAC2, nMAC3, nMAC4, nMAC5, nMAC6,
                  nIP1, nIP2, nIP3, nIP4,
                  nIndex);

               pValue->add(output);
            }
         }

         close(nFd);
      }

      fclose(hFile);
   }

   return nRet;
}

LONG H_NetRoutingTable(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
{
   int nRet = SYSINFO_RC_ERROR;
   FILE *hFile;
   int nFd;

   nFd = socket(AF_INET, SOCK_DGRAM, 0);
   if (nFd <= 0)
   {
      return SYSINFO_RC_ERROR;
   }

   hFile = fopen("/proc/net/route", "r");
   if (hFile == NULL)
   {
      close(nFd);
      return SYSINFO_RC_ERROR;
   }

   char szLine[256];

   if (fgets(szLine, sizeof(szLine), hFile) != NULL)
   {
      if (!strncmp(szLine,
         "Iface\tDestination\tGateway \tFlags\tRefCnt\t"
            "Use\tMetric\tMask", 55))
      {
         nRet = SYSINFO_RC_SUCCESS;

         while(fgets(szLine, sizeof(szLine), hFile) != NULL)
         {
            char szIF[64];
            int nTmp, nType = 0;
            unsigned int nDestination, nGateway, nMask;

            if (sscanf(szLine,
               "%s\t%08X\t%08X\t%d\t%d\t%d\t%d\t%08X",
               szIF,
               &nDestination,
               &nGateway,
               &nTmp, &nTmp, &nTmp, &nTmp,
               &nMask) == 8)
            {
               int nIndex;
               struct ifreq irq;

               strncpy(irq.ifr_name, szIF, IFNAMSIZ);
               if (ioctl(nFd, SIOCGIFINDEX, &irq) != 0)
               {
                  perror("ioctl()");
                  nIndex = 0;
               }
               else
               {
                  nIndex = irq.ifr_ifindex;
               }

               TCHAR output[256], szBuf1[64], szBuf2[64];
               _sntprintf(output, 256, _T("%s/%d %s %d %d"),
                  IpToStr(ntohl(nDestination), szBuf1),
                  BitsInMask(htonl(nMask)),
                  IpToStr(ntohl(nGateway), szBuf2),
                  nIndex,
                  nType);
               pValue->add(output);
            }
         }
      }
   }

   close(nFd);
   fclose(hFile);

   return nRet;
}

static int SendMessage(int socket)
{
   sockaddr_nl kernel = {};
   msghdr message = {};
   iovec io;
   NETLINK_REQ request = {};

   kernel.nl_family = AF_NETLINK;

   request.header.nlmsg_len = NLMSG_LENGTH(sizeof(rtgenmsg));
   request.header.nlmsg_type = RTM_GETLINK;
   request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
   request.header.nlmsg_seq = 1;
   request.header.nlmsg_pid = GetCurrentProcessId();
   request.message.rtgen_family = AF_PACKET;

   io.iov_base = &request;
   io.iov_len = request.header.nlmsg_len;
   message.msg_iov = &io;
   message.msg_iovlen = 1;
   message.msg_name = &kernel;
   message.msg_namelen = sizeof(kernel);

   return sendmsg(socket, (msghdr*) &message, 0);
}

static int ReceiveMessage(int socket, char* replyBuffer)
{
   iovec io;
   iovec io_reply = {};
   msghdr reply = {};
   sockaddr_nl kernel;

   io.iov_base = replyBuffer;
   io.iov_len = 4096;
   kernel.nl_family = AF_NETLINK;
   reply.msg_iov = &io;
   reply.msg_iovlen = 1;
   reply.msg_name = &kernel;
   reply.msg_namelen = sizeof(kernel);

   return recvmsg(socket, &reply, 0);
}

static IFINFO* ParseMessage(nlmsghdr* messageHeader)
{
   ifinfomsg* interface;
   rtattr* attribute;
   int len;
   IFINFO* interfaceInfo = new IFINFO();

   interface = (ifinfomsg*) NLMSG_DATA(messageHeader);
   len = messageHeader->nlmsg_len - NLMSG_LENGTH(sizeof(*interface));

   for(attribute = IFLA_RTA(interface); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
   {

      if (attribute->rta_type == IFLA_IFNAME)
      {
         strncpy(interfaceInfo->name, (char*) RTA_DATA(attribute), sizeof(interfaceInfo->name));
      }

      if (attribute->rta_type == IFLA_ADDRESS)
      {
         BYTE* hw = (BYTE*) RTA_DATA(attribute);
         snprintf(interfaceInfo->mac, sizeof(interfaceInfo->mac), "%02X%02X%02X%02X%02X%02X", hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
      }
   }

   // TODO: replace these calls with netlink (if possible)
   int inetSocket;
   struct ifreq ifr = {};

   inetSocket = socket(AF_INET, SOCK_DGRAM, 0);

   ifr.ifr_addr.sa_family = AF_INET;
   strncpy(ifr.ifr_name, interfaceInfo->name, IFNAMSIZ - 1);

   ioctl(inetSocket, SIOCGIFADDR, &ifr);
   sockaddr_in *socketAddress = (sockaddr_in *)&ifr.ifr_addr;
   inet_ntop(AF_INET, &socketAddress->sin_addr, interfaceInfo->addr, 24);

   ioctl(inetSocket, SIOCGIFNETMASK, &ifr);
   sockaddr_in *socketMask = (sockaddr_in *)&ifr.ifr_addr;
   interfaceInfo->mask = (BYTE)BitsInMask(ntohl(socketMask->sin_addr.s_addr));

   close(inetSocket);

   interfaceInfo->index = interface->ifi_index;
   interfaceInfo->type = interface->ifi_type;

   return interfaceInfo;
}

static IFINFO *GetInterfaceInfo(int socket)
{
   if (SendMessage(socket) == -1)
   {
      AgentWriteDebugLog(4, _T("GetInterfaceInfo: SendMessage failed (%s)"), _tcserror(errno));
      return NULL;
   }

   msghdr reply = {};
   nlmsghdr* msg_ptr;
   int msgLen;
   int done = 0;
   IFINFO* interfaceInfo = NULL;
   IFINFO* curIfInfo = NULL;
   char replyBuffer[8192];

   while(!done)
   {
      msgLen = ReceiveMessage(socket, replyBuffer);
      if (msgLen <= 0)
      {
         AgentWriteDebugLog(4, _T("GetInterfaceInfo: ReceiveMessage failed (%s)"), _tcserror(errno));
         break;
      }

      for(msg_ptr = (nlmsghdr*) replyBuffer; NLMSG_OK(msg_ptr, msgLen); msg_ptr = NLMSG_NEXT(msg_ptr, msgLen))
      {
         if (msg_ptr->nlmsg_type == RTM_NEWLINK)
         {
            IFINFO* ifInfo = ParseMessage(msg_ptr);

            if (curIfInfo != NULL)
               curIfInfo->next = ifInfo;
            else
               interfaceInfo = ifInfo;

            curIfInfo = ifInfo;
         }
         else if (msg_ptr->nlmsg_type == NLMSG_DONE)
         {
            done++;
         }
      }
   }

   return interfaceInfo;
}

static void FreeInterfaceInfo(IFINFO* ifInfo)
{
   if (ifInfo == NULL)
      return;

   FreeInterfaceInfo(ifInfo->next);
   delete ifInfo;
   ifInfo = NULL;
}

LONG H_NetIfList(const TCHAR* pszParam, const TCHAR* pArg, StringList* pValue)
{
   int netlinkSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
   if (netlinkSocket == -1)
   {
      AgentWriteDebugLog(4, _T("H_NetIfList: failed to opens socket"));
      return SYSINFO_RC_ERROR;
   }

   setsockopt(netlinkSocket, SOL_SOCKET, SO_REUSEADDR, NULL, 0);

   sockaddr_nl local = {};
   local.nl_family = AF_NETLINK;
   local.nl_pid = GetCurrentProcessId();
   local.nl_groups = 0;

   if (bind(netlinkSocket, (sockaddr*) &local, sizeof(local)) < 0)
   {
      AgentWriteDebugLog(4, _T("H_NetIfList: failed to bind socket"));
      close(netlinkSocket);
      return SYSINFO_RC_ERROR;
   }

   IFINFO* interfaceInfo = GetInterfaceInfo(netlinkSocket);
   close(netlinkSocket);

   if (interfaceInfo == NULL)
   {
      AgentWriteDebugLog(4, _T("H_NetIfList: failed to get interface list"));
      return SYSINFO_RC_ERROR;
   }

   TCHAR infoString[1024];
   IFINFO* curInterface = interfaceInfo;
   while(curInterface != NULL)
   {
      _sntprintf(infoString, 1024, _T("%d %hs/%d %d %hs %hs"),
         curInterface->index,
         curInterface->addr,
         curInterface->mask,
         curInterface->type,
         curInterface->mac,
         curInterface->name);
      pValue->add(infoString);

      curInterface = curInterface->next;

      AgentWriteDebugLog(9, _T("H_NetIfList: got interface (%s)"), infoString);
   }

   FreeInterfaceInfo(interfaceInfo);
   return SYSINFO_RC_SUCCESS;
}

LONG H_NetIfInfoFromIOCTL(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
   char *eptr, szBuffer[256];
   LONG nRet = SYSINFO_RC_SUCCESS;
   struct ifreq ifr;
   int fd;

   if (!AgentGetParameterArgA(pszParam, 1, szBuffer, 256))
      return SYSINFO_RC_UNSUPPORTED;

   fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (fd == -1)
   {
      return 1;
   }

   // Check if we have interface name or index
   ifr.ifr_ifindex = strtol(szBuffer, &eptr, 10);
   if (*eptr == 0)
   {
      // Index passed as argument, convert to name
      if (ioctl(fd, SIOCGIFNAME, &ifr) != 0)
         nRet = SYSINFO_RC_ERROR;
   }
   else
   {
      // Name passed as argument
      strncpy(ifr.ifr_name, szBuffer, IFNAMSIZ);
   }

   // Get interface information
   if (nRet == SYSINFO_RC_SUCCESS)
   {
      switch ((long) pArg)
      {
         case IF_INFO_ADMIN_STATUS:
            if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
            {
               ret_int(pValue, (ifr.ifr_flags & IFF_UP) ? 1 : 2);
            }
            else
            {
               nRet = SYSINFO_RC_ERROR;
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
            }
            else
            {
               nRet = SYSINFO_RC_ERROR;
            }
            break;
         case IF_INFO_DESCRIPTION:
            ret_mbstring(pValue, ifr.ifr_name);
            break;
         default:
            nRet = SYSINFO_RC_UNSUPPORTED;
            break;
      }
   }

   // Cleanup
   close(fd);
   return nRet;
}

static LONG ValueFromLine(char *pszLine, int nPos, TCHAR *pValue)
{
   int i;
   char *eptr, szBuffer[256];
   const char *pszWord;
   DWORD dwValue;
   LONG nRet = SYSINFO_RC_ERROR;

   for(i = 0, pszWord = pszLine; i <= nPos; i++)
      pszWord = ExtractWordA(pszWord, szBuffer);
   dwValue = strtoul(szBuffer, &eptr, 0);
   if (*eptr == 0)
   {
      ret_uint(pValue, dwValue);
      nRet = SYSINFO_RC_SUCCESS;
   }
   return nRet;
}

LONG H_NetIfInfoFromProc(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
   char *ptr, szBuffer[256], szName[IFNAMSIZ];
   LONG nIndex, nRet = SYSINFO_RC_SUCCESS;
   FILE *fp;

   if (!AgentGetParameterArgA(pszParam, 1, szBuffer, 256))
      return SYSINFO_RC_UNSUPPORTED;

// Check if we have interface name or index
   nIndex = strtol(szBuffer, &ptr, 10);
   if (*ptr == 0)
   {
      // Index passed as argument, convert to name
      if (if_indextoname(nIndex, szName) == NULL)
         nRet = SYSINFO_RC_ERROR;
   }
   else
   {
      // Name passed as argument
      strncpy(szName, szBuffer, IFNAMSIZ);
   }

// Get interface information
   if (nRet == SYSINFO_RC_SUCCESS)
   {
      // If name is an alias (i.e. eth0:1), remove alias number
      ptr = strchr(szName, ':');
      if (ptr != NULL)
         *ptr = 0;

      fp = fopen("/proc/net/dev", "r");
      if (fp != NULL)
      {
         while(1)
         {
            char *ret = fgets(szBuffer, 256, fp);
            if (ret == NULL || feof(fp))
            {
               nRet = SYSINFO_RC_ERROR;   // Interface record not found
               break;
            }

            // We expect line in form interface:stats
            StrStripA(szBuffer);
            ptr = strchr(szBuffer, ':');
            if (ptr == NULL)
               continue;
            *ptr = 0;

            if (!stricmp(szBuffer, szName))
            {
               ptr++;
               break;
            }
         }
         fclose(fp);
      }
      else
      {
         nRet = SYSINFO_RC_ERROR;
      }

      if (nRet == SYSINFO_RC_SUCCESS)
      {
         StrStripA(ptr);
         switch ((long) pArg)
         {
            case IF_INFO_BYTES_IN:
               nRet = ValueFromLine(ptr, 0, pValue);
               break;
            case IF_INFO_PACKETS_IN:
               nRet = ValueFromLine(ptr, 1, pValue);
               break;
            case IF_INFO_IN_ERRORS:
               nRet = ValueFromLine(ptr, 2, pValue);
               break;
            case IF_INFO_BYTES_OUT:
               nRet = ValueFromLine(ptr, 8, pValue);
               break;
            case IF_INFO_PACKETS_OUT:
               nRet = ValueFromLine(ptr, 9, pValue);
               break;
            case IF_INFO_OUT_ERRORS:
               nRet = ValueFromLine(ptr, 10, pValue);
               break;
            default:
               nRet = SYSINFO_RC_UNSUPPORTED;
               break;
         }
      }
   }

   return nRet;
}
