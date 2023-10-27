/* 
 ** NetXMS subagent for GNU/Linux
 ** Copyright (C) 2004-2023 Raden Solutions
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
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <interface_types.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

/**
 * Handler for Net.IP.Forwarding and Net.IP6.Forwarding parameters
 */
LONG H_NetIpForwarding(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int rc = SYSINFO_RC_ERROR;

   const char *procFileName;
   switch (CAST_FROM_POINTER(arg, int))
   {
      case 4:
         procFileName = "/proc/sys/net/ipv4/conf/all/forwarding";
         break;
      case 6:
         procFileName = "/proc/sys/net/ipv6/conf/all/forwarding";
         break;
      default:
         procFileName = nullptr;
         break;
   }

   if (procFileName != nullptr)
   {
      FILE *hFile = fopen(procFileName, "r");
      if (hFile != nullptr)
      {
         char buffer[4];
         if (fgets(buffer, sizeof(buffer), hFile) != nullptr)
         {
            rc = SYSINFO_RC_SUCCESS;
            switch (buffer[0])
            {
               case '0':
                  ret_boolean(value, false);
                  break;
               case '1':
                  ret_boolean(value, true);
                  break;
               default:
                  rc = SYSINFO_RC_ERROR;
                  break;
            }
         }
         fclose(hFile);
      }
   }

   return rc;
}

/**
 * Handler for Net.ArpCache list
 */
LONG H_NetArpCache(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int rc = SYSINFO_RC_ERROR;
   FILE *hFile = fopen("/proc/net/arp", "r");
   if (hFile != nullptr)
   {
      char szBuff[256];
      if (fgets(szBuff, sizeof(szBuff), hFile) != nullptr) // skip first line
      {
         int nFd = socket(AF_INET, SOCK_DGRAM, 0);
         if (nFd > 0)
         {
            rc = SYSINFO_RC_SUCCESS;

            while(fgets(szBuff, sizeof(szBuff), hFile) != nullptr)
            {
               int nIP1, nIP2, nIP3, nIP4;
               UINT32 nMAC1, nMAC2, nMAC3, nMAC4, nMAC5, nMAC6;
               char szIf[256];

               if (sscanf(szBuff,
                          "%d.%d.%d.%d %*s %*s %02X:%02X:%02X:%02X:%02X:%02X %*s %255s",
                          &nIP1, &nIP2, &nIP3, &nIP4,
                          &nMAC1, &nMAC2, &nMAC3, &nMAC4, &nMAC5, &nMAC6,
                          szIf) == 11)
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

                  strlcpy(irq.ifr_name, szIf, IFNAMSIZ);
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

                  value->add(output);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("H_NetArpCache: cannot parse line \"%hs\""), szBuff);
               }
            }
            close(nFd);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("H_NetArpCache: cannot open socket (%s)"), _tcserror(errno));
         }
      }
      fclose(hFile);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_NetArpCache: cannot open cache file (%s)"), _tcserror(errno));
   }

   return rc;
}

/**
 * Handler for Net.IP.RoutingTable
 */
