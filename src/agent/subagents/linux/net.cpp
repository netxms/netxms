/* 
 ** NetXMS subagent for GNU/Linux
 ** Copyright (C) 2004-2015 Raden Solutions
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
#include <net/if_arp.h>

LONG H_NetIpForwarding(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

/**
 * Handler for Net.ArpCache list
 */
LONG H_NetArpCache(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
   int nRet = SYSINFO_RC_ERROR;
   FILE *hFile;

   hFile = fopen("/proc/net/arp", "r");
   if (hFile != NULL)
   {
      char szBuff[256];
      if (fgets(szBuff, sizeof(szBuff), hFile) != NULL) // skip first line
      {
         int nFd = socket(AF_INET, SOCK_DGRAM, 0);
         if (nFd > 0)
         {
            nRet = SYSINFO_RC_SUCCESS;

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
      }
      fclose(hFile);
   }

   return nRet;
}

/**
 * Handler for Net.IP.RoutingTable
 */
LONG H_NetRoutingTable(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
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

/**
 * Send netlink message
 */
static int SendMessage(int socket, unsigned short type)
{
   sockaddr_nl kernel = {};
   msghdr message = {};
   iovec io;
   NETLINK_REQ request = {};

   kernel.nl_family = AF_NETLINK;

   request.header.nlmsg_len = NLMSG_LENGTH(sizeof(rtgenmsg));
   request.header.nlmsg_type = type;
   request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
   request.header.nlmsg_seq = 1;
   request.header.nlmsg_pid = getpid();
   //request.message.rtgen_family = AF_PACKET;
   request.message.rtgen_family = AF_UNSPEC;

   io.iov_base = &request;
   io.iov_len = request.header.nlmsg_len;
   message.msg_iov = &io;
   message.msg_iovlen = 1;
   message.msg_name = &kernel;
   message.msg_namelen = sizeof(kernel);

   return sendmsg(socket, (msghdr*) &message, 0);
}

/**
 * Receive netlink message
 */
static int ReceiveMessage(int socket, char *replyBuffer, size_t replyBufferSize)
{
   iovec io;
   msghdr reply = {};
   sockaddr_nl kernel;

   io.iov_base = replyBuffer;
   io.iov_len = replyBufferSize;
   kernel.nl_family = AF_NETLINK;
   reply.msg_iov = &io;
   reply.msg_iovlen = 1;
   reply.msg_name = &kernel;
   reply.msg_namelen = sizeof(kernel);

   return recvmsg(socket, &reply, 0);
}

/**
 * Interface type conversion table
 */
static struct
{
   int netlinkType;
   int snmpType;
} s_ifTypeConversion[] =
{
   { ARPHRD_ARCNET, IFTYPE_ARCNET },
   { ARPHRD_ATM, IFTYPE_ATM },
   { ARPHRD_DLCI, IFTYPE_FRDLCIENDPT },
   { ARPHRD_ETHER, IFTYPE_ETHERNET_CSMACD },
   { ARPHRD_EETHER, IFTYPE_ETHERNET_CSMACD },
   { ARPHRD_FDDI, IFTYPE_FDDI },
   { ARPHRD_IEEE1394, IFTYPE_IEEE1394 },
   { ARPHRD_IEEE80211, IFTYPE_IEEE80211 },
   { ARPHRD_IEEE80211_PRISM, IFTYPE_IEEE80211 },
   { ARPHRD_IEEE80211_RADIOTAP, IFTYPE_IEEE80211 },
   { ARPHRD_INFINIBAND, IFTYPE_INFINIBAND },
   { ARPHRD_LOOPBACK, IFTYPE_SOFTWARE_LOOPBACK },
   { ARPHRD_PPP, IFTYPE_PPP },
   { ARPHRD_SLIP, IFTYPE_SLIP },
   { ARPHRD_TUNNEL, IFTYPE_TUNNEL },
   { ARPHRD_TUNNEL6, IFTYPE_TUNNEL },
   { -1, -1 }
};

/**
 * Parse interface information (RTM_NEWLINK) message
 */
static LinuxInterfaceInfo *ParseInterfaceMessage(nlmsghdr *messageHeader)
{
   LinuxInterfaceInfo *ifInfo = new LinuxInterfaceInfo();

   struct ifinfomsg *interface = (ifinfomsg *)NLMSG_DATA(messageHeader);
   int len = messageHeader->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifinfomsg));

   for(struct rtattr *attribute = IFLA_RTA(interface); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
   {
      switch(attribute->rta_type)
      {
         case IFLA_IFNAME:
            strncpy(ifInfo->name, (char *)RTA_DATA(attribute), sizeof(ifInfo->name));
            break;
         case IFLA_ADDRESS:
            memcpy(ifInfo->macAddr, (BYTE *)RTA_DATA(attribute), 6);
            break;
         case IFLA_MTU:
            ifInfo->mtu = *((unsigned int *)RTA_DATA(attribute));
            break;
      }
   }

   ifInfo->index = interface->ifi_index;

   ifInfo->type = IFTYPE_OTHER;
   for(int i = 0; s_ifTypeConversion[i].netlinkType != -1; i++)
   {
      if (interface->ifi_type == s_ifTypeConversion[i].netlinkType)
      {
         ifInfo->type = s_ifTypeConversion[i].snmpType;
         break;
      }
   }
   return ifInfo;
}

/**
 * Parse interface address (RTM_NEWADDR) message
 */
static void ParseAddressMessage(nlmsghdr *messageHeader, ObjectArray<LinuxInterfaceInfo> *ifList)
{
   struct ifaddrmsg *addrMsg = (struct ifaddrmsg *)NLMSG_DATA(messageHeader);
   if ((addrMsg->ifa_family != AF_INET) && (addrMsg->ifa_family != AF_INET6))
      return;  // protocol not supported

   // Find interface by index
   LinuxInterfaceInfo *iface = NULL;
   for(int i = 0; i < ifList->size(); i++)
   {
      if (ifList->get(i)->index == addrMsg->ifa_index)
      {
         iface = ifList->get(i);
         break;
      }
   }
   if (iface == NULL)
   {
      AgentWriteDebugLog(5, _T("ParseInterfaceMessage: cannot find interface with index %d"), addrMsg->ifa_index);
      return;  // interface not found
   }

   //int len = messageHeader->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifaddrmsg));
   InetAddress *addr = NULL;
   int len = IFA_PAYLOAD(messageHeader);
   for(struct rtattr *attribute = IFA_RTA(addrMsg); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
   {
      // Address may be given as IFA_ADDRESS or IFA_LOCAL
      // We prefer IFA_LOCAL because it should be local address
      // for point-to-point interfaces. For normal broadcast interfaces
      // only IFA_ADDRESS may be set.
      if ((attribute->rta_type == IFA_LOCAL) || (attribute->rta_type == IFA_ADDRESS))
      {
         delete addr;  // if it was created from IFA_ADDRESS
         addr = (addrMsg->ifa_family == AF_INET) ? 
            new InetAddress(ntohl(*((UINT32 *)RTA_DATA(attribute)))) :
            new InetAddress((BYTE *)RTA_DATA(attribute));
         if (attribute->rta_type == IFA_LOCAL)
            break;
      }
   }
   
   if (addr != NULL)
   {
      addr->setMaskBits(addrMsg->ifa_prefixlen);
      iface->addrList.add(addr);
   }
}

/**
 * Get interfce information
 */
static ObjectArray<LinuxInterfaceInfo> *GetInterfaces()
{
   int netlinkSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
   if (netlinkSocket == -1)
   {
      AgentWriteDebugLog(4, _T("GetInterfaces: failed to opens socket"));
      return NULL;
   }

   setsockopt(netlinkSocket, SOL_SOCKET, SO_REUSEADDR, NULL, 0);

   sockaddr_nl local = {};
   local.nl_family = AF_NETLINK;
   local.nl_pid = getpid();
   local.nl_groups = 0;

   bool done;
   ObjectArray<LinuxInterfaceInfo> *ifList = NULL;

   if (bind(netlinkSocket, (struct sockaddr *)&local, sizeof(local)) < 0)
   {
      AgentWriteDebugLog(4, _T("GetInterfaces: failed to bind socket"));
      goto failure_1;
   }

   // Send request to read interface list
   if (SendMessage(netlinkSocket, RTM_GETLINK) == -1)
   {
      AgentWriteDebugLog(4, _T("GetInterfaces: SendMessage(RTM_GETLINK) failed (%s)"), _tcserror(errno));
      goto failure_1;
   }

   ifList = new ObjectArray<LinuxInterfaceInfo>(16, 16, true);

   // Read and parse interface list
   done = false;
   while(!done)
   {
      char replyBuffer[8192];
      int msgLen = ReceiveMessage(netlinkSocket, replyBuffer, sizeof(replyBuffer));
      if (msgLen <= 0)
      {
         AgentWriteDebugLog(4, _T("GetInterfaces: ReceiveMessage failed (%s)"), _tcserror(errno));
         goto failure_2;
      }

      for(struct nlmsghdr *msg_ptr = (struct nlmsghdr *)replyBuffer; NLMSG_OK(msg_ptr, msgLen); msg_ptr = NLMSG_NEXT(msg_ptr, msgLen))
      {
         if (msg_ptr->nlmsg_type == RTM_NEWLINK)
         {
            ifList->add(ParseInterfaceMessage(msg_ptr));
         }
         else if (msg_ptr->nlmsg_type == NLMSG_DONE)
         {
            done = true;
         }
      }
   }

   // Send request to read IP address list
   if (SendMessage(netlinkSocket, RTM_GETADDR) == -1)
   {
      AgentWriteDebugLog(4, _T("GetInterfaces: SendMessage(RTM_GETADDR) failed (%s)"), _tcserror(errno));
      goto failure_2;
   }

   // Read and parse address list
   done = false;
   while(!done)
   {
      char replyBuffer[8192];
      int msgLen = ReceiveMessage(netlinkSocket, replyBuffer, sizeof(replyBuffer));
      if (msgLen <= 0)
      {
         AgentWriteDebugLog(4, _T("GetInterfaces: ReceiveMessage failed (%s)"), _tcserror(errno));
         goto failure_2;
      }

      for(struct nlmsghdr *msg_ptr = (struct nlmsghdr *)replyBuffer; NLMSG_OK(msg_ptr, msgLen); msg_ptr = NLMSG_NEXT(msg_ptr, msgLen))
      {
         if (msg_ptr->nlmsg_type == RTM_NEWADDR)
         {
            ParseAddressMessage(msg_ptr, ifList);
         }
         else if (msg_ptr->nlmsg_type == NLMSG_DONE)
         {
            done = true;
         }
      }
   }

   close(netlinkSocket);
   return ifList;

failure_2:
   delete ifList;

failure_1:
   close(netlinkSocket);
   return NULL;
}

/**
 * Handler for Net.InterfaceList list
 */
LONG H_NetIfList(const TCHAR* pszParam, const TCHAR* pArg, StringList* pValue, AbstractCommSession *session)
{
   ObjectArray<LinuxInterfaceInfo> *ifList = GetInterfaces();
   if (ifList == NULL)
   {
      AgentWriteDebugLog(4, _T("H_NetIfList: failed to get interface list"));
      return SYSINFO_RC_ERROR;
   }

   TCHAR infoString[1024], macAddr[32], ipAddr[64];
   for(int i = 0; i < ifList->size(); i++)
   {
      LinuxInterfaceInfo *iface = ifList->get(i);
      if (iface->addrList.size() > 0)
      {
         for(int j = 0; j < iface->addrList.size(); j++)
         {
            const InetAddress *addr = iface->addrList.get(j);
            if ((addr->getFamily() == AF_INET) || session->isIPv6Aware())
            {
               _sntprintf(infoString, 1024, _T("%d %s/%d %d %s %hs"),
                  iface->index,
                  addr->toString(ipAddr),
                  addr->getMaskBits(),
                  iface->type,
                  MACToStr(iface->macAddr, macAddr),
                  iface->name);
               pValue->add(infoString);
            }
         }
      }
      else
      {
			_sntprintf(infoString, 1024, _T("%d 0.0.0.0/0 %d %s %hs"),
				iface->index,
				iface->type,
				MACToStr(iface->macAddr, macAddr),
				iface->name);
         pValue->add(infoString);
      }
   }

   delete ifList;
   return SYSINFO_RC_SUCCESS;
}

LONG H_NetIfInfoFromIOCTL(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

LONG H_NetIfInfoFromProc(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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
