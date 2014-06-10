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
   MP_OBJECT *pObject = new MP_OBJECT;
   pObject->pszName = strdup(pszName);
   pObject->iType = MIBC_OBJECT;

   MP_SUBID *pSubId = new MP_SUBID;
   pSubId->bResolved = TRUE;
   pSubId->dwValue = dwId;
   pSubId->pszName = strdup(pszName);
   pObject->pOID->add(pSubId);
   
   return pObject;
}

/**
 * Create builtin object
 */
static MP_OBJECT *GetBuiltinObject(const char *pszName)
{
   MP_OBJECT *pObject = NULL;

   if (!strcmp(pszName, "iso"))
   {
      static MP_OBJECT *iso = NULL;
      if (iso == NULL)
         iso = CreateObject("iso", 1);
      pObject = iso;
   }
   else if (!strcmp(pszName, "ccitt"))
   {
      static MP_OBJECT *ccitt = NULL;
      if (ccitt == NULL)
         ccitt = CreateObject("ccitt", 0);
      pObject = ccitt;
   }
   return pObject;
}

/**
 * Find module by name
 */
static MP_MODULE *FindModuleByName(ObjectArray<MP_MODULE> *pModuleList, const char *pszName)
{
   for(int i = 0; i < pModuleList->size(); i++)
      if (!strcmp(pModuleList->get(i)->pszName, pszName))
         return pModuleList->get(i);
   return NULL;
}

/**
 * Find object in module
 */
