/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: mib.cpp
**
**/

#include "libnxsnmp.h"


//
// Default constructor for SNMP_MIBObject
//

SNMP_MIBObject::SNMP_MIBObject(void)
{
   Initialize();

   m_dwOID = 0;
   m_pszName = NULL;
   m_pszDescription = NULL;
   m_iStatus = -1;
   m_iAccess = -1;
   m_iType = -1;
}


//
// Construct object with all data
//

SNMP_MIBObject::SNMP_MIBObject(DWORD dwOID, TCHAR *pszName, int iType, 
                               int iStatus, int iAccess, TCHAR *pszDescription)
{
   Initialize();

   m_dwOID = dwOID;
   m_pszName = (pszName != NULL) ? _tcsdup(pszName) : NULL;
   m_pszDescription = (pszDescription != NULL) ? _tcsdup(pszDescription) : pszDescription;
   m_iStatus = iStatus;
   m_iAccess = iAccess;
   m_iType = iType;
}


//
// Construct object with only ID and name
//

SNMP_MIBObject::SNMP_MIBObject(DWORD dwOID, TCHAR *pszName)
{
   Initialize();

   m_dwOID = dwOID;
   m_pszName = (pszName != NULL) ? _tcsdup(pszName) : NULL;
   m_pszDescription = NULL;
   m_iStatus = -1;
   m_iAccess = -1;
   m_iType = -1;
}


//
// Common initialization
//

void SNMP_MIBObject::Initialize(void)
{
   m_pParent = NULL;
   m_pNext = NULL;
   m_pPrev = NULL;
   m_pFirst = NULL;
   m_pLast = NULL;
}


//
// Destructor
//

SNMP_MIBObject::~SNMP_MIBObject()
{
   SNMP_MIBObject *pCurr, *pNext;

   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pNext)
   {
      pNext = pCurr->Next();
      delete pCurr;
   }
   safe_free(m_pszName);
   safe_free(m_pszDescription);
}


//
// Add child object
//

void SNMP_MIBObject::AddChild(SNMP_MIBObject *pObject)
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
   pObject->SetParent(this);
}


//
// Find child object by OID
//

SNMP_MIBObject *SNMP_MIBObject::FindChildByID(DWORD dwOID)
{
   SNMP_MIBObject *pCurr;

   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pCurr->Next())
      if (pCurr->m_dwOID == dwOID)
         return pCurr;
   return NULL;
}


//
// Set information
//

void SNMP_MIBObject::SetInfo(int iType, int iStatus, int iAccess, TCHAR *pszDescription)
{
   safe_free(m_pszDescription);
   m_iType = iType;
   m_iStatus = iStatus;
   m_iAccess = iAccess;
   m_pszDescription = (pszDescription != NULL) ? _tcsdup(pszDescription) : NULL;
}


//
// Print MIB subtree
//

void SNMP_MIBObject::Print(int nIndent)
{
   SNMP_MIBObject *pCurr;

   if ((nIndent == 0) && (m_pszName == NULL) && (m_dwOID == 0))
      _tprintf(_T("[root]\n"));
   else
      _tprintf(_T("%*s%s(%d)\n"), nIndent, _T(""), m_pszName, m_dwOID);

   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pCurr->Next())
      pCurr->Print(nIndent + 2);
}


//
// Write string to file
//

static void WriteStringToFile(FILE *pFile, TCHAR *pszStr)
{
   WORD wLen, wTemp;
#ifdef UNICODE
   char *pszBuffer;
#endif

   wLen = _tcslen(pszStr);
   wTemp = htons(wLen);
   fwrite(&wTemp, 2, 1, pFile);
#ifdef UNICODE
   pszBuffer = (char *)malloc(wLen + 1);
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
		                 pszStr, -1, pszBuffer, wLen + 1, NULL, NULL);
   fwrite(pszBuffer, 1, wLen, pFile);
   free(pszBuffer);
#else
   fwrite(pszStr, 1, wLen, pFile);
#endif
}


//
// Write object to file
//

