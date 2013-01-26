/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
** Copyright (C) 2005-2013 Victor Kirhenshtein
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
** File: mibparse.cpp
**
**/

#include "nxmibc.h"
#include "mibparse.h"

/**
 * Do actual builtin object creation
 */
static MP_OBJECT *CreateObject(const char *pszName, DWORD dwId)
{
   MP_OBJECT *pObject;
   MP_SUBID *pSubId;

   pObject = CREATE(MP_OBJECT);
   pObject->pszName = strdup(pszName);
   pObject->iType = MIBC_OBJECT;
   pObject->pOID = da_create();
   pSubId = CREATE(MP_SUBID);
   pSubId->bResolved = TRUE;
   pSubId->dwValue = dwId;
   pSubId->pszName = strdup(pszName);
   da_add(pObject->pOID, pSubId);
   return pObject;
}

/**
 * Create builtin object
 */
static MP_OBJECT *CreateBuiltinObject(char *pszName)
{
   MP_OBJECT *pObject = NULL;

   if (!strcmp(pszName, "iso"))
   {
      pObject = CreateObject("iso", 1);
   }
   else if (!strcmp(pszName, "ccitt"))
   {
      pObject = CreateObject("ccitt", 0);
   }
   return pObject;
}

/**
 * Find module by name
 */
static MP_MODULE *FindModuleByName(DynArray *pModuleList, char *pszName)
{
   int i, iNumModules;

   iNumModules = da_size(pModuleList);
   for(i = 0; i < iNumModules; i++)
      if (!strcmp(((MP_MODULE *)da_get(pModuleList, i))->pszName, pszName))
         return (MP_MODULE *)da_get(pModuleList, i);
   return NULL;
}

/**
 * Find object in module
 */
static MP_OBJECT *FindObjectByName(MP_MODULE *pModule, char *pszName, int *pnIndex)
{
   int i, iNumObjects;
	MP_OBJECT *pObject;

   iNumObjects = da_size(pModule->pObjectList);
   for(i = (pnIndex != NULL) ? *pnIndex : 0; i < iNumObjects; i++)
   {
		pObject = (MP_OBJECT *)da_get(pModule->pObjectList, i);
      if (!strcmp(pObject->pszName, pszName))
		{
			if (pnIndex != NULL)
				*pnIndex = i + 1;
         return pObject;
		}
   }
	if (pnIndex != NULL)
		*pnIndex = i;
   return NULL;
}

/**
 * Find imported object in module
 */
static MP_OBJECT *FindImportedObjectByName(MP_MODULE *pModule, char *pszName,
                                           MP_MODULE **ppImportModule)
{
   int i, j, iNumImports, iNumSymbols;
   MP_IMPORT_MODULE *pImport;

   iNumImports = da_size(pModule->pImportList);
   for(i = 0; i < iNumImports; i++)
   {
      pImport = (MP_IMPORT_MODULE *)da_get(pModule->pImportList, i);
      iNumSymbols = da_size(pImport->pSymbols);
      for(j = 0; j < iNumSymbols; j++)
         if (!strcmp((char *)da_get(pImport->pSymbols, j), pszName))
         {
            *ppImportModule = pImport->pModule;
            return (MP_OBJECT *)da_get(pImport->pObjects, j);
         }
   }
   return NULL;
}

/**
 * Find next module in chain, if symbol is imported and then re-exported
 */
static MP_MODULE *FindNextImportModule(DynArray *pModuleList, MP_MODULE *pModule, char *pszSymbol)
{
   int i, j, iNumImports, iNumSymbols;
   MP_IMPORT_MODULE *pImport;

   iNumImports = da_size(pModule->pImportList);
   for(i = 0; i < iNumImports; i++)
   {
      pImport = (MP_IMPORT_MODULE *)da_get(pModule->pImportList, i);
      iNumSymbols = da_size(pImport->pSymbols);
      for(j = 0; j < iNumSymbols; j++)
         if (!strcmp((char *)da_get(pImport->pSymbols, j), pszSymbol))
            return FindModuleByName(pModuleList, pImport->pszName);
   }
   return NULL;
}

/**
 * Resolve imports
 */
static void ResolveImports(DynArray *pModuleList, MP_MODULE *pModule)
{
   int i, j, iNumImports, iNumSymbols;
   MP_MODULE *pImportModule;
   MP_IMPORT_MODULE *pImport;
   MP_OBJECT *pObject;
   char *pszSymbol;

   iNumImports = da_size(pModule->pImportList);
   for(i = 0; i < iNumImports; i++)
   {
      pImport = (MP_IMPORT_MODULE *)da_get(pModule->pImportList, i);
      pImportModule = FindModuleByName(pModuleList, pImport->pszName);
      if (pImportModule != NULL)
      {
         pImport->pModule = pImportModule;
         iNumSymbols = da_size(pImport->pSymbols);
         pImport->pObjects = da_create();
         for(j = 0; j < iNumSymbols; j++)
         {
            pszSymbol = (char *)da_get(pImport->pSymbols, j);
            do
            {
               pObject = FindObjectByName(pImportModule, pszSymbol, NULL);
               if (pObject != NULL)
               {
                  da_add(pImport->pObjects, pObject);
               }
               else
               {
                  pImportModule = FindNextImportModule(pModuleList, pImportModule, pszSymbol);
                  if (pImportModule == NULL)
                  {
                     Error(ERR_UNRESOLVED_IMPORT, pModule->pszName, pszSymbol);
                     da_add(pImport->pObjects, NULL);
                     break;
                  }
               }
            } while(pObject == NULL);
				pImportModule = pImport->pModule;	// Restore current import module if it was changed in lookup cycle
         }
      }
      else
      {
         Error(ERR_UNRESOLVED_MODULE, pModule->pszName, pImport->pszName);
      }
   }
}