static MP_OBJECT *FindObjectByName(MP_MODULE *pModule, const char *pszName, int *pnIndex)
{
   int i;

   for(i = (pnIndex != NULL) ? *pnIndex : 0; i < pModule->pObjectList->size(); i++)
   {
		MP_OBJECT *pObject = pModule->pObjectList->get(i);
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
static MP_OBJECT *FindImportedObjectByName(MP_MODULE *pModule, const char *pszName, MP_MODULE **ppImportModule)
{
   for(int i = 0; i < pModule->pImportList->size(); i++)
   {
      MP_IMPORT_MODULE *pImport = pModule->pImportList->get(i);
      for(int j = 0; j < pImport->pSymbols->size(); j++)
         if (!strcmp((char *)pImport->pSymbols->get(j), pszName))
         {
            *ppImportModule = pImport->pModule;
            return pImport->pObjects->get(j);
         }
   }
   return NULL;
}

/**
 * Find next module in chain, if symbol is imported and then re-exported
 */
static MP_MODULE *FindNextImportModule(ObjectArray<MP_MODULE> *pModuleList, MP_MODULE *pModule, const char *pszSymbol)
{
   for(int i = 0; i < pModule->pImportList->size(); i++)
   {
      MP_IMPORT_MODULE *pImport = pModule->pImportList->get(i);
      for(int j = 0; j < pImport->pSymbols->size(); j++)
         if (!strcmp((char *)pImport->pSymbols->get(j), pszSymbol))
            return FindModuleByName(pModuleList, pImport->pszName);
   }
   return NULL;
}

/**
 * Resolve imports
 */
static void ResolveImports(ObjectArray<MP_MODULE> *pModuleList, MP_MODULE *pModule)
{
   for(int i = 0; i < pModule->pImportList->size(); i++)
   {
      MP_IMPORT_MODULE *pImport = pModule->pImportList->get(i);
      MP_MODULE *pImportModule = FindModuleByName(pModuleList, pImport->pszName);
      if (pImportModule != NULL)
      {
         pImport->pModule = pImportModule;
         for(int j = 0; j < pImport->pSymbols->size(); j++)
         {
            const char *pszSymbol = (const char *)pImport->pSymbols->get(j);
            MP_OBJECT *pObject;
            do
            {
               pObject = FindObjectByName(pImportModule, pszSymbol, NULL);
               if (pObject != NULL)
               {
                  pImport->pObjects->add(pObject);
               }
               else
               {
                  pImportModule = FindNextImportModule(pModuleList, pImportModule, pszSymbol);
                  if (pImportModule == NULL)
                  {
                     Error(ERR_UNRESOLVED_IMPORT, pModule->pszName, pszSymbol);
                     pImport->pObjects->add(NULL);
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

   iLen = pObject->pOID->size();
   while(iLen > 0)
   {
      pSubId = pObject->pOID->get(iLen - 1);
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
               pParent = GetBuiltinObject(pSubId->pszName);
            }
         }
         if (pParent != NULL)
         {
            ObjectArray<MP_SUBID> *pOID = new ObjectArray<MP_SUBID>(pParent->pOID->size() + pObject->pOID->size() - iLen, 16, true);
            for(int i = 0; i < pParent->pOID->size(); i++)
            {
               pOID->add(new MP_SUBID(pParent->pOID->get(i)));
            }
            for(int j = iLen; j < pObject->pOID->size(); j++)
            {
               pOID->add(new MP_SUBID(pObject->pOID->get(j)));
            }
            delete pObject->pOID;
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
         pObject->pszTextualConvention = (pType->pszDescription != NULL) ? strdup(pType->pszDescription) : NULL;
   }
   else
   {
      Error(ERR_UNRESOLVED_SYNTAX, pModule->pszName, pObject->pszDataType, pObject->pszName);
   }
}

/**
 * Resolve object identifiers
 */
static void ResolveObjects(ObjectArray<MP_MODULE> *pModuleList, MP_MODULE *pModule)
{
   for(int i = 0; i < pModule->pObjectList->size(); i++)
   {
      MP_OBJECT *pObject = pModule->pObjectList->get(i);
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
   for(int i = 0; i < pModule->pObjectList->size(); i++)
   {
      MP_OBJECT *pObject = pModule->pObjectList->get(i);
      if (pObject->iType == MIBC_OBJECT)
      {
         int iLen = pObject->pOID->size();
         SNMP_MIBObject *pCurrObj = pRoot;
         for(int j = 0; j < iLen; j++)
         {
            MP_SUBID *pSubId = pObject->pOID->get(j);
            SNMP_MIBObject *pNewObj = pCurrObj->findChildByID(pSubId->dwValue);
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
   int i, nRet;
   SNMP_MIBObject *pRoot;

   _tprintf(_T("Parsing source files:\n"));
   ObjectArray<MP_MODULE> *pModuleList = new ObjectArray<MP_MODULE>(16, 16, true);
   for(i = 0; i < nNumFiles; i++)
   {
      _tprintf(_T("   %hs\n"), ppszFileList[i]);
      MP_MODULE *pModule = ParseMIB(ppszFileList[i]);
      if (pModule == NULL)
      {
         nRet = SNMP_MPE_PARSE_ERROR;
         goto parse_error;
      }
      pModuleList->add(pModule);
   }

   _tprintf(_T("Resolving imports:\n"));
   for(i = 0; i < pModuleList->size(); i++)
   {
      MP_MODULE *pModule = pModuleList->get(i);
      _tprintf(_T("   %hs\n"), pModule->pszName);
      ResolveImports(pModuleList, pModule);
   }

   _tprintf(_T("Resolving object identifiers:\n"));
   for(i = 0; i < pModuleList->size(); i++)
   {
      MP_MODULE *pModule = pModuleList->get(i);
      _tprintf(_T("   %hs\n"), pModule->pszName);
      ResolveObjects(pModuleList, pModule);
   }

   _tprintf(_T("Creating MIB tree:\n"));
   pRoot = new SNMP_MIBObject;
   for(i = 0; i < pModuleList->size(); i++)
   {
      MP_MODULE *pModule = pModuleList->get(i);
      _tprintf(_T("   %hs\n"), pModule->pszName);
      BuildMIBTree(pRoot, pModule);
   }

   *ppRoot = pRoot;
   delete pModuleList;
   return SNMP_MPE_SUCCESS;

parse_error:
   *ppRoot = NULL;
   delete pModuleList;
   return nRet;
}
