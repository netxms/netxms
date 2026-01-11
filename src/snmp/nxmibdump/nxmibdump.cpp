/*
** nxmibdump - MIB file diagnostic tool
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nxmibdump.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <netxms_getopt.h>
#include <nxsnmp.h>

// Include internal header for SNMP_MIB_HEADER structure
#include "../libnxsnmp/libnxsnmp.h"

NETXMS_EXECUTABLE_HEADER(nxmibdump)

/**
 * Print MIB file header
 */
static bool PrintMIBHeader(const TCHAR *fileName)
{
   FILE *fp = _tfopen(fileName, _T("rb"));
   if (fp == nullptr)
   {
      _tprintf(_T("Error: Cannot open file %s\n"), fileName);
      return false;
   }

   SNMP_MIB_HEADER header;
   if (fread(&header, 1, sizeof(SNMP_MIB_HEADER), fp) != sizeof(SNMP_MIB_HEADER))
   {
      _tprintf(_T("Error: Cannot read MIB header\n"));
      fclose(fp);
      return false;
   }
   fclose(fp);

   if (memcmp(header.chMagic, MIB_FILE_MAGIC, 6) != 0)
   {
      _tprintf(_T("Error: Invalid MIB file (bad magic)\n"));
      return false;
   }

   uint16_t flags = ntohs(header.flags);
   uint32_t timestamp = ntohl(header.dwTimeStamp);
   time_t t = static_cast<time_t>(timestamp);
   TCHAR timeStr[64];
   _tcsftime(timeStr, 64, _T("%Y-%m-%d %H:%M:%S"), localtime(&t));

   _tprintf(_T("MIB File: %s\n"), fileName);
   _tprintf(_T("  Version: %d\n"), header.bVersion);
   _tprintf(_T("  Header size: %d bytes\n"), header.bHeaderSize);
   _tprintf(_T("  Timestamp: %s (unix: %u)\n"), timeStr, timestamp);
   _tprintf(_T("  Flags: 0x%04X%s\n"), flags,
            (flags & SMT_COMPRESS_DATA) ? _T(" (compressed)") : _T(""));
   _tprintf(_T("\n"));

   return true;
}

/**
 * Print type name
 */
static const TCHAR *GetTypeName(int type)
{
   switch(type)
   {
      case MIB_TYPE_OTHER: return _T("OTHER");
      case MIB_TYPE_IMPORT_ITEM: return _T("IMPORT_ITEM");
      case MIB_TYPE_OBJID: return _T("OBJECT IDENTIFIER");
      case MIB_TYPE_BITSTRING: return _T("BIT STRING");
      case MIB_TYPE_INTEGER: return _T("INTEGER");
      case MIB_TYPE_INTEGER32: return _T("Integer32");
      case MIB_TYPE_INTEGER64: return _T("Integer64");
      case MIB_TYPE_UNSIGNED32: return _T("Unsigned32");
      case MIB_TYPE_UINTEGER: return _T("UInteger32");
      case MIB_TYPE_COUNTER: return _T("Counter");
      case MIB_TYPE_COUNTER32: return _T("Counter32");
      case MIB_TYPE_COUNTER64: return _T("Counter64");
      case MIB_TYPE_GAUGE: return _T("Gauge");
      case MIB_TYPE_GAUGE32: return _T("Gauge32");
      case MIB_TYPE_TIMETICKS: return _T("TimeTicks");
      case MIB_TYPE_OCTETSTR: return _T("OCTET STRING");
      case MIB_TYPE_OPAQUE: return _T("Opaque");
      case MIB_TYPE_IPADDR: return _T("IpAddress");
      case MIB_TYPE_PHYSADDR: return _T("PhysAddress");
      case MIB_TYPE_NETADDR: return _T("NetworkAddress");
      case MIB_TYPE_NAMED_TYPE: return _T("NAMED_TYPE");
      case MIB_TYPE_SEQID: return _T("SEQUENCE OF");
      case MIB_TYPE_SEQUENCE: return _T("SEQUENCE");
      case MIB_TYPE_CHOICE: return _T("CHOICE");
      case MIB_TYPE_TEXTUAL_CONVENTION: return _T("TEXTUAL-CONVENTION");
      case MIB_TYPE_MACRO_DEFINITION: return _T("MACRO");
      case MIB_TYPE_MODCOMP: return _T("MODULE-COMPLIANCE");
      case MIB_TYPE_TRAPTYPE: return _T("TRAP-TYPE");
      case MIB_TYPE_NOTIFTYPE: return _T("NOTIFICATION-TYPE");
      case MIB_TYPE_MODID: return _T("MODULE-IDENTITY");
      case MIB_TYPE_OBJGROUP: return _T("OBJECT-GROUP");
      case MIB_TYPE_NOTIFGROUP: return _T("NOTIFICATION-GROUP");
      case MIB_TYPE_AGENTCAP: return _T("AGENT-CAPABILITIES");
      case MIB_TYPE_NSAPADDRESS: return _T("NsapAddress");
      case MIB_TYPE_NULL: return _T("NULL");
      default: return _T("UNKNOWN");
   }
}

/**
 * Print access name
 */
static const TCHAR *GetAccessName(int access)
{
   switch(access)
   {
      case MIB_ACCESS_READONLY: return _T("read-only");
      case MIB_ACCESS_READWRITE: return _T("read-write");
      case MIB_ACCESS_WRITEONLY: return _T("write-only");
      case MIB_ACCESS_NOACCESS: return _T("not-accessible");
      case MIB_ACCESS_NOTIFY: return _T("accessible-for-notify");
      case MIB_ACCESS_CREATE: return _T("read-create");
      default: return _T("(none)");
   }
}

