/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
   Initialize();

   m_dwOID = 0;
   m_pszName = NULL;
   m_pszDescription = NULL;
	m_pszTextualConvention = NULL;
   m_iStatus = -1;
   m_iAccess = -1;
   m_iType = -1;
}

/**
 * Construct object with all data
 */
SNMP_MIBObject::SNMP_MIBObject(UINT32 dwOID, const TCHAR *pszName, int iType, 
                               int iStatus, int iAccess, const TCHAR *pszDescription,
										 const TCHAR *pszTextualConvention)
{
   Initialize();

   m_dwOID = dwOID;
   m_pszName = (pszName != NULL) ? _tcsdup(pszName) : NULL;
   m_pszDescription = (pszDescription != NULL) ? _tcsdup(pszDescription) : NULL;
   m_pszTextualConvention = (pszTextualConvention != NULL) ? _tcsdup(pszTextualConvention) : NULL;
   m_iStatus = iStatus;
   m_iAccess = iAccess;
   m_iType = iType;
}

/**
 * Construct object with only ID and name
 */
SNMP_MIBObject::SNMP_MIBObject(UINT32 dwOID, const TCHAR *pszName)
{
   Initialize();

   m_dwOID = dwOID;
   m_pszName = (pszName != NULL) ? _tcsdup(pszName) : NULL;
   m_pszDescription = NULL;
	m_pszTextualConvention = NULL;
   m_iStatus = -1;
   m_iAccess = -1;
   m_iType = -1;
}

/**
 * Common initialization
 */
void SNMP_MIBObject::Initialize()
{
   m_pParent = NULL;
   m_pNext = NULL;
   m_pPrev = NULL;
   m_pFirst = NULL;
   m_pLast = NULL;
}

/**
 * Destructor
 */
SNMP_MIBObject::~SNMP_MIBObject()
{
   SNMP_MIBObject *pCurr, *pNext;

   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pNext)
   {
      pNext = pCurr->getNext();
      delete pCurr;
   }
   safe_free(m_pszName);
   safe_free(m_pszDescription);
	safe_free(m_pszTextualConvention);
}

/**
 * Add child object
 */
void SNMP_MIBObject::addChild(SNMP_MIBObject *pObject)
{
   if (m_pLast == NULL)
   {
      m_pFirst = m_pLast = pObject;
   }
   else
   {
      m_pLast->m_pNext = pObject;
      pObject->m_pPrev = m_pLast;
      pObject->m_pNext = NULL;
      m_pLast = pObject;
   }
   pObject->setParent(this);
}

/**
 * Find child object by OID
 */
SNMP_MIBObject *SNMP_MIBObject::findChildByID(UINT32 dwOID)
{
   SNMP_MIBObject *pCurr;

   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pCurr->getNext())
      if (pCurr->m_dwOID == dwOID)
         return pCurr;
   return NULL;
}

/**
 * Set information
 */
void SNMP_MIBObject::setInfo(int iType, int iStatus, int iAccess, const TCHAR *pszDescription, const TCHAR *pszTextualConvention)
{
   safe_free(m_pszDescription);
	safe_free(m_pszTextualConvention);
   m_iType = iType;
   m_iStatus = iStatus;
   m_iAccess = iAccess;
   m_pszDescription = (pszDescription != NULL) ? _tcsdup(pszDescription) : NULL;
	m_pszTextualConvention = (pszTextualConvention != NULL) ? _tcsdup(pszTextualConvention) : NULL;
}

/**
 * Print MIB subtree
 */
void SNMP_MIBObject::print(int nIndent)
{
   SNMP_MIBObject *pCurr;

   if ((nIndent == 0) && (m_pszName == NULL) && (m_dwOID == 0))
      _tprintf(_T("[root]\n"));
   else
      _tprintf(_T("%*s%s(%d)\n"), nIndent, _T(""), m_pszName, m_dwOID);

   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pCurr->getNext())
      pCurr->print(nIndent + 2);
}

/**
 * Write string to file
 */
static void WriteStringToFile(ZFile *pFile, const TCHAR *pszStr)
{
   WORD wLen, wTemp;
#ifdef UNICODE
   char *pszBuffer;
#endif

   wLen = (WORD)_tcslen(pszStr);
   wTemp = htons(wLen);
   pFile->write(&wTemp, 2);
#ifdef UNICODE
   pszBuffer = (char *)malloc(wLen + 1);
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
		                 pszStr, -1, pszBuffer, wLen + 1, NULL, NULL);
   pFile->write(pszBuffer, wLen);
   free(pszBuffer);
#else
   pFile->write(pszStr, wLen);
#endif
}

/**
 * Write object to file
 */
