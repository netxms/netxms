/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: main.cpp
**
**/

#include "leef.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("LEEF", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Configuration
 */
InetAddress g_leefServer;
uint16_t g_leefPort = 514;
char g_leefVendor[32] = "Raden Solutions";
char g_leefProduct[32] = "NetXMS";
char g_leefProductVersion[32] = NETXMS_VERSION_STRING_A;
char g_leefHostname[256];
uint32_t g_leefEventCode = 0;
BYTE g_leefSeparatorChar = '^';
bool g_leefRFC5424Timestamp = true;
int g_leefSyslogFacility = 13;
int g_leefSyslogSeverity = 5;
char *g_leefExtraData = nullptr;

/**
 * Get configuration parameter as UTF-8 string
 */
static void GetConfigurationValueAsUTF8String(const Config& config, const TCHAR *name, char *buffer, size_t size)
{
   const TCHAR *v = config.getValue(name);
   if (v != nullptr)
   {
      tchar_to_utf8(v, -1, buffer, size);
      buffer[size - 1] = 0;
   }
}

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   g_leefServer = InetAddress::resolveHostName(config->getValue(_T("/LEEF/Server"), _T("127.0.0.1")));
   g_leefEventCode = config->getValueAsUInt(_T("/LEEF/EventCode"), g_leefEventCode);
   g_leefPort = static_cast<uint16_t>(config->getValueAsInt(_T("/LEEF/Port"), g_leefPort));
   g_leefRFC5424Timestamp = config->getValueAsBoolean(_T("/LEEF/RFC5424Timestamp"), g_leefRFC5424Timestamp);
   g_leefSyslogFacility = config->getValueAsInt(_T("/LEEF/Facility"), g_leefSyslogFacility);
   g_leefSyslogSeverity = config->getValueAsInt(_T("/LEEF/Severity"), g_leefSyslogSeverity);
   GetConfigurationValueAsUTF8String(*config, _T("/LEEF/Product"), g_leefProduct, sizeof(g_leefProduct));
   GetConfigurationValueAsUTF8String(*config, _T("/LEEF/ProductVersion"), g_leefProductVersion, sizeof(g_leefProductVersion));
   GetConfigurationValueAsUTF8String(*config, _T("/LEEF/Vendor"), g_leefVendor, sizeof(g_leefVendor));

   wchar_t buffer[256];
   GetLocalHostName(buffer, 256, true);
   wchar_to_utf8(buffer, -1, g_leefHostname, 256);

   const wchar_t *separatorDefinition = config->getValue(L"/LEEF/Separator");
   if (separatorDefinition != nullptr)
   {
      if (*separatorDefinition == _T('x'))
      {
         BYTE c = static_cast<BYTE>(_tcstoul(separatorDefinition + 1, nullptr, 16));
         if (c != 0)
            g_leefSeparatorChar = c;
         else
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_LEEF, _T("Invalid separator char definition \"%s\""), separatorDefinition);
      }
      else
      {
         g_leefSeparatorChar = static_cast<BYTE>(*separatorDefinition);
      }
   }

   // Setup extra data string
   unique_ptr<ObjectArray<ConfigEntry>> extraDataEntries = config->getSubEntries(_T("/LEEF/ExtraData"));
   if (extraDataEntries != nullptr)
   {
      ByteStream extraDataBuilder;
      for(int i = 0; i < extraDataEntries->size(); i++)
      {
         ConfigEntry *e = extraDataEntries->get(i);
         char *name = UTF8StringFromTString(e->getName());
         char *value = UTF8StringFromTString(e->getValue());
         extraDataBuilder.write(g_leefSeparatorChar);
         extraDataBuilder.write(name, strlen(name));
         extraDataBuilder.write('=');
         extraDataBuilder.write(value, strlen(value));
         MemFree(name);
         MemFree(value);
      }
      if (extraDataBuilder.size() > 0)
      {
         extraDataBuilder.write(static_cast<BYTE>(0));
         g_leefExtraData = MemAllocStringA(extraDataBuilder.size());
         memcpy(g_leefExtraData, extraDataBuilder.buffer(), extraDataBuilder.size());
      }
   }

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_LEEF, _T("LEEF sender configuration: server=%s:%u, vendor=\"%hs\", product=\"%hs\", event=%u, separator=0x%02X"),
            g_leefServer.toString().cstr(), g_leefPort, g_leefVendor, g_leefProduct, g_leefEventCode, g_leefSeparatorChar);
   return true;
}

/**
 * Server start handler
 */
static void OnServerStart()
{
   StartLeefSender();
}

/**
 * Shutdown module
 */
static void ShutdownModule()
{
   StopLeefSender();
   MemFreeAndNull(g_leefExtraData);
   nxlog_debug_tag(DEBUG_TAG_LEEF, 2, L"LEEF module shutdown completed");
}

/**
 * Agent message handler
 */
static bool OnAgentMessage(NXCPMessage *msg, uint32_t nodeId)
{
   if (msg->getCode() != CMD_AUDIT_RECORD)
      return false;

   EnqueueLeefAuditRecord(*msg,  nodeId);
   return true;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"LEEF");
   module->pfInitialize = InitModule;
   module->pfServerStarted = OnServerStart;
   module->pfShutdown = ShutdownModule;
   module->pfProcessAuditRecord = EnqueueLeefAuditRecord;
   module->pfOnAgentMessage = OnAgentMessage;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