/**
 * Print status name
 */
static const TCHAR *GetStatusName(int status)
{
   switch(status)
   {
      case MIB_STATUS_MANDATORY: return _T("mandatory");
      case MIB_STATUS_OPTIONAL: return _T("optional");
      case MIB_STATUS_OBSOLETE: return _T("obsolete");
      case MIB_STATUS_DEPRECATED: return _T("deprecated");
      case MIB_STATUS_CURRENT: return _T("current");
      default: return _T("(none)");
   }
}

/**
 * Build full OID string for an object
 */
static void BuildFullOID(SNMP_MIBObject *obj, TCHAR *buffer, size_t bufferSize)
{
   if (obj->getParent() != nullptr)
   {
      BuildFullOID(obj->getParent(), buffer, bufferSize);
      size_t len = _tcslen(buffer);
      _sntprintf(buffer + len, bufferSize - len, _T(".%u"), obj->getObjectId());
   }
   else
   {
      buffer[0] = 0;
   }
}

/**
 * Print MIB object details
 */
static void PrintMIBObject(SNMP_MIBObject *obj)
{
   TCHAR oidStr[1024];
   BuildFullOID(obj, oidStr, 1024);

   _tprintf(_T("Object: %s\n"), obj->getName() ? obj->getName() : _T("(unnamed)"));
   _tprintf(_T("  OID: %s\n"), oidStr);
   _tprintf(_T("  Type: %s (%d)\n"), GetTypeName(obj->getType()), obj->getType());
   _tprintf(_T("  Access: %s (%d)\n"), GetAccessName(obj->getAccess()), obj->getAccess());
   _tprintf(_T("  Status: %s (%d)\n"), GetStatusName(obj->getStatus()), obj->getStatus());
   _tprintf(_T("  Textual Convention: %s\n"),
            obj->getTextualConvention() ? obj->getTextualConvention() : _T("(none)"));
   _tprintf(_T("  Display Hint: %s\n"),
            obj->getDisplayHint() ? obj->getDisplayHint() : _T("(none)"));
   _tprintf(_T("  Index: %s\n"),
            obj->getIndex() ? obj->getIndex() : _T("(none)"));

   if (obj->getDescription() != nullptr)
   {
      _tprintf(_T("  Description: %.200s%s\n"), obj->getDescription(),
               _tcslen(obj->getDescription()) > 200 ? _T("...") : _T(""));
   }
   else
   {
      _tprintf(_T("  Description: (none)\n"));
   }
}

/**
 * Find object by name (recursive search)
 */
static SNMP_MIBObject *FindObjectByName(SNMP_MIBObject *root, const TCHAR *name)
{
   if (root->getName() != nullptr && !_tcsicmp(root->getName(), name))
      return root;

   for (SNMP_MIBObject *child = root->getFirstChild(); child != nullptr; child = child->getNext())
   {
      SNMP_MIBObject *found = FindObjectByName(child, name);
      if (found != nullptr)
         return found;
   }
   return nullptr;
}

/**
 * Main entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   int ch;
   bool showHelp = false;
   opterr = 1;

   while ((ch = getopt(argc, argv, "h")) != -1)
   {
      switch (ch)
      {
         case 'h':
            showHelp = true;
            break;
         case '?':
            return 1;
      }
   }

   if (showHelp || argc - optind < 1)
   {
      _tprintf(_T("Usage: nxmibdump [options] <mib-file> [<oid-or-name>]\n")
               _T("\n")
               _T("Options:\n")
               _T("   -h            : Display this help and exit\n")
               _T("\n")
               _T("If OID or object name is provided, prints details of that object.\n")
               _T("Otherwise, prints MIB file header information only.\n")
               _T("\n"));
      return showHelp ? 0 : 1;
   }

#ifdef UNICODE
   WCHAR *mibFile = WideStringFromMBStringSysLocale(argv[optind]);
#else
   const char *mibFile = argv[optind];
#endif

   // Print header
   if (!PrintMIBHeader(mibFile))
   {
#ifdef UNICODE
      MemFree(mibFile);
#endif
      return 2;
   }

   // If OID/name specified, look it up
   if (argc - optind >= 2)
   {
#ifdef UNICODE
      WCHAR *oidArg = WideStringFromMBStringSysLocale(argv[optind + 1]);
#else
      const char *oidArg = argv[optind + 1];
#endif

      SNMP_MIBObject *root = nullptr;
      uint32_t rc = SnmpLoadMIBTree(mibFile, &root);
      if (rc != SNMP_ERR_SUCCESS)
      {
         _tprintf(_T("Error: Cannot load MIB tree (%s)\n"), SnmpGetErrorText(rc));
#ifdef UNICODE
         MemFree(mibFile);
         MemFree(oidArg);
#endif
         return 3;
      }

      SNMP_MIBObject *obj = nullptr;

      // Check if it's an OID (starts with . or digit)
      if (oidArg[0] == _T('.') || _istdigit(oidArg[0]))
      {
         SNMP_ObjectId oid = SNMP_ObjectId::parse(oidArg);
         if (oid.isValid())
         {
            obj = SnmpFindMIBObjectByOID(root, oid);
         }
      }

      // If not found as OID, try as name
      if (obj == nullptr)
      {
         obj = FindObjectByName(root, oidArg);
      }

      if (obj != nullptr)
      {
         PrintMIBObject(obj);
      }
      else
      {
         _tprintf(_T("Object not found: %s\n"), oidArg);
      }

      delete root;
#ifdef UNICODE
      MemFree(oidArg);
#endif
   }

#ifdef UNICODE
   MemFree(mibFile);
#endif
   return 0;
}
