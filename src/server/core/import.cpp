/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: import.cpp
**
**/

#include "nxcore.h"

/**
 * Check if given event exist either in server configuration or in configuration being imported
 */
static bool IsEventExist(const TCHAR *name, Config *config)
{
	if (FindEventTemplateByName(name) != NULL)
		return true;

	ConfigEntry *eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != NULL)
	{
		ObjectArray<ConfigEntry> *events = eventsRoot->getSubEntries(_T("event#*"));
		for(int i = 0; i < events->size(); i++)
		{
			ConfigEntry *event = events->get(i);
			if (!_tcsicmp(event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")), name))
         {
            delete events;
				return true;
         }
		}
      delete events;
	}

	return false;
}

/**
 * Validate DCI from template
 */
static bool ValidateDci(Config *config, ConfigEntry *dci, const TCHAR *templateName, TCHAR *errorText, int errorTextLen)
{
	ConfigEntry *thresholdsRoot = dci->findEntry(_T("thresholds"));
	if (thresholdsRoot == NULL)
		return true;

	bool success = true;
	ObjectArray<ConfigEntry> *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
	for(int i = 0; i < thresholds->size(); i++)
	{
		ConfigEntry *threshold = thresholds->get(i);
		if (!IsEventExist(threshold->getSubEntryValue(_T("activationEvent")), config))
		{
			_sntprintf(errorText, errorTextLen,
						  _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"activationEvent\" refers to unknown event"),
						  templateName, dci->getSubEntryValue(_T("description"), 0, _T("<unnamed>")), i + 1);
			success = false;
			break;
		}
		if (!IsEventExist(threshold->getSubEntryValue(_T("deactivationEvent")), config))
		{
			_sntprintf(errorText, errorTextLen,
						  _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"deactivationEvent\" refers to unknown event"),
						  templateName, dci->getSubEntryValue(_T("description"), 0, _T("<unnamed>")), i + 1);
			success = false;
			break;
		}
	}
	delete thresholds;
	return success;
}

/**
 * Validate template
 */
static bool ValidateTemplate(Config *config, ConfigEntry *root, TCHAR *errorText, int errorTextLen)
{
	DbgPrintf(6, _T("ValidateConfig(): validating template \"%s\""), root->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));

	ConfigEntry *dcRoot = root->findEntry(_T("dataCollection"));
	if (dcRoot == NULL)
		return true;

	bool success = true;
	const TCHAR *name = root->getSubEntryValue(_T("name"), 0, _T("<unnamed>"));

	ObjectArray<ConfigEntry> *dcis = dcRoot->getSubEntries(_T("dci#*"));
	for(int i = 0; i < dcis->size(); i++)
	{
		if (!ValidateDci(config, dcis->get(i), name, errorText, errorTextLen))
		{
			success = false;
			break;
		}
	}
	delete dcis;

   if (success)
   {
	   ObjectArray<ConfigEntry> *dctables = dcRoot->getSubEntries(_T("dctable#*"));
	   for(int i = 0; i < dctables->size(); i++)
	   {
		   if (!ValidateDci(config, dctables->get(i), name, errorText, errorTextLen))
		   {
			   success = false;
			   break;
		   }
	   }
	   delete dctables;
   }
   
   return success;
}

/**
 * Validate configuration before import
 */