/**
 * Build full OID for object
 */
static void BuildFullOID(MP_MODULE *pModule, MP_OBJECT *pObject)
{
   int iLen;
   MP_SUBID *pSubId;
   MP_OBJECT *pParent;
   DynArray *pOID;

   iLen = da_size(pObject->pOID);
   while(iLen > 0)
   {
      pSubId = (MP_SUBID *)da_get(pObject->pOID, iLen - 1);
      if (!pSubId->bResolved)
      {
         pParent = FindObjectByName(pModule, pSubId->pszName, NULL);
         if (pParent != NULL)
         {
            BuildFullOID(pModule, pParent);
         }
         else
         {
            MP_MODULE *pImportModule;

            pParent = FindImportedObjectByName(pModule, pSubId->pszName, &pImportModule);
            if (pParent != NULL)
            {
               BuildFullOID(pImportModule, pParent);
            }
            else
            {
               pParent = CreateBuiltinObject(pSubId->pszName);
            }
         }
         if (pParent != NULL)
         {
            pOID = da_create();
            pOID->nSize = pParent->pOID->nSize + pObject->pOID->nSize - iLen;
            pOID->ppData = (void **)malloc(sizeof(void *) * pOID->nSize);
            memcpy(pOID->ppData, pParent->pOID->ppData, sizeof(void *) * pParent->pOID->nSize);
            memcpy(&pOID->ppData[pParent->pOID->nSize], &pObject->pOID->ppData[iLen],
                   sizeof(void *) * (pObject->pOID->nSize - iLen));
            da_destroy(pObject->pOID);
            pObject->pOID = pOID;
            break;
         }
         else
         {
            Error(ERR_UNRESOLVED_SYMBOL, pModule->pszName, pSubId->pszName);
         }
      }
      else
      {
         iLen--;
      }
   }
}

/**
 * Resolve syntax for object
 */
static void ResolveSyntax(MP_MODULE *pModule, MP_OBJECT *pObject)
{
   MP_OBJECT *pType;
   MP_MODULE *pCurrModule = pModule;
   char *pszType;
	int index;

   if ((pObject->iSyntax != -1) || (pObject->pszDataType == NULL))
      return;

   pszType = pObject->pszDataType;
   do
   {
		index = 0;
		do
		{
			pType = FindObjectByName(pCurrModule, CHECK_NULL_A(pszType), &index);
		}
		while((pType != NULL) && (pType->iType == MIBC_OBJECT));
      if (pType == NULL)
         pType = FindImportedObjectByName(pCurrModule,
                                          CHECK_NULL_A(pszType),
                                          &pCurrModule);
      if (pType == NULL)
         break;
      pszType = pType->pszDataType;
   } while(pType->iSyntax == -1);

   if (pType != NULL)
   {
      pObject->iSyntax = pType->iSyntax;
		if (pType->iType == MIBC_TEXTUAL_CONVENTION)
			pObject->pszTextualConvention = pType->pszDescription;
   }
   else
   {
      Error(ERR_UNRESOLVED_SYNTAX, pModule->pszName, pObject->pszDataType, pObject->pszName);
   }
}

/**
 * Resolve object identifiers
 */
static void ResolveObjects(DynArray *pModuleList, MP_MODULE *pModule)
{
   int i, iNumObjects;
   MP_OBJECT *pObject;

   iNumObjects = da_size(pModule->pObjectList);
   for(i = 0; i < iNumObjects; i++)
   {
      pObject = (MP_OBJECT *)da_get(pModule->pObjectList, i);
      if (pObject->iType == MIBC_OBJECT)
      {
         BuildFullOID(pModule, pObject);
         ResolveSyntax(pModule, pObject);
      }
   }
}

/**
 * Build MIB tree from object list
 */