LONG H_NetRoutingTable(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
   int nRet = SYSINFO_RC_ERROR;

   int nFd = socket(AF_INET, SOCK_DGRAM, 0);
   if (nFd == INVALID_SOCKET)
   {
      return SYSINFO_RC_ERROR;
   }

   FILE *hFile = fopen("/proc/net/route", "r");
   if (hFile == NULL)
   {
      close(nFd);
      return SYSINFO_RC_ERROR;
   }

   char szLine[256];

   if (fgets(szLine, sizeof(szLine), hFile) != nullptr)
   {
      if (!strncmp(szLine, "Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask", 55))
      {
         nRet = SYSINFO_RC_SUCCESS;

         while(fgets(szLine, sizeof(szLine), hFile) != nullptr)
         {
            char szIF[64];
            int nType = 0;
            int metric;
            unsigned int nDestination, nGateway, nMask;

            if (sscanf(szLine,
               "%63s\t%08X\t%08X\t%*d\t%*d\t%*d\t%d\t%08X",
               szIF,
               &nDestination,
               &nGateway,
               &metric,
               &nMask) == 5)
            {
               int nIndex;
               struct ifreq irq;
               strlcpy(irq.ifr_name, szIF, IFNAMSIZ);
               if (ioctl(nFd, SIOCGIFINDEX, &irq) != 0)
               {
                  AgentWriteDebugLog(4, _T("H_NetRoutingTable: ioctl() failed (%s)"), _tcserror(errno));
                  nIndex = 0;
               }
               else
               {
                  nIndex = irq.ifr_ifindex;
               }

               TCHAR output[256], szBuf1[64], szBuf2[64];
               _sntprintf(output, 256, _T("%s/%d %s %d %d %d"),
                  IpToStr(ntohl(nDestination), szBuf1),
                  BitsInMask(htonl(nMask)),
                  IpToStr(ntohl(nGateway), szBuf2),
                  nIndex, nType, metric);
               pValue->add(output);
            }
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_NetRoutingTable: cannot open route file (%s)"), _tcserror(errno));
   }

   close(nFd);
   fclose(hFile);

   return nRet;
}

/**
 * Netlink request
 */
typedef struct
{
   nlmsghdr header;
   rtgenmsg message;
} NETLINK_REQ;

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

   return sendmsg(socket, &message, 0);
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
 * Interface info
 */
struct LinuxInterfaceInfo
{
   int index;
   int type;
   int mtu;
   BYTE macAddr[8];
   char name[IFNAMSIZ];
   char alias[256];
   ObjectArray<InetAddress> addrList;

   LinuxInterfaceInfo() : addrList(16, 16, Ownership::True)
   {
      index = 0;
      type = IFTYPE_OTHER;
      mtu = 0;
      memset(macAddr, 0, sizeof(macAddr));
      name[0] = 0;
      alias[0] = 0;
   }
};

/**
 * Parse interface information (RTM_NEWLINK) message
 */
static LinuxInterfaceInfo *ParseInterfaceMessage(nlmsghdr *messageHeader)
{
   LinuxInterfaceInfo *ifInfo = new LinuxInterfaceInfo();

   auto interface = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(messageHeader));
   int len = messageHeader->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifinfomsg));

   for(struct rtattr *attribute = IFLA_RTA(interface); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
   {
      switch(attribute->rta_type)
      {
         case IFLA_IFNAME:
            strlcpy(ifInfo->name, (char *)RTA_DATA(attribute), sizeof(ifInfo->name));
            break;
         case IFLA_ADDRESS:
            if (RTA_PAYLOAD(attribute) > 0)
            {
               memcpy(ifInfo->macAddr, (BYTE *)RTA_DATA(attribute), std::min(RTA_PAYLOAD(attribute), sizeof(ifInfo->macAddr)));
            }
            break;
         case IFLA_MTU:
            ifInfo->mtu = *((unsigned int *)RTA_DATA(attribute));
            break;
         case IFLA_IFALIAS:
            strlcpy(ifInfo->alias, (char *)RTA_DATA(attribute), sizeof(ifInfo->alias));
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
   LinuxInterfaceInfo *iface = nullptr;
   for(int i = 0; i < ifList->size(); i++)
   {
      if (ifList->get(i)->index == addrMsg->ifa_index)
      {
         iface = ifList->get(i);
         break;
      }
   }
   if (iface == nullptr)
   {
      AgentWriteDebugLog(5, _T("ParseInterfaceMessage: cannot find interface with index %u"), addrMsg->ifa_index);
      return;  // interface not found
   }

   InetAddress *addr = nullptr;
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
               new InetAddress(ntohl(*static_cast<uint32_t*>(RTA_DATA(attribute)))) :
               new InetAddress(static_cast<BYTE*>(RTA_DATA(attribute)));
         if (attribute->rta_type == IFA_LOCAL)
            break;
      }
   }

   if (addr != nullptr)
   {
      addr->setMaskBits(addrMsg->ifa_prefixlen);
      iface->addrList.add(addr);
   }
}

/**
 * Get interface information
 */
static ObjectArray<LinuxInterfaceInfo> *GetInterfaces()
{
   int netlinkSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
   if (netlinkSocket == INVALID_SOCKET)
   {
      AgentWriteDebugLog(4, _T("GetInterfaces: failed to open socket"));
      return NULL;
   }

   int nOne = 1;
   setsockopt(netlinkSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));

   sockaddr_nl local;
   memset(&local, 0, sizeof(local));
   local.nl_family = AF_NETLINK;
   local.nl_pid = getpid();

   bool done;
   ObjectArray<LinuxInterfaceInfo> *ifList = nullptr;

   if (bind(netlinkSocket, (struct sockaddr *)&local, sizeof(local)) == -1)
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

   ifList = new ObjectArray<LinuxInterfaceInfo>(16, 16, Ownership::True);

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

      for(auto mptr = reinterpret_cast<struct nlmsghdr*>(replyBuffer); NLMSG_OK(mptr, msgLen); mptr = NLMSG_NEXT(mptr, msgLen))
      {
         if (mptr->nlmsg_type == RTM_NEWLINK)
         {
            ifList->add(ParseInterfaceMessage(mptr));
         }
         else if (mptr->nlmsg_type == NLMSG_DONE)
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
   return nullptr;
}