void SNMP_MIBObject::WriteToFile(FILE *pFile, DWORD dwFlags)
{
   SNMP_MIBObject *pCurr;

   fputc(MIB_TAG_OBJECT, pFile);

   // Object name
   fputc(MIB_TAG_NAME, pFile);
   WriteStringToFile(pFile, CHECK_NULL_EX(m_pszName));
   fputc(MIB_TAG_NAME | MIB_END_OF_TAG, pFile);

   // Object ID
   if (m_dwOID < 256)
   {
      fputc(MIB_TAG_BYTE_OID, pFile);
      fputc((int)m_dwOID, pFile);
      fputc(MIB_TAG_BYTE_OID | MIB_END_OF_TAG, pFile);
   }
   else if (m_dwOID < 65536)
   {
      WORD wTemp;

      fputc(MIB_TAG_WORD_OID, pFile);
      wTemp = htons((WORD)m_dwOID);
      fwrite(&wTemp, 2, 1, pFile);
      fputc(MIB_TAG_WORD_OID | MIB_END_OF_TAG, pFile);
   }
   else
   {
      DWORD dwTemp;

      fputc(MIB_TAG_DWORD_OID, pFile);
      dwTemp = htonl(m_dwOID);
      fwrite(&dwTemp, 4, 1, pFile);
      fputc(MIB_TAG_DWORD_OID | MIB_END_OF_TAG, pFile);
   }

   // Object status
   fputc(MIB_TAG_STATUS, pFile);
   fputc(m_iStatus, pFile);
   fputc(MIB_TAG_STATUS | MIB_END_OF_TAG, pFile);

   // Object access
   fputc(MIB_TAG_ACCESS, pFile);
   fputc(m_iAccess, pFile);
   fputc(MIB_TAG_ACCESS | MIB_END_OF_TAG, pFile);

   // Object type
   fputc(MIB_TAG_TYPE, pFile);
   fputc(m_iType, pFile);
   fputc(MIB_TAG_TYPE | MIB_END_OF_TAG, pFile);

   // Object description
   if (!(dwFlags & SMT_SKIP_DESCRIPTIONS))
   {
      fputc(MIB_TAG_DESCRIPTION, pFile);
      WriteStringToFile(pFile, CHECK_NULL_EX(m_pszDescription));
      fputc(MIB_TAG_DESCRIPTION | MIB_END_OF_TAG, pFile);
   }

   // Save childs
   for(pCurr = m_pFirst; pCurr != NULL; pCurr = pCurr->Next())
      pCurr->WriteToFile(pFile, dwFlags);

   fputc(MIB_TAG_OBJECT | MIB_END_OF_TAG, pFile);
}


//
// Save MIB tree to file
//

DWORD LIBNXSNMP_EXPORTABLE SNMPSaveMIBTree(TCHAR *pszFile, SNMP_MIBObject *pRoot,
                                           DWORD dwFlags)
{
   FILE *pFile;
   SNMP_MIB_HEADER header;
   DWORD dwRet = SNMP_ERR_SUCCESS;

   pFile = _tfopen(pszFile, _T("wb"));
   if (pFile != NULL)
   {
      memcpy(header.chMagic, MIB_FILE_MAGIC, 6);
      header.bVersion = MIB_FILE_VERSION;
      header.bHeaderSize = sizeof(SNMP_MIB_HEADER);
      header.dwTimeStamp = htonl(time(NULL));
      memset(header.bReserved, 0, sizeof(header.bReserved));
      fwrite(&header, sizeof(SNMP_MIB_HEADER), 1, pFile);
      pRoot->WriteToFile(pFile, dwFlags);
      fclose(pFile);
   }
   else
   {
      dwRet = SNMP_ERR_FILE_IO;
   }
   return dwRet;
}


//
// Read string from file
//

static TCHAR *ReadStringFromFile(FILE *pFile)
{
   TCHAR *pszStr;
   WORD wLen;

   fread(&wLen, 1, 2, pFile);
   wLen = ntohs(wLen);
   if (wLen > 0)
   {
      pszStr = (TCHAR *)malloc(sizeof(TCHAR) * (wLen + 1));
#ifdef UNICODE
      pszBuffer = (char *)malloc(wLen + 1);
      fread(pszBuffer, 1, wLen, pFile);
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszBuffer, wLen, pszStr, wLen + 1);
      free(pszBuffer);
#else
      fread(pszStr, 1, wLen, pFile);
#endif
      pszStr[wLen] = 0;
   }
   else
   {
      pszStr = NULL;
   }
   return pszStr;
}