static void BuildMIBTree(SNMP_MIBObject *pRoot, MP_MODULE *pModule)
{
   int i, j, iLen, iNumObjects;
   MP_OBJECT *pObject;
   MP_SUBID *pSubId;
   SNMP_MIBObject *pCurrObj, *pNewObj;

   iNumObjects = da_size(pModule->pObjectList);
   for(i = 0; i < iNumObjects; i++)
   {
      pObject = (MP_OBJECT *)da_get(pModule->pObjectList, i);
      if (pObject->iType == MIBC_OBJECT)
      {
         iLen = da_size(pObject->pOID);
         for(j = 0, pCurrObj = pRoot; j < iLen; j++)
         {
            pSubId = (MP_SUBID *)da_get(pObject->pOID, j);
            pNewObj = pCurrObj->findChildByID(pSubId->dwValue);
            if (pNewObj == NULL)
            {
               if (j == iLen - 1)
					{
#ifdef UNICODE
						WCHAR *wname = (pObject->pszName != NULL) ? WideStringFromMBString(pObject->pszName) : NULL;
						WCHAR *wdescr = (pObject->pszDescription != NULL) ? WideStringFromMBString(pObject->pszDescription) : NULL;
						WCHAR *wtc = (pObject->pszTextualConvention != NULL) ? WideStringFromMBString(pObject->pszTextualConvention) : NULL;
                  pNewObj = new SNMP_MIBObject(pSubId->dwValue, wname,
                                               pObject->iSyntax,
                                               pObject->iStatus,
                                               pObject->iAccess,
                                               wdescr, wtc);
						safe_free(wname);
						safe_free(wdescr);
						safe_free(wtc);
#else
                  pNewObj = new SNMP_MIBObject(pSubId->dwValue, pObject->pszName,
                                               pObject->iSyntax,
                                               pObject->iStatus,
                                               pObject->iAccess,
                                               pObject->pszDescription,
															  pObject->pszTextualConvention);
#endif
					}
               else
					{
#ifdef UNICODE
						WCHAR *wname = (pSubId->pszName != NULL) ? WideStringFromMBString(pSubId->pszName) : NULL;
                  pNewObj = new SNMP_MIBObject(pSubId->dwValue, wname);
						safe_free(wname);
#else
                  pNewObj = new SNMP_MIBObject(pSubId->dwValue, pSubId->pszName);
#endif
					}
               pCurrObj->addChild(pNewObj);
            }
            else
            {
               if (j == iLen - 1)
               {
                  // Last OID in chain, update object information
#ifdef UNICODE
						WCHAR *wdescr = (pObject->pszDescription != NULL) ? WideStringFromMBString(pObject->pszDescription) : NULL;
						WCHAR *wtc = (pObject->pszTextualConvention != NULL) ? WideStringFromMBString(pObject->pszTextualConvention) : NULL;
                  pNewObj->setInfo(pObject->iSyntax, pObject->iStatus, pObject->iAccess, wdescr, wtc);
						safe_free(wdescr);
						safe_free(wtc);
#else
                  pNewObj->setInfo(pObject->iSyntax, pObject->iStatus, pObject->iAccess, pObject->pszDescription, pObject->pszTextualConvention);
#endif
                  if (pNewObj->getName() == NULL)
						{
#ifdef UNICODE
							WCHAR *wname = (pObject->pszName != NULL) ? WideStringFromMBString(pObject->pszName) : NULL;
                     pNewObj->setName(wname);
							safe_free(wname);
#else
                     pNewObj->setName(pObject->pszName);
#endif
						}
               }
            }
            pCurrObj = pNewObj;
         }
      }
   }
}

/**
 * Interface to parser
 */
int ParseMIBFiles(int nNumFiles, char **ppszFileList, SNMP_MIBObject **ppRoot)
{
   int i, iNumModules, nRet;
   DynArray *pModuleList;
   MP_MODULE *pModule;
   SNMP_MIBObject *pRoot;

   _tprintf(_T("Parsing source files:\n"));
   pModuleList = da_create();
   for(i = 0; i < nNumFiles; i++)
   {
      _tprintf(_T("   %hs\n"), ppszFileList[i]);
      pModule = ParseMIB(ppszFileList[i]);
      if (pModule == NULL)
      {
         nRet = SNMP_MPE_PARSE_ERROR;
         goto parse_error;
      }
      da_add(pModuleList, pModule);
   }

   iNumModules = da_size(pModuleList);
   _tprintf(_T("Resolving imports:\n"));
   for(i = 0; i < iNumModules; i++)
   {
      pModule = (MP_MODULE *)da_get(pModuleList, i);
      _tprintf(_T("   %hs\n"), pModule->pszName);
      ResolveImports(pModuleList, pModule);
   }

   _tprintf(_T("Resolving object identifiers:\n"));
   for(i = 0; i < iNumModules; i++)
   {
      pModule = (MP_MODULE *)da_get(pModuleList, i);
      _tprintf(_T("   %hs\n"), pModule->pszName);
      ResolveObjects(pModuleList, pModule);
   }

   _tprintf(_T("Creating MIB tree:\n"));
   pRoot = new SNMP_MIBObject;
   for(i = 0; i < iNumModules; i++)
   {
      pModule = (MP_MODULE *)da_get(pModuleList, i);
      _tprintf(_T("   %hs\n"), pModule->pszName);
      BuildMIBTree(pRoot, pModule);
   }

   *ppRoot = pRoot;
   return SNMP_MPE_SUCCESS;

parse_error:
   *ppRoot = NULL;
   return nRet;
}