bool ValidateConfig(Config *config, UINT32 flags, TCHAR *errorText, int errorTextLen)
{
   int i;
	ObjectArray<ConfigEntry> *events = NULL, *traps = NULL, *templates = NULL;
	ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot;
   bool success = false;

	DbgPrintf(4, _T("ValidateConfig() called, flags = 0x%04X"), flags);

   // Validate events
	eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != NULL)
	{
		events = eventsRoot->getSubEntries(_T("event#*"));
		for(i = 0; i < events->size(); i++)
		{
			ConfigEntry *event = events->get(i);
			DbgPrintf(6, _T("ValidateConfig(): validating event %s"), event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));
		
			UINT32 code = event->getSubEntryValueAsUInt(_T("code"));
			if ((code >= FIRST_USER_EVENT_ID) || (code == 0))
			{
				ConfigEntry *e = event->findEntry(_T("name"));
				if (e != NULL)
				{
					EVENT_TEMPLATE *pEvent = FindEventTemplateByName(e->getValue());
					if (pEvent != NULL)
					{
						if (!(flags & CFG_IMPORT_REPLACE_EVENT_BY_NAME))
						{
							_sntprintf(errorText, errorTextLen, _T("Event with name %s already exist"), e->getValue());
							goto stop_processing;
						}
					}
				}
				else
				{
					_sntprintf(errorText, errorTextLen, _T("Mandatory attribute \"name\" missing for entry %s"), event->getName());
					goto stop_processing;
				}
			}
			else
			{
				EVENT_TEMPLATE *pEvent = FindEventTemplateByCode(code);
				if (pEvent != NULL)
				{
					if (!(flags & CFG_IMPORT_REPLACE_EVENT_BY_CODE))
					{
						_sntprintf(errorText, errorTextLen, _T("Event with code %d already exist (existing event name: %s; new event name: %s)"),
						           pEvent->dwCode, pEvent->szName, event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));
						goto stop_processing;
					}
				}
			}
		}
	}

	// Validate traps
	trapsRoot = config->getEntry(_T("/traps"));
	if (trapsRoot != NULL)
	{
		traps = trapsRoot->getSubEntries(_T("trap#*"));
		for(i = 0; i < traps->size(); i++)
		{
			ConfigEntry *trap = traps->get(i);
			DbgPrintf(6, _T("ValidateConfig(): validating trap \"%s\""), trap->getSubEntryValue(_T("description"), 0, _T("<unnamed>")));
			if (!IsEventExist(trap->getSubEntryValue(_T("event")), config))
			{
				_sntprintf(errorText, errorTextLen, _T("Trap \"%s\" references unknown event"), trap->getSubEntryValue(_T("description"), 0, _T("")));
				goto stop_processing;
			}
		}
	}

	// Validate templates
	templatesRoot = config->getEntry(_T("/templates"));
	if (templatesRoot != NULL)
	{
		templates = templatesRoot->getSubEntries(_T("template#*"));
		for(i = 0; i < templates->size(); i++)
		{
			if (!ValidateTemplate(config, templates->get(i), errorText, errorTextLen))
				goto stop_processing;
		}
	}

   success = true;

stop_processing:
	delete events;
	delete traps;
	delete templates;

	DbgPrintf(4, _T("ValidateConfig() finished, status = %d"), success);
	if (!success)
		DbgPrintf(4, _T("ValidateConfig(): %s"), errorText);
   return success;
}

/**
 * Import event
 */
