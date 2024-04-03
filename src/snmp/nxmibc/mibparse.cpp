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
static MP_MODULE *FindModuleByName(const ObjectArray<MP_MODULE>& moduleList, const char *name)
{
   for(int i = 0; i < moduleList.size(); i++)
      if (!strcmp(moduleList.get(i)->pszName, name))
         return moduleList.get(i);
   return nullptr;
}

/**
 * Find object in module
 */
static MP_OBJECT *FindObjectByName(MP_MODULE *module, const char *name, int *pnIndex)
{
   int i;
   for(i = (pnIndex != nullptr) ? *pnIndex : 0; i < module->pObjectList->size(); i++)
   {
		MP_OBJECT *pObject = module->pObjectList->get(i);
      if (!strcmp(pObject->pszName, name))
		{
			if (pnIndex != nullptr)
				*pnIndex = i + 1;
         return pObject;
		}
   }
	if (pnIndex != nullptr)
		*pnIndex = i;
   return nullptr;
}

/**
 * Find imported object in module
 */
static MP_OBJECT *FindImportedObjectByName(MP_MODULE *module, const char *name, MP_MODULE **ppImportModule)
{
   for(int i = 0; i < module->pImportList->size(); i++)
   {
      MP_IMPORT_MODULE *import = module->pImportList->get(i);
      for(int j = 0; j < import->symbols->size(); j++)
         if (!strcmp((char *)import->symbols->get(j), name))
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
static MP_MODULE *FindNextImportModule(const ObjectArray<MP_MODULE>& moduleList, MP_MODULE *module, const char *symbol)
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
static void ResolveImports(const ObjectArray<MP_MODULE>& moduleList, MP_MODULE *module)
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
                     ReportError(ERR_UNRESOLVED_IMPORT, module->pszName, pszSymbol);
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
         ReportError(ERR_UNRESOLVED_MODULE, module->pszName, pImport->name);
      }
   }
}

/**
 * Build full OID for object
 */
static bool BuildFullOID(MP_MODULE *module, MP_OBJECT *pObject, StringSet *unresolvedSymbols)
{
   MP_SUBID *pSubId;
   MP_OBJECT *pParent;

   int iLen = pObject->oid->size();
   while(iLen > 0)
   {
      pSubId = pObject->oid->get(iLen - 1);
      if (!pSubId->bResolved)
      {
         pParent = FindObjectByName(module, pSubId->pszName, nullptr);
         if (pParent != nullptr)
         {
            if (!pParent->valid)
               return false;
            if (!BuildFullOID(module, pParent, unresolvedSymbols))
               return false;
         }
         else
         {
            MP_MODULE *importModule;
            pParent = FindImportedObjectByName(module, pSubId->pszName, &importModule);
            if (pParent != nullptr)
            {
               if (!pParent->valid)
                  return false;
               if (!BuildFullOID(importModule, pParent, unresolvedSymbols))
                  return false;
            }
            else
            {
               pParent = GetBuiltinObject(pSubId->pszName);
            }
         }
         if (pParent != nullptr)
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
            // Prevent multiple reports on same unresolved symbol
            TCHAR key[1024];
            _sntprintf(key, 1024, _T("%hs::%hs"), module->pszName, pSubId->pszName);
            if (!unresolvedSymbols->contains(key))
            {
               ReportError(ERR_UNRESOLVED_SYMBOL, module->pszName, pSubId->pszName);
               unresolvedSymbols->add(key);
            }
            return false;
         }
      }
      else
      {
         iLen--;
      }
   }
   return true;
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
      ReportError(ERR_UNRESOLVED_SYNTAX, module->pszName, pObject->pszDataType, pObject->pszName);
   }
}

/**
 * Resolve object identifiers
 */
static void ResolveObjects(MP_MODULE *module)
{
   StringSet unresolvedSymbols;
   for(int i = 0; i < module->pObjectList->size(); i++)
   {
      MP_OBJECT *object = module->pObjectList->get(i);
      if (object->iType == MIBC_OBJECT)
      {
         if (BuildFullOID(module, object, &unresolvedSymbols))
         {
            ResolveSyntax(module, object);
         }
         else
         {
            object->valid = false;
         }
      }
   }
}

/**
 * Build MIB tree from object list
 */
static void BuildMIBTree(SNMP_MIBObject *root, MP_MODULE *module)
{
   StringBuffer indexBuffer;
   for(int i = 0; i < module->pObjectList->size(); i++)
   {
      MP_OBJECT *pObject = module->pObjectList->get(i);
      if ((pObject->iType == MIBC_OBJECT) && pObject->valid)
      {
         int iLen = pObject->oid->size();
         SNMP_MIBObject *pCurrObj = root;
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

#ifdef UNICODE

/**
 * Mark processing step
 */
static inline void MarkStep(const WCHAR *name)
{
   WriteToTerminalEx(_T("\x1b[34;1m=> %-74s\x1b[0m\r"), name);
}

#endif

/**
 * Mark processing step
 */
static inline void MarkStep(const char *name)
{
   if (g_terminalOutput)
      WriteToTerminalEx(_T("\x1b[34;1m=> %-74hs\x1b[0m\r"), name);
}

/**
 * Start processing stage
 */
static inline void StartStage(const TCHAR *message)
{
   if (g_machineParseableOutput)
      _tprintf(_T("P:%s\n"), message);

   else
      WriteToTerminalEx(_T("\x1b[1m%s\x1b[0m\n"), message);
}

/**
 * Complete processing stage
 */
static inline void CompleteStage()
{
   if (g_terminalOutput)
      WriteToTerminalEx(_T("   %-74s\r"), _T(""));
}

/**
 * Interface to parser
 */
int ParseMIBFiles(StringList *fileList, SNMP_MIBObject **rootObject)
{
   *rootObject = nullptr;

   ObjectArray<MP_MODULE> moduleList(16, 16, Ownership::True);

   StartStage(_T("Parsing source files"));
   for(int i = 0; i < fileList->size(); i++)
   {
      const TCHAR *file = fileList->get(i);

      if (g_terminalOutput)
         MarkStep(ShortenFilePathForDisplay(file, 74));
      else if (g_machineParseableOutput)
         _tprintf(_T("F:%s\n"), file);
      else
         _tprintf(_T("Parsing MIB file %s\n"), file);

      MP_MODULE *module = ParseMIB(file);
      if (module == nullptr)
      {
         return SNMP_MPE_PARSE_ERROR;
      }
      moduleList.add(module);
   }
   CompleteStage();

   StartStage(_T("Resolving imports"));
   for(int i = 0; i < moduleList.size(); i++)
   {
      MP_MODULE *module = moduleList.get(i);
      MarkStep(module->pszName);
      ResolveImports(moduleList, module);
   }
   CompleteStage();

   StartStage(_T("Resolving object identifiers"));
   for(int i = 0; i < moduleList.size(); i++)
   {
      MP_MODULE *module = moduleList.get(i);
      MarkStep(module->pszName);
      ResolveObjects(module);
   }
   CompleteStage();

   StartStage(_T("Creating MIB tree"));
   moduleList.setOwner(Ownership::False);
   auto root = new SNMP_MIBObject;
   for(int i = 0; i < moduleList.size(); i++)
   {
      MP_MODULE *module = moduleList.get(i);
      MarkStep(module->pszName);
      BuildMIBTree(root, module);
      delete module;
   }
   CompleteStage();

   *rootObject = root;
   return SNMP_MPE_SUCCESS;
}
