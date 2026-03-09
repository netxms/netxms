/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004-2014 Victor Kirhenshtein
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

#include "sunos_subagent.h"
#include <net/if.h>
#include <sys/sockio.h>
#include <sys/utsname.h>
#include <stropts.h>
#include <sys/tihdr.h>
#include <inet/mib2.h>

#ifndef LIFC_ALLZONES
#define LIFC_ALLZONES 0
#endif

/**
 * Determine interface type by it's name
 */
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

/**
 * Get interface's hardware address
 */
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

/**
 * Resolve interface index to name. Uses if_indextoname
 * if agent runs in global zone and custom method otherwise.
 */
static BOOL IfIndexToName(int ifIndex, char *ifName)
{
   if (g_flags & SF_GLOBAL_ZONE)
      return if_indextoname(ifIndex, ifName) != NULL;

   char baseIfName[IF_NAMESIZE];
   if (if_indextoname(ifIndex, baseIfName) == NULL)
      return FALSE;
   int baseIfNameLen = (int)strlen(baseIfName);

   struct lifnum ln;
   struct lifconf lc;
   BOOL success = FALSE;

   int nFd = socket(AF_INET, SOCK_DGRAM, 0);
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
            for (int i = 0; i < ln.lifn_count; i++)
            {
               if (!strncmp(baseIfName, lc.lifc_req[i].lifr_name, baseIfNameLen) &&
                   ((lc.lifc_req[i].lifr_name[baseIfNameLen] == ':') || (lc.lifc_req[i].lifr_name[baseIfNameLen] == 0)))
               {
                  strcpy(ifName, lc.lifc_req[i].lifr_name);
                  success = TRUE;
                  break;
               }
            }
         }
         free(lc.lifc_buf);
      }
      close(nFd);
   }
   return success;
}

/**
 * Interface names list
 */
LONG H_NetIfNames(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
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
      ln.lifn_flags = (g_flags & SF_IF_ALL_ZONES) ? LIFC_ALLZONES : 0;
      if (ioctl(nFd, SIOCGLIFNUM, &ln) == 0)
      {
         lc.lifc_family = AF_INET;
         lc.lifc_flags = (g_flags & SF_IF_ALL_ZONES) ? LIFC_ALLZONES : 0;
         lc.lifc_len = sizeof(struct lifreq) * ln.lifn_count;
         lc.lifc_buf = (caddr_t)malloc(lc.lifc_len);
         if (ioctl(nFd, SIOCGLIFCONF, &lc) == 0)
         {
            for (i = 0; i < ln.lifn_count; i++)
            {
               value->addMBString(lc.lifc_req[i].lifr_name);
            }
            nRet = SYSINFO_RC_SUCCESS;
         }
         free(lc.lifc_buf);
      }
      close(nFd);
   }

   return nRet;
}

/**
 * Interface list
 */
