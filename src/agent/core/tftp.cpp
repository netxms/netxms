/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: tftp.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("tftp")

/**
 * Transfer local file to remote system via TFTP (handler for TFTP.Put action)
 * Arguments: remote_system local_file [remote_file [port]]
 */
uint32_t H_TFTPPut(const shared_ptr<ActionExecutionContext>& context)
{
   if (!(g_dwFlags & AF_ENABLE_TFTP_PROXY))
      return ERR_ACCESS_DENIED;

   if (context->getArgCount() < 2)
      return ERR_BAD_ARGUMENTS;

   uint16_t port = static_cast<uint16_t>((context->getArgCount() > 3) ? _tcstoul(context->getArg(3), nullptr, 10) : 69);
   if (port == 0)
      return ERR_BAD_ARGUMENTS;

   InetAddress addr = InetAddress::resolveHostName(context->getArg(0));
   if (!addr.isValid() && !addr.isLoopback())
      return ERR_BAD_ARGUMENTS;

   TFTPError rc = TFTPWrite(context->getArg(1), context->getArg(2), addr, port);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("TFTP transfer to %s:%u of file \"%s\": %s"), addr.toString().cstr(), port, context->getArg(1), TFTPErrorMessage(rc));
   switch(rc)
   {
      case TFTP_SUCCESS:
         return ERR_SUCCESS;
      case TFTP_SOCKET_ERROR:
         return ERR_SOCKET_ERROR;
      case TFTP_FILE_ALREADY_EXISTS:
         return ERR_FILE_ALREADY_EXISTS;
      case TFTP_FILE_READ_ERROR:
      case TFTP_PROTOCOL_ERROR:
      case TFTP_TIMEOUT:
      case TFTP_DISK_FULL:
         return ERR_IO_FAILURE;
      default:
         return ERR_INTERNAL_ERROR;
   }
}

/**
 * Read file from TFTP server and present it as string list line by line
 * Arguments: remote_system, remote_file, [port]
 */
LONG H_TFTPGet(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   if (!(g_dwFlags & AF_ENABLE_TFTP_PROXY))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR hostName[256] = _T(""), fileName[256] = _T(""), portText[64] = _T("");
   if (!AgentGetParameterArg(cmd, 1, hostName, MAX_PATH) || !AgentGetParameterArg(cmd, 2, fileName, 256) || !AgentGetParameterArg(cmd, 3, portText, 64))
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t port = static_cast<uint16_t>((portText[0] != 0) ? _tcstoul(portText, nullptr, 10) : 69);
   if ((hostName[0] == 0) || (fileName[0] == 0) || (port == 0))
      return SYSINFO_RC_UNSUPPORTED;

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValid() && !addr.isLoopback())
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   ByteStream bs;
   TFTPError rc = TFTPRead(&bs, fileName, addr, port);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("TFTP transfer of file %s from %s:%u: %s"), fileName, addr.toString().cstr(), port, TFTPErrorMessage(rc));
   if (rc != TFTP_SUCCESS)
      return SYSINFO_RC_ERROR;

   bs.write('\0');
   char *curr = (char*)bs.buffer();
   while(true)
   {
      char *next = strchr(curr, '\n');
      if (next == nullptr)
      {
         if (strlen(curr) != 0)
            value->addUTF8String(curr);
         break;
      }
      *next = 0;
      if ((next > curr) && (*(next - 1) == '\r'))
         *(next - 1) = 0;
      value->addUTF8String(curr);
      curr = next + 1;
   }
   return SYSINFO_RC_SUCCESS;
}
