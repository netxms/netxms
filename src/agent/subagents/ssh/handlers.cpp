/*
** NetXMS SSH subagent
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
** File: handlers.cpp
**
**/

#include "ssh_subagent.h"

/**
 * Generic handler to execute command on any host
 */
LONG H_SSHCommand(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR hostName[256], login[64], password[64], command[256];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64) ||
       !AgentGetParameterArg(param, 4, command, 256))
      return SYSINFO_RC_UNSUPPORTED;

   UINT16 port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != NULL)
   {
      *p = 0;
      p++;
      port = (UINT16)_tcstoul(p, NULL, 10);
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_ERROR;
   SSHSession *ssh = AcquireSession(addr, port, login, password);
   if (ssh != NULL)
   {
      StringList *output = ssh->execute(command);
      if (output != NULL)
      {
         if (output->size() > 0)
         {
            ret_string(value, output->get(0));
            rc = SYSINFO_RC_SUCCESS;
         }
         delete output;
      }
      ReleaseSession(ssh);
   }
   return rc;
}

/**
 * Generic handler to execute command on any host (arguments as list
 */
LONG H_SSHCommandList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR hostName[256], login[64], password[64], command[256];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64) ||
       !AgentGetParameterArg(param, 4, command, 256))
      return SYSINFO_RC_UNSUPPORTED;

   UINT16 port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != NULL)
   {
      *p = 0;
      p++;
      port = (UINT16)_tcstoul(p, NULL, 10);
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_ERROR;
   SSHSession *ssh = AcquireSession(addr, port, login, password);
   if (ssh != NULL)
   {
      StringList *output = ssh->execute(command);
      if (output != NULL)
      {
         value->addAll(output);
         rc = SYSINFO_RC_SUCCESS;
         delete output;
      }
      ReleaseSession(ssh);
   }
   return rc;
}