static UINT32 ImportEvent(ConfigEntry *event)
{
	const TCHAR *name = event->getSubEntryValue(_T("name"));
	if (name == NULL)
		return RCC_INTERNAL_ERROR;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	UINT32 code = event->getSubEntryValueAsUInt(_T("code"), 0, 0);
	if ((code == 0) || (code >= FIRST_USER_EVENT_ID))
		code = CreateUniqueId(IDG_EVENT);

	// Create or update event template in database
   const TCHAR *msg = event->getSubEntryValue(_T("message"), 0, name);
   const TCHAR *descr = event->getSubEntryValue(_T("description"));
	TCHAR query[8192];
   if (IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), code))
   {
      _sntprintf(query, 8192, _T("UPDATE event_cfg SET event_name=%s,severity=%d,flags=%d,message=%s,description=%s WHERE event_code=%d"),
                 (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueAsInt(_T("severity")),
					  event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
					  (const TCHAR *)DBPrepareString(hdb, descr), code);
   }
   else
   {
      _sntprintf(query, 8192, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
                              _T("message,description) VALUES (%d,%s,%d,%d,%s,%s)"),
                 code, (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueAsInt(_T("severity")),
					  event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
					  (const TCHAR *)DBPrepareString(hdb, descr));
   }
	UINT32 rcc = DBQuery(hdb, query) ? RCC_SUCCESS : RCC_DB_FAILURE;

	DBConnectionPoolReleaseConnection(hdb);
	return rcc;
}

/**
 * Import SNMP trap configuration
 */
static UINT32 ImportTrap(ConfigEntry *trap)
{
	NXC_TRAP_CFG_ENTRY tc;
	EVENT_TEMPLATE *event;

	event = FindEventTemplateByName(trap->getSubEntryValue(_T("event"), 0, _T("")));
	if (event == NULL)
		return RCC_INTERNAL_ERROR;

	memset(&tc, 0, sizeof(NXC_TRAP_CFG_ENTRY));
	tc.dwEventCode = event->dwCode;
	nx_strncpy(tc.szDescription, trap->getSubEntryValue(_T("description"), 0, _T("")), MAX_DB_STRING);
	nx_strncpy(tc.szUserTag, trap->getSubEntryValue(_T("userTag"), 0, _T("")), MAX_USERTAG_LENGTH);

	UINT32 oid[256];
	tc.dwOidLen = (UINT32)SNMPParseOID(trap->getSubEntryValue(_T("oid"), 0, _T("")), oid, 256);
	tc.pdwObjectId = oid;
	if (tc.dwOidLen == 0)
		return RCC_INTERNAL_ERROR;

	ConfigEntry *parametersRoot = trap->findEntry(_T("parameters"));
	if (parametersRoot != NULL)
	{
		ObjectArray<ConfigEntry> *parameters = parametersRoot->getOrderedSubEntries(_T("parameter#*"));
		if (parameters->size() > 0)
		{
			tc.dwNumMaps = parameters->size();
			tc.pMaps = (NXC_OID_MAP *)malloc(sizeof(NXC_OID_MAP) * tc.dwNumMaps);
			for(int i = 0; i < parameters->size(); i++)
			{
				ConfigEntry *parameter = parameters->get(i);

				int position = parameter->getSubEntryValueAsInt(_T("position"), 0, -1);
				if (position > 0)
				{
					// Positional parameter
					tc.pMaps[i].pdwObjectId = NULL;
					tc.pMaps[i].dwOidLen = (UINT32)position | 0x80000000;
				}
				else
				{
					// OID parameter
					UINT32 temp[256];
					tc.pMaps[i].dwOidLen = (UINT32)SNMPParseOID(parameter->getSubEntryValue(_T("oid"), 0, _T("")), temp, 256);
					tc.pMaps[i].pdwObjectId = (UINT32 *)nx_memdup(temp, sizeof(UINT32) * tc.pMaps[i].dwOidLen);
				}
				nx_strncpy(tc.pMaps[i].szDescription, parameter->getSubEntryValue(_T("description"), 0, _T("")), MAX_DB_STRING);
				tc.pMaps[i].dwFlags = parameter->getSubEntryValueAsUInt(_T("flags"), 0, 0);
			}
		}
		delete parameters;
	}

	UINT32 rcc = CreateNewTrap(&tc);

	// Cleanup
	for(UINT32 i = 0; i < tc.dwNumMaps; i++)
		safe_free(tc.pMaps[i].pdwObjectId);

	return rcc;
}

/**
 * Find (and create as necessary) parent object for imported template
 */
NetObj *FindTemplateRoot(ConfigEntry *config)
{
	ConfigEntry *pathRoot = config->findEntry(_T("path"));
	if (pathRoot == NULL)
      return g_pTemplateRoot;  // path not specified in config

   NetObj *parent = g_pTemplateRoot;
	ObjectArray<ConfigEntry> *path = pathRoot->getSubEntries(_T("element#*"));
	for(int i = 0; i < path->size(); i++)
	{
      const TCHAR *name = path->get(i)->getValue();
      NetObj *o = parent->findChildObject(name, OBJECT_TEMPLATEGROUP);
      if (o == NULL)
      {
         o = new TemplateGroup(name);
         NetObjInsert(o, TRUE);
         o->AddParent(parent);
         parent->AddChild(o);
         o->unhide();
         o->calculateCompoundStatus();	// Force status change to NORMAL
      }
      parent = o;
   }
   delete path;
   return parent;
}

/**
 * Import configuration
 */
UINT32 ImportConfig(Config *config, UINT32 flags)
{
	ObjectArray<ConfigEntry> *events = NULL, *traps = NULL, *templates = NULL, *rules = NULL, *scripts = NULL, *objectTools = NULL, *summaryTables = NULL;
	ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot, *rulesRoot, *scriptsRoot, *objectToolsRoot, *summaryTablesRoot;
	UINT32 rcc = RCC_SUCCESS;
	int i;

   DbgPrintf(4, _T("ImportConfig() called, flags=0x%04X"), flags);

   // Import events
	eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != NULL)
	{
		events = eventsRoot->getSubEntries(_T("event#*"));
		DbgPrintf(5, _T("ImportConfig(): %d events to import"), events->size());
		for(i = 0; i < events->size(); i++)
		{
			rcc = ImportEvent(events->get(i));
			if (rcc != RCC_SUCCESS)
				goto stop_processing;
		}

		if (events->size() > 0)
		{
			ReloadEvents();
			NotifyClientSessions(NX_NOTIFY_RELOAD_EVENT_DB, 0);
		}
		DbgPrintf(5, _T("ImportConfig(): events imported"));
	}

	// Import traps
	trapsRoot = config->getEntry(_T("/traps"));
	if (trapsRoot != NULL)
	{
		traps = trapsRoot->getSubEntries(_T("trap#*"));
		DbgPrintf(5, _T("ImportConfig(): %d SNMP traps to import"), traps->size());
		for(i = 0; i < traps->size(); i++)
		{
			rcc = ImportTrap(traps->get(i));
			if (rcc != RCC_SUCCESS)
				goto stop_processing;
		}
		DbgPrintf(5, _T("ImportConfig(): SNMP traps imported"));
	}

	// Import templates
	templatesRoot = config->getEntry(_T("/templates"));
	if (templatesRoot != NULL)
	{
		templates = templatesRoot->getSubEntries(_T("template#*"));
		for(i = 0; i < templates->size(); i++)
		{
         NetObj *parent = FindTemplateRoot(templates->get(i));
			Template *object = new Template(templates->get(i));
			NetObjInsert(object, TRUE);
			object->AddParent(parent);
			parent->AddChild(object);
			object->unhide();
		}
		DbgPrintf(5, _T("ImportConfig(): templates imported"));
	}

	// Import rules
	rulesRoot = config->getEntry(_T("/rules"));
	if (rulesRoot != NULL)
	{
		rules = rulesRoot->getOrderedSubEntries(_T("rule#*"));
		for(i = 0; i < rules->size(); i++)
		{
         EPRule *rule = new EPRule(rules->get(i));
         g_pEventPolicy->importRule(rule);
		}
		DbgPrintf(5, _T("ImportConfig(): event processing policy rules imported"));
	}

	// Import scripts
	scriptsRoot = config->getEntry(_T("/scripts"));
	if (scriptsRoot != NULL)
	{
		scripts = scriptsRoot->getSubEntries(_T("script#*"));
		for(i = 0; i < scripts->size(); i++)
		{
         ImportScript(scripts->get(i));
		}
		DbgPrintf(5, _T("ImportConfig(): scripts imported"));
	}

	// Import object tools
	objectToolsRoot = config->getEntry(_T("/objectTools"));
	if (objectToolsRoot != NULL)
	{
		objectTools = objectToolsRoot->getSubEntries(_T("objectTool#*"));
		for(i = 0; i < objectTools->size(); i++)
		{
         ImportObjectTool(objectTools->get(i));
		}
		DbgPrintf(5, _T("ImportConfig(): object tools imported"));
	}

	// Import summary tables
	summaryTablesRoot = config->getEntry(_T("/dciSummaryTables"));
	if (summaryTablesRoot != NULL)
	{
		summaryTables = summaryTablesRoot->getSubEntries(_T("table#*"));
		for(i = 0; i < summaryTables->size(); i++)
		{
         ImportSummaryTable(summaryTables->get(i));
		}
		DbgPrintf(5, _T("ImportConfig(): DCI summary tables imported"));
	}

stop_processing:
	delete events;
	delete traps;
	delete templates;
   delete rules;
   delete scripts;
   delete objectTools;
   delete summaryTables;

	DbgPrintf(4, _T("ImportConfig() finished, rcc = %d"), rcc);
	return rcc;
}