LONG H_NetIfList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
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
      ln.lifn_flags = (g_flags & SF_IF_ALL_ZONES) ? LIFC_ALLZONES : 0;
      if (ioctl(nFd, SIOCGLIFNUM, &ln) == 0)
      {
         lc.lifc_family = AF_INET;
         lc.lifc_flags = (g_flags & SF_IF_ALL_ZONES) ? LIFC_ALLZONES : 0;
         lc.lifc_len = sizeof(struct lifreq) * ln.lifn_count;
         lc.lifc_buf = (caddr_t)malloc(lc.lifc_len);
         if (ioctl(nFd, SIOCGLIFCONF, &lc) == 0)
         {
            for (i = 0; i < ln.lifn_count; i++)
            {
               char szIpAddr[32];
               char szMacAddr[32];
               int nMask;

               nRet = SYSINFO_RC_SUCCESS;

               strcpy(rq.lifr_name, lc.lifc_req[i].lifr_name);
               if (ioctl(nFd, SIOCGLIFADDR, &rq) == 0)
               {
                  strlcpy(szIpAddr, inet_ntoa(((struct sockaddr_in *)&rq.lifr_addr)->sin_addr), sizeof(szIpAddr));
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
                  TCHAR szOut[256];
                  _sntprintf(szOut, 256, _T("%d %hs/%d %d %hs %hs"),
                        rq.lifr_index, szIpAddr, nMask,
                        InterfaceTypeFromName(lc.lifc_req[i].lifr_name),
                        szMacAddr, lc.lifc_req[i].lifr_name);
                  pValue->add(szOut);
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

/**
 * Get interface description
 */
LONG H_NetIfDescription(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   char *eptr, szIfName[IF_NAMESIZE];
   int nIndex, nFd;
   struct lifreq rq;
   LONG nRet = SYSINFO_RC_ERROR;

   AgentGetParameterArgA(pszParam, 1, szIfName, IF_NAMESIZE);
   if (szIfName[0] != 0)
   {
      // Determine if parameter is index or name
      nIndex = strtol(szIfName, &eptr, 10);
      if (*eptr == 0)
      {
         // It's an index, determine name
         if (!IfIndexToName(nIndex, szIfName))
         {
            szIfName[0] = 0;
         }
      }
   }

   if (szIfName[0] != 0)
   {
      ret_mbstring(pValue, szIfName);
      nRet = SYSINFO_RC_SUCCESS;
   }

   return nRet;
}

/**
 * Get interface administrative status
 */
LONG H_NetIfAdminStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   char *eptr, szIfName[IF_NAMESIZE];
   int nIndex, nFd;
   struct lifreq rq;
   LONG nRet = SYSINFO_RC_ERROR;

   AgentGetParameterArgA(pszParam, 1, szIfName, IF_NAMESIZE);
   if (szIfName[0] != 0)
   {
      // Determine if parameter is index or name
      nIndex = strtol(szIfName, &eptr, 10);
      if (*eptr == 0)
      {
         // It's an index, determine name
         if (!IfIndexToName(nIndex, szIfName))
         {
            szIfName[0] = 0;
            AgentWriteDebugLog(5, _T("SunOS: H_NetIfAdminStatus: call to IfIndexToName(%d) failed (%s)"), nIndex, _tcserror(errno));
         }
      }
   }

   if (szIfName[0] != 0)
   {
      nFd = socket(AF_INET, SOCK_DGRAM, 0);
      if (nFd >= 0)
      {
         strcpy(rq.lifr_name, szIfName);
         if (ioctl(nFd, SIOCGLIFFLAGS, &rq) == 0)
         {
            ret_int(pValue, (rq.lifr_flags & IFF_UP) ? 1 : 2);
            nRet = SYSINFO_RC_SUCCESS;
         }
         else
         {
            AgentWriteDebugLog(5, _T("SunOS: H_NetIfAdminStatus: call to ioctl() failed (%s)"), _tcserror(errno));
         }
         close(nFd);
      }
      else
      {
         AgentWriteDebugLog(5, _T("SunOS: H_NetIfAdminStatus: call to socket() failed (%s)"), _tcserror(errno));
      }
   }

   return nRet;
}

/**
 * Get interface statistics
 */
LONG H_NetInterfaceStats(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   LONG nRet = SYSINFO_RC_ERROR;

   char ifName[IF_NAMESIZE];
   AgentGetParameterArgA(pszParam, 1, ifName, IF_NAMESIZE);
   if (ifName[0] != 0)
   {
      // Determine if parameter is index or name
      char *eptr;
      int ifIndex = strtol(ifName, &eptr, 10);
      if (*eptr == 0)
      {
         // It's an index, determine name
         if (!IfIndexToName(ifIndex, ifName))
         {
            ifName[0] = 0;
         }
      }
   }

   if (ifName[0] != 0)
   {
      // Parse interface name and create device name and instance number
      char *ptr, *nptr, deviceName[IF_NAMESIZE];
      for(ptr = ifName; (*ptr != 0) && (!isdigit(*ptr)); ptr++);
      memcpy(deviceName, ifName, ptr - ifName);
      deviceName[(int)(ptr - ifName)] = 0;
      for(nptr = ptr; (*nptr != 0) && isdigit(*nptr); nptr++);
      *nptr = 0;
      int deviceNumber = atoi(ptr);

      // Open kstat
      kstat_lock();
      kstat_ctl_t *kc = kstat_open();
      if (kc != NULL)
      {
         static char link[] = "link";
         char *module = ((g_flags & SF_SOLARIS_11) && strcmp(deviceName, "lo")) ? link : deviceName;
         int instance = (g_flags & SF_SOLARIS_11) ? 0 : deviceNumber;
         kstat_t *kp = kstat_lookup(kc, module, instance, ifName);
         if (kp != NULL)
         {
            if (kstat_read(kc, kp, 0) != -1)
            {
               kstat_named_t *kn = (kstat_named_t *)kstat_data_lookup(kp, (char *)pArg);
               if (kn != NULL)
               {
                  switch(kn->data_type)
                  {
                     case KSTAT_DATA_CHAR:
                        ret_mbstring(pValue, kn->value.c);
                        break;
                     case KSTAT_DATA_INT32:
                        ret_int(pValue, kn->value.i32);
                        break;
                     case KSTAT_DATA_UINT32:
                        ret_uint(pValue, kn->value.ui32);
                        break;
                     case KSTAT_DATA_INT64:
                        ret_int64(pValue, kn->value.i64);
                        break;
                     case KSTAT_DATA_UINT64:
                        ret_uint64(pValue, kn->value.ui64);
                        break;
                     case KSTAT_DATA_FLOAT:
                        ret_double(pValue, kn->value.f);
                        break;
                     case KSTAT_DATA_DOUBLE:
                        ret_double(pValue, kn->value.d);
                        break;
                     default:
                        ret_int(pValue, 0);
                        break;
                  }
                  nRet = SYSINFO_RC_SUCCESS;
               }
               else
               {
                  AgentWriteDebugLog(5, _T("SunOS: H_NetInterfaceStats: call to kstat_data_lookup() failed (%s)"), _tcserror(errno));
               }
            }
            else
            {
               AgentWriteDebugLog(5, _T("SunOS: H_NetInterfaceStats: call to kstat_read() failed (%s)"), _tcserror(errno));
            }
         }
         else
         {
            AgentWriteDebugLog(7, _T("SunOS: H_NetInterfaceStats: call to kstat_lookup(%hs, %d, %hs) failed (%s)"),
                  module, instance, ifName, _tcserror(errno));
         }
         kstat_close(kc);
      }
      else
      {
         AgentWriteDebugLog(7, _T("SunOS: H_NetInterfaceStats: call to kstat_open() failed (%s)"), _tcserror(errno));
      }
      kstat_unlock();
   }
   else
   {
      AgentWriteDebugLog(7, _T("SunOS: H_NetInterfaceStats: failed to find interface name"));
   }

   return nRet;
}

/**
 * Get Link status
 */
LONG H_NetInterfaceLink(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   char *eptr, szIfName[IF_NAMESIZE];
   int nIndex, nFd;
   struct lifreq rq;
   LONG nRet = SYSINFO_RC_ERROR;

   // try to get status using kstat()
   nRet = H_NetInterfaceStats(pszParam, (const TCHAR *)"link_up", pValue, NULL);
   if (nRet == SYSINFO_RC_SUCCESS)
   {
      return SYSINFO_RC_SUCCESS;
   }

   AgentGetParameterArgA(pszParam, 1, szIfName, IF_NAMESIZE);
   if (szIfName[0] != 0)
   {
      // Determine if parameter is index or name
      nIndex = strtol(szIfName, &eptr, 10);
      if (*eptr == 0)
      {
         // It's an index, determine name
         if (!IfIndexToName(nIndex, szIfName))
         {
            szIfName[0] = 0;
         }
      }

      if (szIfName[0] != 0)
      {
         nFd = socket(AF_INET, SOCK_DGRAM, 0);
         if (nFd >= 0)
         {
            strcpy(rq.lifr_name, szIfName);
            if (ioctl(nFd, SIOCGLIFFLAGS, &rq) == 0)
            {
               ret_int(pValue, (rq.lifr_flags & IFF_RUNNING) ? 1 : 0);
               nRet = SYSINFO_RC_SUCCESS;
            }
            close(nFd);
         }
      }
   }

   return nRet;
}

/**
 * Parse TCP state name to MIB-II state code
 */
static int TCPStateFromName(const TCHAR *name)
{
   static struct { const TCHAR *name; int code; } stateMap[] =
   {
      { _T("ESTABLISHED"), MIB2_TCP_established },
      { _T("SYN_SENT"), MIB2_TCP_synSent },
      { _T("SYN_RECV"), MIB2_TCP_synReceived },
      { _T("FIN_WAIT1"), MIB2_TCP_finWait1 },
      { _T("FIN_WAIT2"), MIB2_TCP_finWait2 },
      { _T("TIME_WAIT"), MIB2_TCP_timeWait },
      { _T("CLOSE"), MIB2_TCP_closed },
      { _T("CLOSE_WAIT"), MIB2_TCP_closeWait },
      { _T("LAST_ACK"), MIB2_TCP_lastAck },
      { _T("LISTEN"), MIB2_TCP_listen },
      { _T("CLOSING"), MIB2_TCP_closing },
      { nullptr, 0 }
   };
   for (int i = 0; stateMap[i].name != nullptr; i++)
   {
      if (!_tcsicmp(name, stateMap[i].name))
         return stateMap[i].code;
   }
   return -1;
}

/**
 * Count TCP connections in MIB2 response data buffer
 */
static int CountTCPConnectionsInBuffer(const char *data, int dataLen, int entrySize, int stateOffset, int targetState)
{
   int count = 0;
   int nEntries = dataLen / entrySize;
   for (int i = 0; i < nEntries; i++)
   {
      int state = *reinterpret_cast<const int*>(data + (i * entrySize) + stateOffset);
      if (targetState == -1 || state == targetState)
         count++;
   }
   return count;
}

/**
 * Handler for Net.IP.Stats.TCPConnections and Net.IP.Stats.TCPConnections(*)
 * Uses STREAMS MIB2 interface to enumerate TCP connections
 */
LONG H_NetTCPConnections(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int targetState = -1;
   int ipVersion = 0; // 0 = both, 4 = IPv4 only, 6 = IPv6 only

   if (arg[0] == _T('O'))
   {
      OptionList options(param);
      if (!options.isValid())
         return SYSINFO_RC_UNSUPPORTED;

      const TCHAR *state = options.get(_T("state"));
      if (state != nullptr)
      {
         targetState = TCPStateFromName(state);
         if (targetState == -1)
            return SYSINFO_RC_UNSUPPORTED;
      }

      const TCHAR *version = options.get(_T("version"));
      if (version != nullptr)
      {
         ipVersion = _tcstol(version, nullptr, 10);
         if (ipVersion != 4 && ipVersion != 6)
            return SYSINFO_RC_UNSUPPORTED;
      }
   }

   int sd = open("/dev/arp", O_RDWR);
   if (sd < 0)
      return SYSINFO_RC_ERROR;

   // Push tcp module onto the stream
   if (ioctl(sd, I_PUSH, "tcp") < 0)
   {
      close(sd);
      return SYSINFO_RC_ERROR;
   }

   // Request MIB2 TCP data
   struct {
      struct T_optmgmt_req req;
      struct opthdr opt;
   } treq;

   treq.req.PRIM_type = T_SVR4_OPTMGMT_REQ;
   treq.req.OPT_offset = (char *)&treq.opt - (char *)&treq;
   treq.req.OPT_length = sizeof(struct opthdr);
   treq.req.MGMT_flags = T_CURRENT;
   treq.opt.level = MIB2_TCP;
   treq.opt.name = 0;
   treq.opt.len = 0;

   struct strbuf ctlbuf;
   ctlbuf.buf = reinterpret_cast<char*>(&treq);
   ctlbuf.len = sizeof(treq);

   if (putmsg(sd, &ctlbuf, nullptr, 0) < 0)
   {
      close(sd);
      return SYSINFO_RC_ERROR;
   }

   int count = 0;
   bool success = false;

   struct {
      struct T_optmgmt_ack ack;
      struct opthdr opt;
   } tack;

   struct strbuf ctlrsp;
   ctlrsp.buf = reinterpret_cast<char*>(&tack);
   ctlrsp.maxlen = sizeof(tack);

   char *data = static_cast<char*>(MemAlloc(65536));
   struct strbuf datarsp;
   datarsp.buf = data;
   datarsp.maxlen = 65536;

   int flags = 0;
   int rc;
   while ((rc = getmsg(sd, &ctlrsp, &datarsp, &flags)) >= 0)
   {
      if (tack.ack.PRIM_type != T_OPTMGMT_ACK)
         break;

      if (tack.opt.level == MIB2_TCP && datarsp.len > 0)
      {
         // IPv4 TCP connection entries
         if (tack.opt.name == MIB2_TCP_CONN && ipVersion != 6)
         {
            count += CountTCPConnectionsInBuffer(data, datarsp.len,
               sizeof(mib2_tcpConnEntry_t),
               offsetof(mib2_tcpConnEntry_t, tcpConnState),
               targetState);
            success = true;
         }
#ifdef MIB2_TCP6_CONN
         // IPv6 TCP connection entries
         else if (tack.opt.name == MIB2_TCP6_CONN && ipVersion != 4)
         {
            count += CountTCPConnectionsInBuffer(data, datarsp.len,
               sizeof(mib2_tcp6ConnEntry_t),
               offsetof(mib2_tcp6ConnEntry_t, tcp6ConnState),
               targetState);
            success = true;
         }
#endif
      }

      if (rc == 0)
         flags = 0;
   }

   MemFree(data);
   close(sd);

   if (!success)
      return SYSINFO_RC_ERROR;

   ret_int(value, count);
   return SYSINFO_RC_SUCCESS;
}
