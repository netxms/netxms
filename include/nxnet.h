/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxnet.h
**
**/

#ifndef _nxnet_h_
#define _nxnet_h_

#pragma pack(1)

/**
 * IP Header -- RFC 791
 */
struct IPHDR
{
   BYTE m_cVIHL;           // Version and IHL
   BYTE m_cTOS;            // Type Of Service
   uint16_t m_wLen;        // Total Length
   uint16_t m_wId;         // Identification
   uint16_t m_wFlagOff;    // Flags and Fragment Offset
   BYTE m_cTTL;            // Time To Live
   BYTE m_cProtocol;       // Protocol
   uint16_t m_wChecksum;   // Checksum
   struct in_addr m_iaSrc; // Internet Address - Source
   struct in_addr m_iaDst; // Internet Address - Destination
};

/**
 * ICMP Header - RFC 792
 */
struct ICMPHDR
{
   BYTE m_cType;            // Type
   BYTE m_cCode;            // Code
   uint16_t m_wChecksum;    // Checksum
   uint16_t m_wId;          // Identification
   uint16_t m_wSeq;         // Sequence
};

/**
 * Min ping size
 */
#define MIN_PING_SIZE   (sizeof(IPHDR) + sizeof(ICMPHDR))

/**
 * Max ping size
 */
#define MAX_PING_SIZE	8192

/**
 * ICMP echo request structure
 */
struct ICMP_ECHO_REQUEST
{
   ICMPHDR m_icmpHdr;
   BYTE m_data[MAX_PING_SIZE - sizeof(ICMPHDR) - sizeof(IPHDR)];
};

/**
 * ICMP echo reply structure
 */
struct ICMP_ECHO_REPLY
{
   IPHDR m_ipHdr;
   ICMPHDR m_icmpHdr;
   BYTE m_data[MAX_PING_SIZE - sizeof(ICMPHDR) - sizeof(IPHDR)];
};

/**
 * Combined IPv6 + ICMPv6 header for checksum calculation
 */
struct ICMP6_PACKET_HEADER
{
   // IPv6 header
   BYTE srcAddr[16];
   BYTE destAddr[16];
   uint32_t length;
   BYTE padding[3];
   BYTE nextHeader;

   // ICMPv6 header
   BYTE type;
   BYTE code;
   uint16_t checksum;

   // Custom fields
   uint32_t id;
   uint32_t sequence;
   BYTE data[8];      // actual length may differ
};

/**
 * ICMPv6 reply header
 */
struct ICMP6_REPLY
{
   // ICMPv6 header
   BYTE type;
   BYTE code;
   uint16_t checksum;

   // Custom fields
   uint32_t id;
   uint32_t sequence;
};

/**
 * ICMPv6 error report structure
 */
struct ICMP6_ERROR_REPORT
{
   // ICMPv6 header
   BYTE type;
   BYTE code;
   uint16_t checksum;

   // Custom fields
   uint32_t unused;
   BYTE ipv6hdr[8];
   BYTE srcAddr[16];
   BYTE destAddr[16];
};

#pragma pack()

#endif   /* _nxnet_h_ */