void SNMP_MIBObject::writeToFile(ZFile *pFile, UINT32 dwFlags)
{
   SNMP_MIBObject *pCurr;

   pFile->fputc(MIB_TAG_OBJECT);

   // Object name
   pFile->fputc(MIB_TAG_NAME);
   WriteStringToFile(pFile, CHECK_NULL_EX(m_pszName));
   pFile->fputc(MIB_TAG_NAME | MIB_END_OF_TAG);

   // Object ID
   if (m_dwOID < 256)
   {
      pFile->fputc(MIB_TAG_BYTE_OID);
      pFile->fputc((int)m_dwOID);
      pFile->fputc(MIB_TAG_BYTE_OID | MIB_END_OF_TAG);
   }
   else if (m_dwOID < 65536)
   {
      WORD wTemp;

      pFile->fputc(MIB_TAG_WORD_OID);
      wTemp = htons((WORD)m_dwOID);
      pFile->write(&wTemp, 2);
      pFile->fputc(MIB_TAG_WORD_OID | MIB_END_OF_TAG);
   }
   else
   {
      UINT32 dwTemp;

      pFile->fputc(MIB_TAG_UINT32_OID);
      dwTemp = htonl(m_dwOID);
      pFile->write(&dwTemp, 4);
      pFile->fputc(MIB_TAG_UINT32_OID | MIB_END_OF_TAG);
   }

   // Object status
   pFile->fputc(MIB_TAG_STATUS);
   pFile->fputc(m_iStatus);
   pFile->fputc(MIB_TAG_STATUS | MIB_END_OF_TAG);

   // Object access
   pFile->fputc(MIB_TAG_ACCESS);
   pFile->fputc(m_iAccess);
   pFile->fputc(MIB_TAG_ACCESS | MIB_END_OF_TAG);

   // Object type
   pFile->fputc(MIB_TAG_TYPE);
   pFile->fputc(m_iType);
   pFile->fputc(MIB_TAG_TYPE | MIB_END_OF_TAG);

   // Object description
   if (!(dwFlags & SMT_SKIP_DESCRIPTIONS))
   {
      pFile->fputc(MIB_TAG_DESCRIPTION);
      WriteStringToFile(pFile, CHECK_NULL_EX(m_pszDescription));
      pFile->fputc(MIB_TAG_DESCRIPTION | MIB_END_OF_TAG);

		if (m_pszTextualConvention != NULL)
		{
			pFile->fputc(MIB_TAG_TEXTUAL_CONVENTION);
			WriteStringToFile(pFile, m_pszTextualConvention);
			pFile->fputc(MIB_TAG_TEXTUAL_CONVENTION | MIB_END_OF_TAG);
		}
   }

   // Save childs
   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pCurr->getNext())
      pCurr->writeToFile(pFile, dwFlags);

   pFile->fputc(MIB_TAG_OBJECT | MIB_END_OF_TAG);
}

/**
 * Save MIB tree to file
 */
UINT32 LIBNXSNMP_EXPORTABLE SNMPSaveMIBTree(const TCHAR *pszFile, SNMP_MIBObject *pRoot, UINT32 dwFlags)
{
   FILE *pFile;
   ZFile *pZFile;
   SNMP_MIB_HEADER header;
   UINT32 dwRet = SNMP_ERR_SUCCESS;

   pFile = _tfopen(pszFile, _T("wb"));
   if (pFile != NULL)
   {
      memcpy(header.chMagic, MIB_FILE_MAGIC, 6);
      header.bVersion = MIB_FILE_VERSION;
      header.bHeaderSize = sizeof(SNMP_MIB_HEADER);
      header.flags = htons((WORD)dwFlags);
      header.dwTimeStamp = htonl((UINT32)time(NULL));
      memset(header.bReserved, 0, sizeof(header.bReserved));
      fwrite(&header, sizeof(SNMP_MIB_HEADER), 1, pFile);
      pZFile = new ZFile(pFile, dwFlags & SMT_COMPRESS_DATA, TRUE);
      pRoot->writeToFile(pZFile, dwFlags);
      pZFile->close();
      delete pZFile;
   }
   else
   {
      dwRet = SNMP_ERR_FILE_IO;
   }
   return dwRet;
}

/**
 * Read string from file
 */
static TCHAR *ReadStringFromFile(ZFile *pFile)
{
   TCHAR *pszStr;
   WORD wLen;
#ifdef UNICODE
   char *pszBuffer;
#endif

   pFile->read(&wLen, 2);
   wLen = ntohs(wLen);
   if (wLen > 0)
   {
      pszStr = (TCHAR *)malloc(sizeof(TCHAR) * (wLen + 1));
#ifdef UNICODE
      pszBuffer = (char *)malloc(wLen + 1);
      pFile->read(pszBuffer, wLen);
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszBuffer, wLen, pszStr, wLen + 1);
      free(pszBuffer);
#else
      pFile->read(pszStr, wLen);
#endif
      pszStr[wLen] = 0;
   }
   else
   {
      pszStr = NULL;
   }
   return pszStr;
}

/**
 * Read object from file
 */
#define CHECK_NEXT_TAG(x) { ch = pFile->fgetc(); if (ch != (x)) nState--; }

