/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
** Copyright (C) 2005-2024 Victor Kirhenshtein
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
   pObject->pszName = MemCopyStringA(pszName);
   pObject->iType = MIBC_OBJECT;
   pObject->oid = new ObjectArray<MP_SUBID>(16, 16, Ownership::True);

   MP_SUBID *pSubId = new MP_SUBID;
   pSubId->bResolved = TRUE;
   pSubId->dwValue = dwId;
   pSubId->pszName = MemCopyStringA(pszName);
   pObject->oid->add(pSubId);
   
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
static MP_MODULE *FindModuleByName(ObjectArray<MP_MODULE> *moduleList, const char *pszName)
{
   for(int i = 0; i < moduleList->size(); i++)
      if (!strcmp(moduleList->get(i)->pszName, pszName))
         return moduleList->get(i);
   return NULL;
}

/**
 * Find object in module
 */
static MP_OBJECT *FindObjectByName(MP_MODULE *module, const char *pszName, int *pnIndex)
{
   int i;

   for(i = (pnIndex != NULL) ? *pnIndex : 0; i < module->pObjectList->size(); i++)
   {
		MP_OBJECT *pObject = module->pObjectList->get(i);
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
static MP_OBJECT *FindImportedObjectByName(MP_MODULE *module, const char *pszName, MP_MODULE **ppImportModule)
{
   for(int i = 0; i < module->pImportList->size(); i++)
   {
      MP_IMPORT_MODULE *import = module->pImportList->get(i);
      for(int j = 0; j < import->symbols->size(); j++)
         if (!strcmp((char *)import->symbols->get(j), pszName))
         {
            *ppImportModule = import->module;
            return import->objects.get(j);
         }
   }
   return nullptr;
}

/**
 * Find next module in chain, if symbol is imported and then re-exported
 */
static MP_MODULE *FindNextImportModule(ObjectArray<MP_MODULE> *moduleList, MP_MODULE *module, const char *symbol)
{
   for(int i = 0; i < module->pImportList->size(); i++)
   {
      MP_IMPORT_MODULE *pImport = module->pImportList->get(i);
      for(int j = 0; j < pImport->symbols->size(); j++)
         if (!strcmp((char *)pImport->symbols->get(j), symbol))
            return FindModuleByName(moduleList, pImport->name);
   }
   return nullptr;
}

/**
 * Resolve imports
 */
static void ResolveImports(ObjectArray<MP_MODULE> *moduleList, MP_MODULE *module)
{
   for(int i = 0; i < module->pImportList->size(); i++)
   {
      MP_IMPORT_MODULE *pImport = module->pImportList->get(i);
      MP_MODULE *pImportModule = FindModuleByName(moduleList, pImport->name);
      if (pImportModule != nullptr)
      {
         pImport->module = pImportModule;
         for(int j = 0; j < pImport->symbols->size(); j++)
         {
            const char *pszSymbol = (const char *)pImport->symbols->get(j);
            MP_OBJECT *pObject;
            do
            {
               pObject = FindObjectByName(pImportModule, pszSymbol, nullptr);
               if (pObject != nullptr)
               {
                  pImport->objects.add(pObject);
               }
               else
               {
                  pImportModule = FindNextImportModule(moduleList, pImportModule, pszSymbol);
                  if (pImportModule == nullptr)
                  {
                     Error(ERR_UNRESOLVED_IMPORT, module->pszName, pszSymbol);
                     pImport->objects.add(nullptr);
                     break;
                  }
               }
            } while(pObject == nullptr);
				pImportModule = pImport->module;	// Restore current import module if it was changed in lookup cycle
         }
      }
      else
      {
         Error(ERR_UNRESOLVED_MODULE, module->pszName, pImport->name);
      }
   }
}

/**
 * Build full OID for object
 */
static void BuildFullOID(MP_MODULE *module, MP_OBJECT *pObject)
{
   int iLen;
   MP_SUBID *pSubId;
   MP_OBJECT *pParent;

   iLen = pObject->oid->size();
   while(iLen > 0)
   {
      pSubId = pObject->oid->get(iLen - 1);
      if (!pSubId->bResolved)
      {
         pParent = FindObjectByName(module, pSubId->pszName, nullptr);
         if (pParent != nullptr)
         {
            BuildFullOID(module, pParent);
         }
         else
         {
            MP_MODULE *pImportModule;

            pParent = FindImportedObjectByName(module, pSubId->pszName, &pImportModule);
            if (pParent != nullptr)
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
            ObjectArray<MP_SUBID> *oid = new ObjectArray<MP_SUBID>(pParent->oid->size() + pObject->oid->size() - iLen, 16, Ownership::True);
            for(int i = 0; i < pParent->oid->size(); i++)
            {
               oid->add(new MP_SUBID(pParent->oid->get(i)));
            }
            for(int j = iLen; j < pObject->oid->size(); j++)
            {
               oid->add(new MP_SUBID(pObject->oid->get(j)));
            }
            delete pObject->oid;
            pObject->oid = oid;
            break;
         }
         else
         {
            Error(ERR_UNRESOLVED_SYMBOL, module->pszName, pSubId->pszName);
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
static void ResolveSyntax(MP_MODULE *module, MP_OBJECT *pObject)
{
   if ((pObject->iSyntax != -1) || (pObject->pszDataType == nullptr))
      return;

   char *pszType = pObject->pszDataType;
   MP_MODULE *pCurrModule = module;
   MP_OBJECT *pType;
   do
   {
		int index = 0;
		do
		{
			pType = FindObjectByName(pCurrModule, CHECK_NULL_A(pszType), &index);
		}
		while((pType != nullptr) && (pType->iType == MIBC_OBJECT));
      if (pType == nullptr)
         pType = FindImportedObjectByName(pCurrModule, CHECK_NULL_A(pszType), &pCurrModule);
      if (pType == nullptr)
         break;
      pszType = pType->pszDataType;
   } while(pType->iSyntax == -1);

   if (pType != nullptr)
   {
      pObject->iSyntax = pType->iSyntax;
		if (pType->iType == MIBC_TEXTUAL_CONVENTION)
         pObject->pszTextualConvention = MemCopyStringA(pType->pszDescription);
   }
   else
   {
      Error(ERR_UNRESOLVED_SYNTAX, module->pszName, pObject->pszDataType, pObject->pszName);
   }
}

/**
 * Resolve object identifiers
 */
static void ResolveObjects(ObjectArray<MP_MODULE> *moduleList, MP_MODULE *module)
{
   for(int i = 0; i < module->pObjectList->size(); i++)
   {
      MP_OBJECT *pObject = module->pObjectList->get(i);
      if (pObject->iType == MIBC_OBJECT)
      {
         BuildFullOID(module, pObject);
         ResolveSyntax(module, pObject);
      }
   }
}

/**
 * Build MIB tree from object list
 */
static void BuildMIBTree(SNMP_MIBObject *pRoot, MP_MODULE *module)
{
   StringBuffer indexBuffer;
   for(int i = 0; i < module->pObjectList->size(); i++)
   {
      MP_OBJECT *pObject = module->pObjectList->get(i);
      if (pObject->iType == MIBC_OBJECT)
      {
         int iLen = pObject->oid->size();
         SNMP_MIBObject *pCurrObj = pRoot;
         for(int j = 0; j < iLen; j++)
         {
            MP_SUBID *pSubId = pObject->oid->get(j);
            SNMP_MIBObject *pNewObj = pCurrObj->findChildByID(pSubId->dwValue);
            if (pNewObj == nullptr)
            {
               if (j == iLen - 1)
					{
                  if (pObject->index != nullptr)
                  {
                     indexBuffer.clear();
                     for(int n = 0; n < pObject->index->size(); n++)
                     {
                        if (n > 0)
                           indexBuffer.append(_T(", "));
                        indexBuffer.appendUtf8String(static_cast<char*>(pObject->index->get(n)));
                     }
                  }
#ifdef UNICODE
						WCHAR *wname = WideStringFromMBString(pObject->pszName);
						WCHAR *wdescr = WideStringFromMBString(pObject->pszDescription);
						WCHAR *wtc = WideStringFromMBString(pObject->pszTextualConvention);
                  pNewObj = new SNMP_MIBObject(pSubId->dwValue, wname,
                                               pObject->iSyntax,
                                               pObject->iStatus,
                                               pObject->iAccess,
                                               wdescr, wtc,
                                               (pObject->index != nullptr) ? indexBuffer.cstr() : nullptr);
						MemFree(wname);
						MemFree(wdescr);
						MemFree(wtc);
#else
                  pNewObj = new SNMP_MIBObject(pSubId->dwValue, pObject->pszName,
                                               pObject->iSyntax,
                                               pObject->iStatus,
                                               pObject->iAccess,
                                               pObject->pszDescription,
															  pObject->pszTextualConvention,
															  (pObject->index != nullptr) ? indexBuffer.cstr() : nullptr);
#endif
					}
               else
					{
#ifdef UNICODE
						WCHAR *wname = WideStringFromMBString(pSubId->pszName);
                  pNewObj = new SNMP_MIBObject(pSubId->dwValue, wname);
						MemFree(wname);
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
                  if (pObject->index != nullptr)
                  {
                     indexBuffer.clear();
                     for(int n = 0; n < pObject->index->size(); n++)
                     {
                        if (n > 0)
                           indexBuffer.append(_T(", "));
                        indexBuffer.appendUtf8String(static_cast<char*>(pObject->index->get(n)));
                     }
                  }
#ifdef UNICODE
						WCHAR *wdescr = WideStringFromMBString(pObject->pszDescription);
						WCHAR *wtc = WideStringFromMBString(pObject->pszTextualConvention);
                  pNewObj->setInfo(pObject->iSyntax, pObject->iStatus, pObject->iAccess,
                     wdescr, wtc, (pObject->index != nullptr) ? indexBuffer.cstr() : nullptr);
						MemFree(wdescr);
						MemFree(wtc);
#else
                  pNewObj->setInfo(pObject->iSyntax, pObject->iStatus, pObject->iAccess,
                     pObject->pszDescription, pObject->pszTextualConvention, (pObject->index != nullptr) ? indexBuffer.cstr() : nullptr);
#endif
                  if (pNewObj->getName() == nullptr)
						{
#ifdef UNICODE
							WCHAR *wname = WideStringFromMBString(pObject->pszName);
                     pNewObj->setName(wname);
							MemFree(wname);
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
int ParseMIBFiles(StringList *fileList, SNMP_MIBObject **ppRoot)
{
   int i, nRet;
   SNMP_MIBObject *pRoot;

   _tprintf(_T("Parsing source files:\n"));
   ObjectArray<MP_MODULE> *moduleList = new ObjectArray<MP_MODULE>(16, 16, Ownership::True);
   for(i = 0; i < fileList->size(); i++)
   {
      const TCHAR *file = fileList->get(i);
      _tprintf(_T("   %s\n"), file);
      MP_MODULE *module = ParseMIB(file);
      if (module == NULL)
      {
         nRet = SNMP_MPE_PARSE_ERROR;
         goto parse_error;
      }
      moduleList->add(module);
   }

   _tprintf(_T("Resolving imports:\n"));
   for(i = 0; i < moduleList->size(); i++)
   {
      MP_MODULE *module = moduleList->get(i);
      _tprintf(_T("   %hs\n"), module->pszName);
      ResolveImports(moduleList, module);
   }

   _tprintf(_T("Resolving object identifiers:\n"));
   for(i = 0; i < moduleList->size(); i++)
   {
      MP_MODULE *module = moduleList->get(i);
      _tprintf(_T("   %hs\n"), module->pszName);
      ResolveObjects(moduleList, module);
   }

   _tprintf(_T("Creating MIB tree:\n"));
   moduleList->setOwner(Ownership::False);
   pRoot = new SNMP_MIBObject;
   for(i = 0; i < moduleList->size(); i++)
   {
      MP_MODULE *module = moduleList->get(i);
      _tprintf(_T("   %hs\n"), module->pszName);
      BuildMIBTree(pRoot, module);
      delete module;
   }

   *ppRoot = pRoot;
   delete moduleList;
   return SNMP_MPE_SUCCESS;

parse_error:
   *ppRoot = NULL;
   delete moduleList;
   return nRet;
}
