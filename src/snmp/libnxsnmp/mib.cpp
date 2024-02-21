/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: mib.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Default constructor for SNMP_MIBObject
 */
SNMP_MIBObject::SNMP_MIBObject()
{
   initialize();

   m_dwOID = 0;
   m_pszName = nullptr;
   m_pszDescription = nullptr;
	m_pszTextualConvention = nullptr;
	m_index = nullptr;
   m_iStatus = -1;
   m_iAccess = -1;
   m_iType = -1;
}

/**
 * Construct object with all data
 */
SNMP_MIBObject::SNMP_MIBObject(uint32_t oid, const TCHAR *name, int type,
      int status, int access, const TCHAR *description, const TCHAR *textualConvention, const TCHAR *index)
{
   initialize();

   m_dwOID = oid;
   m_pszName = MemCopyString(name);
   m_pszDescription = MemCopyString(description);
   m_pszTextualConvention = MemCopyString(textualConvention);
   m_index = MemCopyString(index);
   m_iStatus = status;
   m_iAccess = access;
   m_iType = type;
}

/**
 * Construct object with only ID and name
 */
SNMP_MIBObject::SNMP_MIBObject(uint32_t oid, const TCHAR *name)
{
   initialize();

   m_dwOID = oid;
   m_pszName = MemCopyString(name);
   m_pszDescription = nullptr;
	m_pszTextualConvention = nullptr;
	m_index = nullptr;
   m_iStatus = -1;
   m_iAccess = -1;
   m_iType = -1;
}

/**
 * Common initialization
 */
void SNMP_MIBObject::initialize()
{
   m_pParent = nullptr;
   m_pNext = nullptr;
   m_pPrev = nullptr;
   m_pFirst = nullptr;
   m_pLast = nullptr;
}

/**
 * Destructor
 */
SNMP_MIBObject::~SNMP_MIBObject()
{
   SNMP_MIBObject *pCurr, *pNext;
   for(pCurr = m_pFirst; pCurr != nullptr; pCurr = pNext)
   {
      pNext = pCurr->getNext();
      delete pCurr;
   }
   MemFree(m_pszName);
   MemFree(m_pszDescription);
	MemFree(m_pszTextualConvention);
	MemFree(m_index);
}

/**
 * Add child object
 */
void SNMP_MIBObject::addChild(SNMP_MIBObject *pObject)
{
   if (m_pLast == nullptr)
   {
      m_pFirst = m_pLast = pObject;
   }
   else
   {
      m_pLast->m_pNext = pObject;
      pObject->m_pPrev = m_pLast;
      pObject->m_pNext = nullptr;
      m_pLast = pObject;
   }
   pObject->setParent(this);
}

/**
 * Find child object by OID
 */
SNMP_MIBObject *SNMP_MIBObject::findChildByID(uint32_t oid)
{
   for(SNMP_MIBObject *curr = m_pFirst; curr != nullptr; curr = curr->getNext())
      if (curr->m_dwOID == oid)
         return curr;
   return nullptr;
}

/**
 * Set information
 */
void SNMP_MIBObject::setInfo(int type, int status, int access, const TCHAR *description, const TCHAR *textualConvention, const TCHAR *index)
{
   MemFree(m_pszDescription);
	MemFree(m_pszTextualConvention);
	MemFree(m_index);
   m_iType = type;
   m_iStatus = status;
   m_iAccess = access;
   m_pszDescription = MemCopyString(description);
	m_pszTextualConvention = MemCopyString(textualConvention);
   m_index = MemCopyString(index);
}

/**
 * Print MIB subtree
 */
void SNMP_MIBObject::print(int nIndent) const
{
   if ((nIndent == 0) && (m_pszName == nullptr) && (m_dwOID == 0))
      _tprintf(_T("[root]\n"));
   else
      _tprintf(_T("%*s%s(%d)\n"), nIndent, _T(""), m_pszName, m_dwOID);

   for(SNMP_MIBObject *curr = m_pFirst; curr != nullptr; curr = curr->getNext())
      curr->print(nIndent + 2);
}

/**
 * Write string to file
 */
static void WriteStringToFile(ZFile *file, const TCHAR *str)
{
#ifdef UNICODE
   size_t len = wchar_utf8len(str, wcslen(str));
#else
   size_t len = strlen(str);
#endif
   uint16_t nlen = htons(static_cast<uint16_t>(len));
   file->write(&nlen, 2);
#ifdef UNICODE
   char *utf8str = MemAllocStringA(len + 1);
	wchar_to_utf8(str, -1, utf8str, len + 1);
   file->write(utf8str, len);
   MemFree(utf8str);
#else
   file->write(str, len);
#endif
}

/**
 * Write object to file
 */