/**
 * Handler for Net.InterfaceList list
 */
LONG H_NetIfList(const TCHAR *param, const TCHAR *arg, StringList* value, AbstractCommSession *session)
{
   ObjectArray<LinuxInterfaceInfo> *ifList = GetInterfaces();
   if (ifList == nullptr)
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
            if ((addr->getFamily() == AF_INET) || (session == nullptr) || session->isIPv6Aware())
            {
               _sntprintf(infoString, 1024, _T("%d %s/%d %d(%d) %s %hs"),
                     iface->index,
                     addr->toString(ipAddr),
                     addr->getMaskBits(),
                     iface->type,
                     iface->mtu,
                     BinToStr(iface->macAddr, 6, macAddr),
                     iface->name);
               value->add(infoString);
            }
         }
      }
      else
      {
			_sntprintf(infoString, 1024, _T("%d 0.0.0.0/0 %d(%d) %s %hs"),
               iface->index,
               iface->type,
               iface->mtu,
               BinToStr(iface->macAddr, 6, macAddr),
               iface->name);
         value->add(infoString);
      }
   }

   delete ifList;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.InterfaceNames list
 */
LONG H_NetIfNames(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   ObjectArray<LinuxInterfaceInfo> *ifList = GetInterfaces();
   if (ifList == nullptr)
   {
      AgentWriteDebugLog(4, _T("H_NetIfNames: failed to get interface list"));
      return SYSINFO_RC_ERROR;
   }

   for(int i = 0; i < ifList->size(); i++)
   {
#ifdef UNICODE
      value->addMBString(ifList->get(i)->name);
#else
      value->add(ifList->get(i)->name);
#endif
   }

   delete ifList;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.Interfaces table
 */
LONG H_NetIfTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   ObjectArray<LinuxInterfaceInfo> *ifList = GetInterfaces();
   if (ifList == nullptr)
   {
      AgentWriteDebugLog(4, _T("H_NetIfTable: failed to get interface list"));
      return SYSINFO_RC_ERROR;
   }

   value->addColumn(_T("INDEX"), DCI_DT_UINT, _T("Index"), true);
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
   value->addColumn(_T("ALIAS"), DCI_DT_STRING, _T("Alias"));
   value->addColumn(_T("TYPE"), DCI_DT_UINT, _T("Type"));
   value->addColumn(_T("MTU"), DCI_DT_UINT, _T("MTU"));
   value->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("MAC address"));
   value->addColumn(_T("IP_ADDRESSES"), DCI_DT_STRING, _T("IP addresses"));

   TCHAR macAddr[32];
   for(int i = 0; i < ifList->size(); i++)
   {
      value->addRow();

      LinuxInterfaceInfo *iface = ifList->get(i);
      value->set(0, iface->index);
      value->set(1, iface->name);
      value->set(2, iface->alias);
      value->set(3, iface->type);
      value->set(4, iface->mtu);
      value->set(5, BinToStr(iface->macAddr, 6, macAddr));

      StringBuffer sb;
      for(int j = 0; j < iface->addrList.size(); j++)
      {
         if (j > 0)
            sb.append(_T(", "));
         InetAddress *addr = iface->addrList.get(j);
         sb.append(addr->toString());
         sb.append(_T('/'));
         sb.append(addr->getMaskBits());
      }
      value->set(6, sb);
   }

   delete ifList;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for interface parameters (using ioctl)
 */
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
      strlcpy(ifr.ifr_name, szBuffer, IFNAMSIZ);
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

/**
 * Extract 32 bit value from line
 */
