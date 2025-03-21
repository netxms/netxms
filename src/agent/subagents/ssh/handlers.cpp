/*
** NetXMS SSH subagent
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
#include <netxms-regex.h>

/**
 * SSH connectivity check handler
 */
LONG H_SSHConnection(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      ret_boolean(value, false);
      return SYSINFO_RC_SUCCESS;
   }

   shared_ptr<KeyPair> keys;
   TCHAR keyId[16] = _T("");
   AgentGetParameterArg(param, 4, keyId, 16);
   if (keyId[0] != 0)
   {
      TCHAR* end;
      uint32_t id = _tcstoul(keyId, &end, 0);
      keys = GetSshKey(session, id);
   }

   SSHSession* ssh = AcquireSession(addr, port, login, password, keys);
   if (ssh != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("SSH connection to %s:%u created successfully"), hostName, port);
      ret_boolean(value, true);
      ReleaseSession(ssh);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to create SSH connection to %s:%u"), hostName, port);
      ret_boolean(value, false);
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Generic handler to execute command on any host
 */
LONG H_SSHCommand(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   size_t commandBufferLen = _tcslen(param);
   TCHAR *command = MemAllocString(commandBufferLen);
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64) ||
       !AgentGetParameterArg(param, 4, command, commandBufferLen))
   {
      MemFree(command);
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      MemFree(command);
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   shared_ptr<KeyPair> keys;
   TCHAR keyId[16] = _T("");
   AgentGetParameterArg(param, 6, keyId, 16);
   if (keyId[0] != 0)
   {
      TCHAR *end;
      uint32_t id = _tcstoul(keyId, &end, 0);
      keys = GetSshKey(session, id);
   }

   LONG rc = SYSINFO_RC_ERROR;
   SSHSession *ssh = AcquireSession(addr, port, login, password, keys);
   if (ssh != nullptr)
   {
      StringList *output = ssh->execute(command);
      if (output != nullptr)
      {
         if (output->size() > 0)
         {
            TCHAR pattern[256] = _T("");
            AgentGetParameterArg(param, 5, pattern, 256);
            if (pattern[0] != 0)
            {
               bool match = false;
               const char *errptr;
               int erroffset;
               PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &errptr, &erroffset, nullptr);
               if (preg != nullptr)
               {
                  int ovector[60];
                  for(int i = 0; i < output->size(); i++)
                  {
                     const TCHAR *line = output->get(i);
                     int cgcount = _pcre_exec_t(preg, NULL, reinterpret_cast<const PCRE_TCHAR*>(line), static_cast<int>(_tcslen(line)), 0, 0, ovector, 60);
                     if (cgcount >= 0) // MATCH
                     {
                        match = true;
                        if ((cgcount > 1) && (ovector[2] != -1))
                        {
                           int len = ovector[3] - ovector[2];
                           _tcslcpy(value, &line[ovector[2]], std::min(MAX_RESULT_LENGTH, len + 1));
                        }
                        else
                        {
                           ret_string(value, line);
                        }
                        break;
                     }
                  }
                  _pcre_free_t(preg);
               }
               if (!match)
                  ret_string(value, _T(""));
            }
            else
            {
               ret_string(value, output->get(0));
            }
            rc = SYSINFO_RC_SUCCESS;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH output is empty"));
         }
         delete output;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH command execution on %s failed"), hostName);
      }
      ReleaseSession(ssh);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to create SSH connection to %s:%u"), hostName, port);
   }
   MemFree(command);
   return rc;
}

/**
 * Generic handler to execute command on any host (arguments as list
 */
LONG H_SSHCommandList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   size_t commandBufferLen = _tcslen(param);
   TCHAR *command = MemAllocString(commandBufferLen);
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64) ||
       !AgentGetParameterArg(param, 4, command, commandBufferLen))
   {
      MemFree(command);
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      MemFree(command);
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   shared_ptr<KeyPair> key;
   TCHAR keyId[16] = _T("");
   AgentGetParameterArg(param, 6, keyId, 16);
   if (keyId[0] != 0)
   {
      TCHAR *end;
      uint32_t id = _tcstoul(keyId, &end, 0);
      key = GetSshKey(session, id);
   }

   LONG rc = SYSINFO_RC_ERROR;
   SSHSession *ssh = AcquireSession(addr, port, login, password, key);
   if (ssh != nullptr)
   {
      StringList *output = ssh->execute(command);
      if (output != nullptr)
      {
         value->addAll(output);
         rc = SYSINFO_RC_SUCCESS;
         delete output;
      }
      ReleaseSession(ssh);
   }
   MemFree(command);
   return rc;
}

/**
 * Generic handler to execute command on host by ssh
 */
uint32_t H_SSHCommandAction(const shared_ptr<ActionExecutionContext>& context)
{
	if (context->getArgCount() < 6)
      return ERR_MALFORMED_COMMAND;

   InetAddress addr = InetAddress::resolveHostName(context->getArg(0));
   if (!addr.isValidUnicast())
      return ERR_BAD_ARGUMENTS;

   uint16_t port = (uint16_t)_tcstoul(context->getArg(1), nullptr, 10);
   uint32_t id = _tcstoul(context->getArg(5), nullptr, 10);
   shared_ptr<KeyPair> key = GetSshKey(context->getSession().get(), id);
   uint32_t rc = ERR_EXEC_FAILED;
   SSHSession *ssh = AcquireSession(addr, port, context->getArg(2), context->getArg(3), key);
   if (ssh != nullptr)
   {
      if (ssh->execute(context->getArg(4), context))
      {
         rc = ERR_SUCCESS;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH command execution on %s successful"), context->getArg(0));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH command execution on %s failed"), context->getArg(0));
      }
      context->sendEndOfOutputMarker();
      ReleaseSession(ssh);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to create SSH connection to %s:%u"), context->getArg(0), port);
   }
   return rc;
}
