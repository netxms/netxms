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
** $module: libnxsnmp.h
**
**/

#ifndef _libnxsnmp_h_
#define _libnxsnmp_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxsnmp.h>

#ifndef _WIN32
#include <netdb.h>
#endif


//
// Buffer structure for BER_DecodeContent for ASN_OBJECT_ID type
//

typedef struct
{
   DWORD dwLength;
   DWORD *pdwValue;
} SNMP_OID;


//
// Header structure for compiled MIB file
//

typedef struct
{
   char chMagic[6];
   BYTE bHeaderSize;    // Header size in bytes
   BYTE bVersion;
   BYTE bReserved[4];
   DWORD dwTimeStamp;   // Server's timestamp
} SNMP_MIB_HEADER;


//
// MIB file header constants
//

#define MIB_FILE_MAGIC     "NXMIB "
#define MIB_FILE_VERSION   1


//
// Tags for compiled MIB file
//

#define MIB_TAG_OBJECT        0x01
#define MIB_TAG_NAME          0x02
#define MIB_TAG_DESCRIPTION   0x03
#define MIB_TAG_ACCESS        0x04
#define MIB_TAG_STATUS        0x05
#define MIB_TAG_TYPE          0x06
#define MIB_TAG_BYTE_OID      0x07     /* Used if OID < 256 */
#define MIB_TAG_WORD_OID      0x08     /* Used if OID < 65536 */
#define MIB_TAG_DWORD_OID     0x09

#define MIB_END_OF_TAG        0x80


//
// Functions
//

BOOL BER_DecodeIdentifier(BYTE *pRawData, DWORD dwRawSize, DWORD *pdwType, 
                          DWORD *pdwLength, BYTE **pData, DWORD *pdwIdLength);
BOOL BER_DecodeContent(DWORD dwType, BYTE *pData, DWORD dwLength, BYTE *pBuffer);
DWORD BER_Encode(DWORD dwType, BYTE *pData, DWORD dwDataLength, 
                 BYTE *pBuffer, DWORD dwBufferSize);

#endif   /* _libnxsnmp_h_ */