static LONG ValueFromLine(char *line, int pos, TCHAR *value)
{
   char buffer[256];
   const char *curr = line;
   for(int i = 0; i <= pos; i++)
      curr = ExtractWordA(curr, buffer);

   char *eptr;
   // On 64 bit systems interface counters are 64 bit
#if SIZEOF_LONG > 4
   uint32_t n = (uint32_t)(strtoull(buffer, &eptr, 0) & _ULL(0xFFFFFFFF));
#else
   uint32_t n = strtoul(buffer, &eptr, 0);
#endif
   if (*eptr != 0)
      return SYSINFO_RC_ERROR;

   ret_uint(value, n);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Extract 64 bit value from line
 */
static LONG ValueFromLine64(char *line, int pos, TCHAR *value)
{
   char buffer[256];
   const char *curr = line;
   for(int i = 0; i <= pos; i++)
      curr = ExtractWordA(curr, buffer);

   char *eptr;
   uint64_t n = strtoull(buffer, &eptr, 0);
   if (*eptr != 0)
      return SYSINFO_RC_ERROR;

   ret_uint64(value, n);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for interface parameters (using /proc file system)
 */
LONG H_NetIfInfoFromProc(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char *ptr, szBuffer[256], szName[IFNAMSIZ];
   LONG nIndex, nRet = SYSINFO_RC_SUCCESS;

   if (!AgentGetParameterArgA(param, 1, szBuffer, 256))
      return SYSINFO_RC_UNSUPPORTED;

   // Check if we have interface name or index
   nIndex = strtol(szBuffer, &ptr, 10);
   if (*ptr == 0)
   {
      // Index passed as argument, convert to name
      if (if_indextoname(nIndex, szName) == nullptr)
         nRet = SYSINFO_RC_ERROR;
   }
   else
   {
      // Name passed as argument
      strlcpy(szName, szBuffer, IFNAMSIZ);
   }

   // Get interface information
   if (nRet == SYSINFO_RC_SUCCESS)
   {
      // If name is an alias (i.e. eth0:1), remove alias number
      ptr = strchr(szName, ':');
      if (ptr != nullptr)
         *ptr = 0;

      FILE *fp = fopen("/proc/net/dev", "r");
      if (fp != nullptr)
      {
         while(1)
         {
            char *ret = fgets(szBuffer, 256, fp);
            if (ret == nullptr || feof(fp))
            {
               nRet = SYSINFO_RC_ERROR;   // Interface record not found
               break;
            }

            // We expect line in form interface:stats
            TrimA(szBuffer);
            ptr = strchr(szBuffer, ':');
            if (ptr == nullptr)
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
         TrimA(ptr);
         switch ((long) arg)
         {
            case IF_INFO_BYTES_IN:
               nRet = ValueFromLine(ptr, 0, value);
               break;
            case IF_INFO_BYTES_IN_64:
               nRet = ValueFromLine64(ptr, 0, value);
               break;
            case IF_INFO_PACKETS_IN:
               nRet = ValueFromLine(ptr, 1, value);
               break;
            case IF_INFO_PACKETS_IN_64:
               nRet = ValueFromLine64(ptr, 1, value);
               break;
            case IF_INFO_ERRORS_IN:
               nRet = ValueFromLine(ptr, 2, value);
               break;
            case IF_INFO_ERRORS_IN_64:
               nRet = ValueFromLine64(ptr, 2, value);
               break;
            case IF_INFO_BYTES_OUT:
               nRet = ValueFromLine(ptr, 8, value);
               break;
            case IF_INFO_BYTES_OUT_64:
               nRet = ValueFromLine64(ptr, 8, value);
               break;
            case IF_INFO_PACKETS_OUT:
               nRet = ValueFromLine(ptr, 9, value);
               break;
            case IF_INFO_PACKETS_OUT_64:
               nRet = ValueFromLine64(ptr, 9, value);
               break;
            case IF_INFO_ERRORS_OUT:
               nRet = ValueFromLine(ptr, 10, value);
               break;
            case IF_INFO_ERRORS_OUT_64:
               nRet = ValueFromLine64(ptr, 10, value);
               break;
            default:
               nRet = SYSINFO_RC_UNSUPPORTED;
               break;
         }
      }
   }

   return nRet;
}

/**
 * Handler for Net.Interface.Speed(*) parameter
 */
LONG H_NetIfInfoSpeed(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   char buffer[256];
   if (!AgentGetParameterArgA(param, 1, buffer, 256))
      return SYSINFO_RC_UNSUPPORTED;

   // Check if we have interface name or index
   char *eptr, name[IFNAMSIZ];
   uint32_t index = strtol(buffer, &eptr, 10);
   if (*eptr == 0)
   {
      // Index passed as argument, convert to name
      if (if_indextoname(index, name) == nullptr)
         return SYSINFO_RC_ERROR;
   }
   else
   {
      // Name passed as argument
      strlcpy(name, buffer, IFNAMSIZ);
   }

   struct ifreq ifr;
   struct ethtool_cmd edata;
   int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (sock == -1)
      return SYSINFO_RC_ERROR;

   strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
   ifr.ifr_data = reinterpret_cast<caddr_t>(&edata);
   edata.cmd = ETHTOOL_GSET;
   LONG rc;
   if (ioctl(sock, SIOCETHTOOL, &ifr) >= 0)
   {
      ret_uint64(value, edata.speed * _ULL(1000000));
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   close(sock);
   return rc;
}
