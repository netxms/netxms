/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: nxcore_syslog.h
**
**/

#ifndef _nxcore_syslog_h_
#define _nxcore_syslog_h_

#include <nxlog.h>

/**
 * Syslog message
 */
class SyslogMessage
{
private:
   uint64_t m_id;       // NetXMS internal message ID
   time_t m_timestamp;
   uint16_t m_facility;
   uint16_t m_severity;
   int32_t m_zoneUIN;
   uint32_t m_nodeId;
   shared_ptr<Node> m_node;
   char m_hostName[MAX_SYSLOG_HOSTNAME_LEN];
   char m_tag[MAX_SYSLOG_TAG_LEN];
   char *m_rawMessage;
   wchar_t *m_message;
   InetAddress m_sourceAddress;
   char *m_rawData;
   size_t m_rawDataLen;

public:
   SyslogMessage(const InetAddress& addr, const char *rawData, size_t rawDataLen) : m_sourceAddress(addr)
   {
      m_id = 0;
      m_rawData = MemCopyBlock(rawData, rawDataLen + 1);
      m_rawDataLen = rawDataLen;
      m_timestamp = time(nullptr);
      m_zoneUIN = 0;
      m_nodeId = 0;
      m_facility = 1;
      m_severity = SYSLOG_SEVERITY_NOTICE;
      m_hostName[0] = 0;
      m_tag[0] = 0;
      m_rawMessage = nullptr;
      m_message = nullptr;
   }

   SyslogMessage(const InetAddress& addr, time_t timestamp, uint32_t zoneUIN, uint32_t nodeId, const char *rawData, int rawDataLen) : m_sourceAddress(addr)
   {
      m_id = 0;
      m_rawData = MemCopyBlock(rawData, rawDataLen + 1);
      m_rawDataLen = rawDataLen;
      m_timestamp = timestamp;
      m_zoneUIN = zoneUIN;
      m_nodeId = nodeId;
      m_facility = 1;
      m_severity = SYSLOG_SEVERITY_NOTICE;
      m_hostName[0] = 0;
      m_tag[0] = 0;
      m_rawMessage = nullptr;
      m_message = nullptr;
   }

   ~SyslogMessage()
   {
      MemFree(m_rawData);
      MemFree(m_message);
   }

   bool parse();
   bool bindToNode();
   void setId(uint64_t id) { m_id = id; }

   void convertRawMessage(const char *codepage)
   {
      size_t len = strlen(m_rawMessage) + 1;
      m_message = MemAllocStringW(len);
      mbcp_to_wchar(m_rawMessage, -1, m_message, len, codepage);
   }

   void fillNXCPMessage(NXCPMessage *msg) const;

   uint64_t getId() const { return m_id; }
   const char *getRawData() const { return m_rawData; }
   const InetAddress& getSourceAddress() const { return m_sourceAddress; }
   time_t getTimestamp() const { return m_timestamp; }
   int32_t getZoneUIN() const { return m_zoneUIN; }
   uint32_t getNodeId() const { return m_nodeId; }
   shared_ptr<Node> getNode() const { return m_node; }
   uint16_t getFacility() const { return m_facility; }
   uint16_t getSeverity() const { return m_severity; }
   const TCHAR *getMessage() const { return m_message; }
   const char *getHostName() const { return m_hostName; }
   const char *getTag() const { return m_tag; }
};

#endif   /* _nxcore_syslog_h_ */

