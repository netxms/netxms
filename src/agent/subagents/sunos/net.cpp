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
   kstat_ctl_t *kc;
   kstat_t *kp;
   kstat_named_t *kn;
   char *ptr, *eptr, szIfName[IF_NAMESIZE], szDevice[IF_NAMESIZE];
   int nInstance, nIndex;
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
      // Parse interface name and create device name and instance number
      for(ptr = szIfName; (*ptr != 0) && (!isdigit(*ptr)); ptr++);
      memcpy(szDevice, szIfName, ptr - szIfName);
      szDevice[(int)(ptr - szIfName)] = 0;
      for(eptr = ptr; (*eptr != 0) && isdigit(*eptr); eptr++);
      *eptr = 0;
      nInstance = atoi(ptr);

      // Open kstat
      kstat_lock();
      kc = kstat_open();
      if (kc != NULL)
      {
         kp = kstat_lookup(kc, szDevice, nInstance, szIfName);
         if (kp != NULL)
         {
            if(kstat_read(kc, kp, 0) != -1)
            {
               kn = (kstat_named_t *)kstat_data_lookup(kp, (char *)pArg);
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
            }
         }
         kstat_close(kc);
      }
      kstat_unlock();
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