//
// Read object from file
//

#define CHECK_NEXT_TAG(x) { ch = fgetc(pFile); if (ch != (x)) nState--; }

BOOL SNMP_MIBObject::ReadFromFile(FILE *pFile)
{
   int ch, nState = 0;
   WORD wTmp;
   DWORD dwTmp;
   SNMP_MIBObject *pObject;

   while(nState == 0)
   {
      ch = fgetc(pFile);
      switch(ch)
      {
         case (MIB_TAG_OBJECT | MIB_END_OF_TAG):
            nState++;
            break;
         case MIB_TAG_BYTE_OID:
            m_dwOID = (DWORD)fgetc(pFile);
            CHECK_NEXT_TAG(MIB_TAG_BYTE_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_WORD_OID:
            fread(&wTmp, 1, 2, pFile);
            m_dwOID = (DWORD)ntohs(wTmp);
            CHECK_NEXT_TAG(MIB_TAG_WORD_OID | MIB_END_OF_TAG);
            break;
         case MIB_TAG_DWORD_OID:
            fread(&dwTmp, 1, 4, pFile);
            m_dwOID = ntohl(dwTmp);
            CHECK_NEXT_TAG(MIB_TAG_DWORD_OID | MIB_END_OF_TAG);
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
         case MIB_TAG_TYPE:
            m_iType = fgetc(pFile);
            CHECK_NEXT_TAG(MIB_TAG_TYPE | MIB_END_OF_TAG);
            break;
         case MIB_TAG_STATUS:
            m_iStatus = fgetc(pFile);
            CHECK_NEXT_TAG(MIB_TAG_STATUS | MIB_END_OF_TAG);
            break;
         case MIB_TAG_ACCESS:
            m_iAccess = fgetc(pFile);
            CHECK_NEXT_TAG(MIB_TAG_ACCESS | MIB_END_OF_TAG);
            break;
         case MIB_TAG_OBJECT:
            pObject = new SNMP_MIBObject;
            if (pObject->ReadFromFile(pFile))
            {
               AddChild(pObject);
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


//
// Load MIB tree from file
//

DWORD LIBNXSNMP_EXPORTABLE SNMPLoadMIBTree(TCHAR *pszFile, SNMP_MIBObject **ppRoot)
{
   FILE *pFile;
   SNMP_MIB_HEADER header;
   DWORD dwRet = SNMP_ERR_SUCCESS;

   pFile = _tfopen(pszFile, _T("rb"));
   if (pFile != NULL)
   {
      fread(&header, 1, sizeof(SNMP_MIB_HEADER), pFile);
      if (!memcmp(header.chMagic, MIB_FILE_MAGIC, 6))
      {
         fseek(pFile, header.bHeaderSize, SEEK_SET);
         if (fgetc(pFile) == MIB_TAG_OBJECT)
         {
            *ppRoot = new SNMP_MIBObject;
            if (!(*ppRoot)->ReadFromFile(pFile))
            {
               delete *ppRoot;
               dwRet = SNMP_ERR_BAD_FILE_DATA;
            }
         }
         else
         {
            dwRet = SNMP_ERR_BAD_FILE_DATA;
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


//
// Get timestamp from saved MIB tree
//

DWORD LIBNXSNMP_EXPORTABLE SNMPGetMIBTreeTimestamp(TCHAR *pszFile, DWORD *pdwTimestamp)
{
   FILE *pFile;
   SNMP_MIB_HEADER header;
   DWORD dwRet = SNMP_ERR_SUCCESS;

   pFile = _tfopen(pszFile, _T("rb"));
   if (pFile != NULL)
   {
      fread(&header, 1, sizeof(SNMP_MIB_HEADER), pFile);
      if (!memcmp(header.chMagic, MIB_FILE_MAGIC, 6))
      {
         *pdwTimestamp = ntohl(header.dwTimeStamp);
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