BOOL SNMP_MIBObject::readFromFile(ZFile *pFile)
{
   int ch, nState = 0;
   WORD wTmp;
   UINT32 dwTmp;
   SNMP_MIBObject *pObject;

   while(nState == 0)
   {
      ch = pFile->fgetc();
      switch(ch)
      {
         case (MIB_TAG_OBJECT | MIB_END_OF_TAG):
            nState++;
            break;
         case MIB_TAG_BYTE_OID:
            m_dwOID = (UINT32)pFile->fgetc();
            CHECK_NEXT_TAG(MIB_TAG_BYTE_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_WORD_OID:
            pFile->read(&wTmp, 2);
            m_dwOID = (UINT32)ntohs(wTmp);
            CHECK_NEXT_TAG(MIB_TAG_WORD_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_UINT32_OID:
            pFile->read(&dwTmp, 4);
            m_dwOID = ntohl(dwTmp);
            CHECK_NEXT_TAG(MIB_TAG_UINT32_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_NAME:
            safe_free(m_pszName);
            m_pszName = ReadStringFromFile(pFile);
            CHECK_NEXT_TAG(MIB_TAG_NAME | MIB_END_OF_TAG);
            break;
         case MIB_TAG_DESCRIPTION:
            safe_free(m_pszDescription);
            m_pszDescription = ReadStringFromFile(pFile);
            CHECK_NEXT_TAG(MIB_TAG_DESCRIPTION | MIB_END_OF_TAG);
            break;
			case MIB_TAG_TEXTUAL_CONVENTION:
				safe_free(m_pszTextualConvention);
            m_pszTextualConvention = ReadStringFromFile(pFile);
            CHECK_NEXT_TAG(MIB_TAG_TEXTUAL_CONVENTION | MIB_END_OF_TAG);
            break;
         case MIB_TAG_TYPE:
            m_iType = pFile->fgetc();
            CHECK_NEXT_TAG(MIB_TAG_TYPE | MIB_END_OF_TAG);
            break;
         case MIB_TAG_STATUS:
            m_iStatus = pFile->fgetc();
            CHECK_NEXT_TAG(MIB_TAG_STATUS | MIB_END_OF_TAG);
            break;
         case MIB_TAG_ACCESS:
            m_iAccess = pFile->fgetc();
            CHECK_NEXT_TAG(MIB_TAG_ACCESS | MIB_END_OF_TAG);
            break;
         case MIB_TAG_OBJECT:
            pObject = new SNMP_MIBObject;
            if (pObject->readFromFile(pFile))
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
   return (nState == 1) ? TRUE : FALSE;
}

/**
 * Load MIB tree from file
 */
UINT32 LIBNXSNMP_EXPORTABLE SNMPLoadMIBTree(const TCHAR *pszFile, SNMP_MIBObject **ppRoot)
{
   FILE *pFile;
   ZFile *pZFile;
   SNMP_MIB_HEADER header;
   UINT32 dwRet = SNMP_ERR_SUCCESS;

   pFile = _tfopen(pszFile, _T("rb"));
   if (pFile != NULL)
   {
      if (fread(&header, 1, sizeof(SNMP_MIB_HEADER), pFile) == sizeof(SNMP_MIB_HEADER))
      {
         if (!memcmp(header.chMagic, MIB_FILE_MAGIC, 6))
         {
            header.flags = ntohs(header.flags);
            fseek(pFile, header.bHeaderSize, SEEK_SET);
            pZFile = new ZFile(pFile, header.flags & SMT_COMPRESS_DATA, FALSE);
            if (pZFile->fgetc() == MIB_TAG_OBJECT)
            {
               *ppRoot = new SNMP_MIBObject;
               if (!(*ppRoot)->readFromFile(pZFile))
               {
                  delete *ppRoot;
                  dwRet = SNMP_ERR_BAD_FILE_DATA;
               }
            }
            else
            {
               dwRet = SNMP_ERR_BAD_FILE_DATA;
            }
            pZFile->close();
            delete pZFile;
         }
         else
         {
            dwRet = SNMP_ERR_BAD_FILE_HEADER;
            fclose(pFile);
         }
      }
      else
      {
         dwRet = SNMP_ERR_BAD_FILE_HEADER;
         fclose(pFile);
      }
   }
   else
   {
      dwRet = SNMP_ERR_FILE_IO;
   }
   return dwRet;
}

/**
 * Get timestamp from saved MIB tree
 */
UINT32 LIBNXSNMP_EXPORTABLE SNMPGetMIBTreeTimestamp(const TCHAR *pszFile, UINT32 *pdwTimestamp)
{
   FILE *pFile;
   SNMP_MIB_HEADER header;
   UINT32 dwRet = SNMP_ERR_SUCCESS;

   pFile = _tfopen(pszFile, _T("rb"));
   if (pFile != NULL)
   {
      if (fread(&header, 1, sizeof(SNMP_MIB_HEADER), pFile) == sizeof(SNMP_MIB_HEADER))
      {
         if (!memcmp(header.chMagic, MIB_FILE_MAGIC, 6))
         {
            *pdwTimestamp = ntohl(header.dwTimeStamp);
         }
         else
         {
            dwRet = SNMP_ERR_BAD_FILE_HEADER;
         }
      }
      else
      {
         dwRet = SNMP_ERR_BAD_FILE_HEADER;
      }
      fclose(pFile);
   }
   else
   {
      dwRet = SNMP_ERR_FILE_IO;
   }
   return dwRet;
}