void SNMP_MIBObject::writeToFile(ZFile *file, uint32_t flags)
{
   SNMP_MIBObject *pCurr;

   file->writeByte(MIB_TAG_OBJECT);

   // Object name
   file->writeByte(MIB_TAG_NAME);
   WriteStringToFile(file, CHECK_NULL_EX(m_pszName));
   file->writeByte(MIB_TAG_NAME | MIB_END_OF_TAG);

   // Object ID
   if (m_dwOID < 256)
   {
      file->writeByte(MIB_TAG_BYTE_OID);
      file->writeByte((int)m_dwOID);
      file->writeByte(MIB_TAG_BYTE_OID | MIB_END_OF_TAG);
   }
   else if (m_dwOID < 65536)
   {
      file->writeByte(MIB_TAG_WORD_OID);
      uint16_t temp = htons((WORD)m_dwOID);
      file->write(&temp, 2);
      file->writeByte(MIB_TAG_WORD_OID | MIB_END_OF_TAG);
   }
   else
   {
      file->writeByte(MIB_TAG_UINT32_OID);
      uint32_t temp = htonl(m_dwOID);
      file->write(&temp, 4);
      file->writeByte(MIB_TAG_UINT32_OID | MIB_END_OF_TAG);
   }

   // Object status
   file->writeByte(MIB_TAG_STATUS);
   file->writeByte(m_iStatus);
   file->writeByte(MIB_TAG_STATUS | MIB_END_OF_TAG);

   // Object access
   file->writeByte(MIB_TAG_ACCESS);
   file->writeByte(m_iAccess);
   file->writeByte(MIB_TAG_ACCESS | MIB_END_OF_TAG);

   // Object type
   file->writeByte(MIB_TAG_TYPE);
   file->writeByte(m_iType);
   file->writeByte(MIB_TAG_TYPE | MIB_END_OF_TAG);

   // Object description
   if (!(flags & SMT_SKIP_DESCRIPTIONS))
   {
      file->writeByte(MIB_TAG_DESCRIPTION);
      WriteStringToFile(file, CHECK_NULL_EX(m_pszDescription));
      file->writeByte(MIB_TAG_DESCRIPTION | MIB_END_OF_TAG);

		if (m_pszTextualConvention != nullptr)
		{
			file->writeByte(MIB_TAG_TEXTUAL_CONVENTION);
			WriteStringToFile(file, m_pszTextualConvention);
			file->writeByte(MIB_TAG_TEXTUAL_CONVENTION | MIB_END_OF_TAG);
		}
   }

   if (m_index != nullptr)
   {
      file->writeByte(MIB_TAG_INDEX);
      WriteStringToFile(file, m_index);
      file->writeByte(MIB_TAG_INDEX | MIB_END_OF_TAG);
   }

   // Save children
   for(pCurr = m_pFirst; pCurr != nullptr; pCurr = pCurr->getNext())
      pCurr->writeToFile(file, flags);

   file->writeByte(MIB_TAG_OBJECT | MIB_END_OF_TAG);
}

/**
 * Save MIB tree to file
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpSaveMIBTree(const TCHAR *fileName, SNMP_MIBObject *root, uint32_t flags)
{
   FILE *file = _tfopen(fileName, _T("wb"));
   if (file == nullptr)
      return SNMP_ERR_FILE_IO;

   SNMP_MIB_HEADER header;
   memcpy(header.chMagic, MIB_FILE_MAGIC, 6);
   header.bVersion = MIB_FILE_VERSION;
   header.bHeaderSize = sizeof(SNMP_MIB_HEADER);
   header.flags = htons((WORD)flags);
   header.dwTimeStamp = htonl((UINT32)time(nullptr));
   memset(header.bReserved, 0, sizeof(header.bReserved));
   fwrite(&header, sizeof(SNMP_MIB_HEADER), 1, file);
   ZFile zfile(file, flags & SMT_COMPRESS_DATA, true);
   root->writeToFile(&zfile, flags);
   zfile.close();
   return SNMP_ERR_SUCCESS;
}

/**
 * Read string from file
 */
static TCHAR *ReadStringFromFile(ZFile *file)
{
   TCHAR *pszStr;
   uint16_t wLen;
#ifdef UNICODE
   char *pszBuffer;
#endif

   file->read(&wLen, 2);
   wLen = ntohs(wLen);
   if (wLen > 0)
   {
      pszStr = MemAllocString(wLen + 1);
#ifdef UNICODE
      pszBuffer = MemAllocStringA(wLen + 1);
      file->read(pszBuffer, wLen);
      utf8_to_wchar(pszBuffer, wLen, pszStr, wLen + 1);
      MemFree(pszBuffer);
#else
      file->read(pszStr, wLen);
#endif
      pszStr[wLen] = 0;
   }
   else
   {
      pszStr = nullptr;
   }
   return pszStr;
}

/**
 * Read object from file
 */
#define CHECK_NEXT_TAG(x) { ch = file->readByte(); if (ch != (x)) nState--; }

