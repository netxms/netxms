/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: parse.cpp
**
**/

#include "libnxsnmp.h"


//
// Parse TRAP PDU
//

static BOOL ParseTrapPDU(SNMP_PDU *pdu, BYTE *pData, DWORD dwPDULength)
{
   DWORD dwType, dwLength;
   BYTE *pbCurrPos = pData;

   // Enterprise ID
   if (!DecodeBER(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos))
      return FALSE;
   if (dwType != ASN_OBJECT_ID)
      return FALSE;
   pdu->pEnterprise = (SNMP_OID *)malloc(sizeof(SNMP_OID));
   memset(pdu->pEnterprise, 0, sizeof(SNMP_OID));
   if (!DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)pdu->pEnterprise))
      return FALSE;
   dwPDULength -= dwLength;
   pbCurrPos += dwLength;

   return TRUE;
}


//
// Parse PDU
//

SNMP_PDU LIBNXSNMP_EXPORTABLE *SNMPParsePDU(BYTE *pRawData, DWORD dwRawSize)
{
   BYTE *pbCurrPos;
   DWORD dwType, dwLength, dwPacketLength, dwVersion;
   SNMP_PDU *pdu = NULL;

   // Packet start
   if (!DecodeBER(pRawData, dwRawSize, &dwType, &dwPacketLength, &pbCurrPos))
      return NULL;
   if (dwType != ASN_SEQUENCE)
      return NULL;   // Packet should start with SEQUENCE

   // Version
   if (!DecodeBER(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos))
      return NULL;
   if (dwType != ASN_INTEGER)
      return NULL;   // Version field should be of integer type
   if (!DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&dwVersion))
      return NULL;   // Error parsing content of version field
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength;
   if ((dwVersion != SNMP_VERSION_1) && (dwVersion != SNMP_VERSION_2C))
      return NULL;   // Unsupported SNMP version

   // Community string
   if (!DecodeBER(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos))
      return NULL;
   if (dwType != ASN_OCTET_STRING)
      return NULL;   // Community field should be of string type
   pdu = (SNMP_PDU *)malloc(sizeof(SNMP_PDU));
   memset(pdu, 0, sizeof(SNMP_PDU));
   pdu->pszCommunity = (char *)malloc(dwLength + 1);
   if (!DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)pdu->pszCommunity))
   {
      SNMPDestroyPDU(pdu);
      return NULL;   // Error parsing content of version field
   }
   pdu->pszCommunity[dwLength] = 0;
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength;

   if (!DecodeBER(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos))
      goto parse_error;
   switch(dwType)
   {
      case ASN_TRAP_V1_PDU:
         pdu->iCommand = SNMP_TRAP;
         if (!ParseTrapPDU(pdu, pbCurrPos, dwLength))
            goto parse_error;
         break;
      default:
         goto parse_error;
   }

   // Success
   pdu->iVersion = dwVersion;
   return pdu;

parse_error:
   SNMPDestroyPDU(pdu);
   return NULL;
}
