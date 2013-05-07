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
		ConfigEntryList *events = eventsRoot->getSubEntries(_T("event#*"));
		for(int i = 0; i < events->getSize(); i++)
		{
			ConfigEntry *event = events->getEntry(i);
			if (!_tcsicmp(event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")), name))
				return true;
		}
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
	ConfigEntryList *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
	for(int i = 0; i < thresholds->getSize(); i++)
	{
		ConfigEntry *threshold = thresholds->getEntry(i);
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
	ConfigEntryList *dcis = dcRoot->getSubEntries(_T("dci#*"));
	const TCHAR *name = root->getSubEntryValue(_T("name"), 0, _T("<unnamed>"));
	for(int i = 0; i < dcis->getSize(); i++)
	{
		if (!ValidateDci(config, dcis->getEntry(i), name, errorText, errorTextLen))
		{
			success = false;
			break;
		}
	}
	delete dcis;
	return success;
}

/**
 * Validate configuration before import
 */
bool ValidateConfig(Config *config, DWORD flags, TCHAR *errorText, int errorTextLen)
{
   int i;
	ConfigEntryList *events = NULL, *traps = NULL, *templates = NULL;
	ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot;
   bool success = false;

	DbgPrintf(4, _T("ValidateConfig() called, flags = 0x%04X"), flags);

   // Validate events
	eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != NULL)
	{
		events = eventsRoot->getSubEntries(_T("event#*"));
		for(i = 0; i < events->getSize(); i++)
		{
			ConfigEntry *event = events->getEntry(i);
			DbgPrintf(6, _T("ValidateConfig(): validating event %s"), event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));
		
			DWORD code = event->getSubEntryValueUInt(_T("code"));
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
		for(i = 0; i < traps->getSize(); i++)
		{
			ConfigEntry *trap = traps->getEntry(i);
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
		for(i = 0; i < templates->getSize(); i++)
		{
			if (!ValidateTemplate(config, templates->getEntry(i), errorText, errorTextLen))
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
static DWORD ImportEvent(ConfigEntry *event)
{
	const TCHAR *name = event->getSubEntryValue(_T("name"));
	if (name == NULL)
		return RCC_INTERNAL_ERROR;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	if (hdb == NULL)
		return RCC_DB_FAILURE;

	DWORD code = event->getSubEntryValueUInt(_T("code"), 0, 0);
	if ((code == 0) || (code >= FIRST_USER_EVENT_ID))
		code = CreateUniqueId(IDG_EVENT);

	// Create or update event template in database
   const TCHAR *msg = event->getSubEntryValue(_T("message"), 0, name);
   const TCHAR *descr = event->getSubEntryValue(_T("description"));
	TCHAR query[8192];
   if (IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), code))
   {
      _sntprintf(query, 8192, _T("UPDATE event_cfg SET event_name=%s,severity=%d,flags=%d,message=%s,description=%s WHERE event_code=%d"),
                 (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueInt(_T("severity")),
					  event->getSubEntryValueInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
					  (const TCHAR *)DBPrepareString(hdb, descr), code);
   }
   else
   {
      _sntprintf(query, 8192, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
                              _T("message,description) VALUES (%d,%s,%d,%d,%s,%s)"),
                 code, (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueInt(_T("severity")),
					  event->getSubEntryValueInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
					  (const TCHAR *)DBPrepareString(hdb, descr));
   }
	DWORD rcc = DBQuery(hdb, query) ? RCC_SUCCESS : RCC_DB_FAILURE;

	DBConnectionPoolReleaseConnection(hdb);
	return rcc;
}

/**
 * Import SNMP trap configuration
 */
static DWORD ImportTrap(ConfigEntry *trap)
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

	DWORD oid[256];
	tc.dwOidLen = SNMPParseOID(trap->getSubEntryValue(_T("oid"), 0, _T("")), oid, 256);
	tc.pdwObjectId = oid;
	if (tc.dwOidLen == 0)
		return RCC_INTERNAL_ERROR;

	ConfigEntry *parametersRoot = trap->findEntry(_T("parameters"));
	if (parametersRoot != NULL)
	{
		ConfigEntryList *parameters = parametersRoot->getOrderedSubEntries(_T("parameter#*"));
		if (parameters->getSize() > 0)
		{
			tc.dwNumMaps = parameters->getSize();
			tc.pMaps = (NXC_OID_MAP *)malloc(sizeof(NXC_OID_MAP) * tc.dwNumMaps);
			for(int i = 0; i < parameters->getSize(); i++)
			{
				ConfigEntry *parameter = parameters->getEntry(i);

				int position = parameter->getSubEntryValueInt(_T("position"), 0, -1);
				if (position > 0)
				{
					// Positional parameter
					tc.pMaps[i].pdwObjectId = NULL;
					tc.pMaps[i].dwOidLen = (DWORD)position | 0x80000000;
				}
				else
				{
					// OID parameter
					DWORD temp[256];
					tc.pMaps[i].dwOidLen = SNMPParseOID(parameter->getSubEntryValue(_T("oid"), 0, _T("")), temp, 256);
					tc.pMaps[i].pdwObjectId = (DWORD *)nx_memdup(temp, sizeof(DWORD) * tc.pMaps[i].dwOidLen);
				}
				nx_strncpy(tc.pMaps[i].szDescription, parameter->getSubEntryValue(_T("description"), 0, _T("")), MAX_DB_STRING);
				tc.pMaps[i].dwFlags = parameter->getSubEntryValueUInt(_T("flags"), 0, 0);
			}
		}
		delete parameters;
	}

	DWORD rcc = CreateNewTrap(&tc);

	// Cleanup
	for(DWORD i = 0; i < tc.dwNumMaps; i++)
		safe_free(tc.pMaps[i].pdwObjectId);

	return rcc;
}

/**
 * Import configuration
 */
DWORD ImportConfig(Config *config, DWORD flags)
{
	ConfigEntryList *events = NULL, *traps = NULL, *templates = NULL;
	ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot;
	DWORD rcc = RCC_SUCCESS;
	int i;

   DbgPrintf(4, _T("ImportConfig() called, flags=0x%04X"), flags);

   // Import events
	eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != NULL)
	{
		events = eventsRoot->getSubEntries(_T("event#*"));
		DbgPrintf(5, _T("ImportConfig(): %d events to import"), events->getSize());
		for(i = 0; i < events->getSize(); i++)
		{
			rcc = ImportEvent(events->getEntry(i));
			if (rcc != RCC_SUCCESS)
				goto stop_processing;
		}

		if (events->getSize() > 0)
		{
			ReloadEvents();
			NotifyClientSessions(NX_NOTIFY_EVENTDB_CHANGED, 0);
		}
		DbgPrintf(5, _T("ImportConfig(): events imported"));
	}

	// Import traps
	trapsRoot = config->getEntry(_T("/traps"));
	if (trapsRoot != NULL)
	{
		traps = trapsRoot->getSubEntries(_T("trap#*"));
		DbgPrintf(5, _T("ImportConfig(): %d SNMP traps to import"), traps->getSize());
		for(i = 0; i < traps->getSize(); i++)
		{
			rcc = ImportTrap(traps->getEntry(i));
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
		for(i = 0; i < templates->getSize(); i++)
		{
			Template *object = new Template(templates->getEntry(i));
			NetObjInsert(object, TRUE);
			object->AddParent(g_pTemplateRoot);
			g_pTemplateRoot->AddChild(object);
			object->unhide();
		}
	}

stop_processing:
	delete events;
	delete traps;
	delete templates;

	DbgPrintf(4, _T("ImportConfig() finished, rcc = %d"), rcc);
	return rcc;
}