bool SNMP_MIBObject::readFromFile(ZFile *file)
{
   int ch, nState = 0;
   uint16_t wTmp;
   uint32_t dwTmp;
   SNMP_MIBObject *pObject;

   while(nState == 0)
   {
      ch = file->readByte();
      switch(ch)
      {
         case (MIB_TAG_OBJECT | MIB_END_OF_TAG):
            nState++;
            break;
         case MIB_TAG_BYTE_OID:
            m_dwOID = (UINT32)file->readByte();
            CHECK_NEXT_TAG(MIB_TAG_BYTE_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_WORD_OID:
            file->read(&wTmp, 2);
            m_dwOID = (UINT32)ntohs(wTmp);
            CHECK_NEXT_TAG(MIB_TAG_WORD_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_UINT32_OID:
            file->read(&dwTmp, 4);
            m_dwOID = ntohl(dwTmp);
            CHECK_NEXT_TAG(MIB_TAG_UINT32_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_NAME:
            MemFree(m_pszName);
            m_pszName = ReadStringFromFile(file);
            CHECK_NEXT_TAG(MIB_TAG_NAME | MIB_END_OF_TAG);
            break;
         case MIB_TAG_DESCRIPTION:
            MemFree(m_pszDescription);
            m_pszDescription = ReadStringFromFile(file);
            CHECK_NEXT_TAG(MIB_TAG_DESCRIPTION | MIB_END_OF_TAG);
            break;
			case MIB_TAG_TEXTUAL_CONVENTION:
				MemFree(m_pszTextualConvention);
            m_pszTextualConvention = ReadStringFromFile(file);
            CHECK_NEXT_TAG(MIB_TAG_TEXTUAL_CONVENTION | MIB_END_OF_TAG);
            break;
         case MIB_TAG_INDEX:
            MemFree(m_index);
            m_index = ReadStringFromFile(file);
            CHECK_NEXT_TAG(MIB_TAG_INDEX | MIB_END_OF_TAG);
            break;
         case MIB_TAG_TYPE:
            m_iType = file->readByte();
            CHECK_NEXT_TAG(MIB_TAG_TYPE | MIB_END_OF_TAG);
            break;
         case MIB_TAG_STATUS:
            m_iStatus = file->readByte();
            CHECK_NEXT_TAG(MIB_TAG_STATUS | MIB_END_OF_TAG);
            break;
         case MIB_TAG_ACCESS:
            m_iAccess = file->readByte();
            CHECK_NEXT_TAG(MIB_TAG_ACCESS | MIB_END_OF_TAG);
            break;
         case MIB_TAG_OBJECT:
            pObject = new SNMP_MIBObject;
            if (pObject->readFromFile(file))
            {
               addChild(pObject);
            }
            else
            {
               delete pObject;
               nState--;   // Stop reading
            }
            break;
         default:
            nState--;   // error
            break;
      }
   }
   return nState == 1;
}

/**
 * Load MIB tree from file
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpLoadMIBTree(const TCHAR *pszFile, SNMP_MIBObject **ppRoot)
{
   uint32_t rc = SNMP_ERR_SUCCESS;
   FILE *fp = _tfopen(pszFile, _T("rb"));
   if (fp != nullptr)
   {
      SNMP_MIB_HEADER header;
      if (fread(&header, 1, sizeof(SNMP_MIB_HEADER), fp) == sizeof(SNMP_MIB_HEADER))
      {
         if (!memcmp(header.chMagic, MIB_FILE_MAGIC, 6))
         {
            header.flags = ntohs(header.flags);
            fseek(fp, header.bHeaderSize, SEEK_SET);
            ZFile *pZFile = new ZFile(fp, header.flags & SMT_COMPRESS_DATA, FALSE);
            if (pZFile->readByte() == MIB_TAG_OBJECT)
            {
               *ppRoot = new SNMP_MIBObject;
               if (!(*ppRoot)->readFromFile(pZFile))
               {
                  delete *ppRoot;
                  rc = SNMP_ERR_BAD_FILE_DATA;
               }
            }
            else
            {
               rc = SNMP_ERR_BAD_FILE_DATA;
            }
            pZFile->close();
            delete pZFile;
         }
         else
         {
            rc = SNMP_ERR_BAD_FILE_HEADER;
            fclose(fp);
         }
      }
      else
      {
         rc = SNMP_ERR_BAD_FILE_HEADER;
         fclose(fp);
      }
   }
   else
   {
      rc = SNMP_ERR_FILE_IO;
   }
   return rc;
}

/**
 * Get timestamp from saved MIB tree
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpGetMIBTreeTimestamp(const TCHAR *pszFile, uint32_t *timestamp)
{
   FILE *fp = _tfopen(pszFile, _T("rb"));
   if (fp == nullptr)
      return SNMP_ERR_FILE_IO;

   uint32_t rc = SNMP_ERR_SUCCESS;
   SNMP_MIB_HEADER header;
   if (fread(&header, 1, sizeof(SNMP_MIB_HEADER), fp) == sizeof(SNMP_MIB_HEADER))
   {
      if (!memcmp(header.chMagic, MIB_FILE_MAGIC, 6))
      {
         *timestamp = ntohl(header.dwTimeStamp);
      }
      else
      {
         rc = SNMP_ERR_BAD_FILE_HEADER;
      }
   }
   else
   {
      rc = SNMP_ERR_BAD_FILE_HEADER;
   }
   fclose(fp);
   return rc;
}
