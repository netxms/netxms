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


//
// Buffer structure for BER_DecodeContent for ASN_OBJECT_ID type
//

typedef struct
{
   DWORD dwLength;
   DWORD *pdwValue;
} SNMP_OID;


//
// Functions
//

BOOL BER_DecodeIdentifier(BYTE *pRawData, DWORD dwRawSize, DWORD *pdwType, 
                          DWORD *pdwLength, BYTE **pData, DWORD *pdwIdLength);
BOOL BER_DecodeContent(DWORD dwType, BYTE *pData, DWORD dwLength, BYTE *pBuffer);

#endif   /* _libnxsnmp_h_ */
