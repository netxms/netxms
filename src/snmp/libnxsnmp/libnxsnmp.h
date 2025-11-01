/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: libnxsnmp.h
**
**/

#ifndef _libnxsnmp_h_
#define _libnxsnmp_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxsnmp.h>
#include <zlib.h>

#ifndef _WIN32
#include <netdb.h>
#endif

#define LIBNXSNMP_DEBUG_TAG   _T("snmp.lib")

/**
 * Max number of bytes to be allocated on stack instead of heap
 */
#define LIBNXSNMP_MAX_LOCAL_ALLOC    4096

/**
 * Allocate memory on stack or in heap depending on size
 */
#define SNMP_MemAlloc(n) (((n) <= LIBNXSNMP_MAX_LOCAL_ALLOC) ? MemAllocLocal(n) : MemAlloc(n))

/**
 * Free memory previously allocated by SNMP_MemAlloc
 */
#define SNMP_MemFree(p, n) do { if ((n) <= LIBNXSNMP_MAX_LOCAL_ALLOC) MemFreeLocal(p); else MemFree(p); } while(0)

/**
 * Buffer structure for BER_DecodeContent for ASN_OBJECT_ID type
 */
struct SNMP_OID
{
   uint32_t length;
   uint32_t *value;
   uint32_t internalBuffer[SNMP_OID_INTERNAL_BUFFER_SIZE];
};

/**
 * Header structure for compiled MIB file
 */
struct SNMP_MIB_HEADER
{
   char chMagic[6];
   BYTE bHeaderSize;    // Header size in bytes
   BYTE bVersion;
   uint16_t flags;
   BYTE bReserved[2];
   uint32_t dwTimeStamp;   // Server's timestamp
};

/**
 * MIB file header constants
 */
#define MIB_FILE_MAGIC     "NXMIB "
#define MIB_FILE_VERSION   3

/**
 * Tags for compiled MIB file
 */
#define MIB_TAG_OBJECT             0x01
#define MIB_TAG_NAME               0x02
#define MIB_TAG_DESCRIPTION        0x03
#define MIB_TAG_ACCESS             0x04
#define MIB_TAG_STATUS             0x05
#define MIB_TAG_TYPE               0x06
#define MIB_TAG_BYTE_OID           0x07     /* Used if OID < 256 */
#define MIB_TAG_WORD_OID           0x08     /* Used if OID < 65536 */
#define MIB_TAG_UINT32_OID         0x09
#define MIB_TAG_TEXTUAL_CONVENTION 0x0A
#define MIB_TAG_INDEX              0x0B

#define MIB_END_OF_TAG             0x80

/**
 * Class for compressed/uncompressed I/O
 */
class ZFile
{
private:
   BOOL m_bCompress;
   BOOL m_bWrite;
   FILE *m_pFile;
   z_stream m_stream;
   int m_nLastZLibError;
   size_t m_nBufferSize;
   BYTE *m_pDataBuffer;
   BYTE *m_pCompBuffer;
   BYTE *m_pBufferPos;

   int zwrite(const void *buffer, size_t size);
   int zputc(int ch);
   int zread(void *buffer, size_t size);
   int zgetc();
   int zclose();

   BOOL fillDataBuffer();

public:
   ZFile(FILE *pFile, BOOL bCompress, BOOL bWrite);
   ~ZFile();

   size_t write(const void *buffer, size_t size) { return m_bCompress ? zwrite(buffer, size) : fwrite(buffer, 1, size, m_pFile); }
   int writeByte(int ch) { return m_bCompress ? zputc(ch) : fputc(ch, m_pFile); }

   size_t read(void *buffer, size_t size) { return m_bCompress ? zread(buffer, size) : fread(buffer, 1, size, m_pFile); }
   int readByte() { return m_bCompress ? zgetc() : fgetc(m_pFile); }

   int close() { return m_bCompress ? zclose() : fclose(m_pFile); }

   int getZLibError() { return m_nLastZLibError; }
};

/**
 * Functions
 */
bool BER_DecodeIdentifier(const BYTE *rawData, size_t rawSize, uint32_t *type, size_t *length, const BYTE **data, size_t *idLength);
bool BER_DecodeContent(uint32_t type, const BYTE *data, size_t length, BYTE *buffer);
size_t BER_Encode(uint32_t type, const BYTE *data, size_t dataLength, BYTE *buffer, size_t bufferSize);

#endif   /* _libnxsnmp_h_ */
