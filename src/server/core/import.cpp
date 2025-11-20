/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
#include <nxcore_websvc.h>
#include <asset_management.h>

#define DEBUG_TAG _T("import")

/**
 * Check if given event exist either in server configuration or in configuration being imported
 */
static bool IsEventExist(const TCHAR *name, const Config& config)
{
   if (name == nullptr)
      return false;

   shared_ptr<EventTemplate> e = FindEventTemplateByName(name);
	if (e != nullptr)
		return true;

	ConfigEntry *eventsRoot = config.getEntry(_T("/events"));
	if (eventsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> events = eventsRoot->getSubEntries(_T("event#*"));
      for (int i = 0; i < events->size(); i++)
      {
         ConfigEntry *event = events->get(i);
         if (!_tcsicmp(event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")), name))
            return true;
      }
   }
   return false;
}

/**
 * Validate DCI from template
 */
static bool ValidateDci(const Config& config, const ConfigEntry *dci, const TCHAR *templateName, ImportContext *context)
{
	ConfigEntry *thresholdsRoot = dci->findEntry(_T("thresholds"));
	if (thresholdsRoot == nullptr)
		return true;

	bool success = true;
   unique_ptr<ObjectArray<ConfigEntry>> thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
   for (int i = 0; i < thresholds->size(); i++)
   {
      ConfigEntry *threshold = thresholds->get(i);

      const TCHAR *eventName = threshold->getSubEntryValue(_T("activationEvent"));
      if ((eventName != nullptr) && (*eventName != 0) && !IsEventExist(eventName, config))
      {
         context->log(NXLOG_ERROR, _T("ValidateDci()"),
            _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"activationEvent\" refers to unknown event"),
            templateName, dci->getSubEntryValue(_T("description"), 0, _T("<unnamed>")), i + 1);
         success = false;
      }

      eventName = threshold->getSubEntryValue(_T("deactivationEvent"));
      if ((eventName != nullptr) && (*eventName != 0) && !IsEventExist(eventName, config))
      {
         context->log(NXLOG_ERROR, _T("ValidateDci()"),
            _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"deactivationEvent\" refers to unknown event"),
            templateName, dci->getSubEntryValue(_T("description"), 0, _T("<unnamed>")), i + 1);
         success = false;
      }
   }
   return success;
}

/**
 * Validate template
 */
static bool ValidateTemplate(const Config& config, const ConfigEntry *root, ImportContext *context)
{
   context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validating template \"%s\""), root->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));
	ConfigEntry *dcRoot = root->findEntry(_T("dataCollection"));
	if (dcRoot == nullptr)
		return true;

	bool success = true;
	const TCHAR *name = root->getSubEntryValue(_T("name"), 0, _T("<unnamed>"));

   unique_ptr<ObjectArray<ConfigEntry>> dcis = dcRoot->getSubEntries(_T("dci#*"));
   for (int i = 0; i < dcis->size(); i++)
   {
      if (!ValidateDci(config, dcis->get(i), name, context))
      {
         success = false;
      }
   }

   unique_ptr<ObjectArray<ConfigEntry>> dctables = dcRoot->getSubEntries(_T("dctable#*"));
   for (int i = 0; i < dctables->size(); i++)
   {
      if (!ValidateDci(config, dctables->get(i), name, context))
      {
         success = false;
      }
   }
   return success;
}

/**
 * Validate configuration before import
 */
bool ValidateConfig(const Config& config, uint32_t flags, ImportContext *context)
{
   ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot;
   bool success = true;

	nxlog_debug_tag(DEBUG_TAG, 4, _T("ValidateConfig() called, flags = 0x%04X"), flags);

   // Validate events
	eventsRoot = config.getEntry(_T("/events"));
	if (eventsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> events = eventsRoot->getSubEntries(_T("event#*"));
      for (int i = 0; i < events->size(); i++)
      {
         ConfigEntry *event = events->get(i);
         context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validating event \"%s\""), event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));

         uint32_t code = event->getSubEntryValueAsUInt(_T("code"));
			if ((code >= FIRST_USER_EVENT_ID) || (code == 0))
			{
				ConfigEntry *e = event->findEntry(_T("name"));
				if (e == nullptr)
				{
				   context->log(NXLOG_ERROR, _T("ValidateConfig()"), _T("Mandatory attribute \"name\" missing for event template"));
					success = false;
				}
			}
      }
   }

   // Validate traps
   trapsRoot = config.getEntry(_T("/traps"));
	if (trapsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> traps = trapsRoot->getSubEntries(_T("trap#*"));
      for (int i = 0; i < traps->size(); i++)
      {
         ConfigEntry *trap = traps->get(i);
         context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validating SNMP trap definition \"%s\""), trap->getSubEntryValue(_T("description"), 0, _T("<unnamed>")));
         if (!IsEventExist(trap->getSubEntryValue(_T("event")), config))
			{
            context->log(NXLOG_ERROR, _T("ValidateConfig()"), _T("SNMP trap definition \"%s\" refers to unknown event"), trap->getSubEntryValue(_T("description"), 0, _T("")));
            success = false;
			}
      }
   }

   // Validate templates
   templatesRoot = config.getEntry(_T("/templates"));
	if (templatesRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> templates = templatesRoot->getSubEntries(_T("template#*"));
      for (int i = 0; i < templates->size(); i++)
      {
         if (!ValidateTemplate(config, templates->get(i), context))
            success = false;
      }
   }

	context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validation %s"), success ? _T("successfully finished") : _T("failed"));
   return success;
}

/**
 * Import event
 */
static uint32_t ImportEvent(const ConfigEntry *event, bool overwrite, ImportContext *context)
{
	const TCHAR *name = event->getSubEntryValue(_T("name"));
	if (name == nullptr)
		return RCC_INTERNAL_ERROR;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	UINT32 code = 0;
	bool checkByName = false;
	uuid guid = event->getSubEntryValueAsUUID(_T("guid"));
	if (!guid.isNull())
	{
	   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT event_code FROM event_cfg WHERE guid=?"));
	   if (hStmt == nullptr)
	   {
	      DBConnectionPoolReleaseConnection(hdb);
	      return RCC_DB_FAILURE;
	   }
	   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
	   DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != nullptr)
	   {
	      if (DBGetNumRows(hResult) > 0)
	         code = DBGetFieldULong(hResult, 0, 0);
	      DBFreeResult(hResult);
	   }
	   DBFreeStatement(hStmt);
	   if (code != 0)
	   {
	      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Found existing event template with code %d and GUID %s"), code, guid.toString().cstr());
	   }
	   else
	   {
	      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Event template with GUID %s not found"), guid.toString().cstr());
	   }
	}
	else
	{
	   checkByName = true;
	   code = event->getSubEntryValueAsUInt(_T("code"), 0, 0);
	   if (code >= FIRST_USER_EVENT_ID)
	   {
	      context->log(NXLOG_WARNING, _T("ImportEvent()"), _T("Event template without GUID and code %d is not in system range"), code);
         code = 0;
	   }
	   else
	   {
	      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Using provided event code %d"), code);
	   }
	}

	// Create or update event template in database
   const TCHAR *msg = event->getSubEntryValue(_T("message"), 0, name);
   const TCHAR *descr = event->getSubEntryValue(_T("description"));
   const TCHAR *tags = event->getSubEntryValue(_T("tags"));
	TCHAR query[8192];
   if ((code != 0) && IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), code))
   {
      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Found existing event with code %d (%s)"), code, overwrite ? _T("updating") : _T("skipping"));
      if (overwrite)
      {
         _sntprintf(query, 8192, _T("UPDATE event_cfg SET event_name=%s,severity=%d,flags=%d,message=%s,description=%s,tags=%s WHERE event_code=%d"),
                    (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueAsInt(_T("severity")),
                    event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
                    (const TCHAR *)DBPrepareString(hdb, descr), (const TCHAR *)DBPrepareString(hdb, tags), code);
      }
      else
      {
         query[0] = 0;
      }
   }
   else if (checkByName && IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_name"), name))
   {
      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Found existing event with code %d (%s)"), code, overwrite ? _T("updating") : _T("skipping"));
      if (overwrite)
      {
         _sntprintf(query, 8192, _T("UPDATE event_cfg SET severity=%d,flags=%d,message=%s,description=%s,tags=%s WHERE event_name=%s"),
                    event->getSubEntryValueAsInt(_T("severity")),
                    event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
                    (const TCHAR *)DBPrepareString(hdb, descr), (const TCHAR *)DBPrepareString(hdb, tags), (const TCHAR *)DBPrepareString(hdb, name));
      }
      else
      {
         query[0] = 0;
      }
   }
   else
   {
      if (guid.isNull())
         guid = uuid::generate();
      if (code == 0)
         code = CreateUniqueId(IDG_EVENT);
      _sntprintf(query, 8192, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
                              _T("message,description,guid,tags) VALUES (%d,%s,%d,%d,%s,%s,'%s',%s)"),
                 code, (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueAsInt(_T("severity")),
					  event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
					  (const TCHAR *)DBPrepareString(hdb, descr), (const TCHAR *)guid.toString(), (const TCHAR *)DBPrepareString(hdb, tags));
      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Added new event: code=%d, name=%s, guid=%s"), code, name, (const TCHAR *)guid.toString());
   }
	uint32_t rcc = (query[0] != 0) ? (DBQuery(hdb, query) ? RCC_SUCCESS : RCC_DB_FAILURE) : RCC_SUCCESS;

	DBConnectionPoolReleaseConnection(hdb);
	return rcc;
}

/**
 * Import SNMP trap mappimg
 */
static uint32_t ImportTrapMapping(const ConfigEntry& trap, bool overwrite, ImportContext *context, bool nxslV5) // TODO transactions needed?
{
   uint32_t rcc = RCC_INTERNAL_ERROR;
   shared_ptr<EventTemplate> eventTemplate = FindEventTemplateByName(trap.getSubEntryValue(_T("event"), 0, _T("")));
	if (eventTemplate == nullptr)
		return rcc;

	uuid guid = trap.getSubEntryValueAsUUID(_T("guid"));
   if (guid.isNull())
   {
      guid = uuid::generate();
      context->log(NXLOG_INFO, _T("ImportTrap()"), _T("SNMP trap mapping GUID not found in configuration file, generated new GUID %s"), guid.toString().cstr());
   }
   uint32_t id = ResolveTrapMappingGuid(guid);
   if ((id != 0) && !overwrite)
   {
      context->log(NXLOG_INFO, _T("ImportTrap()"), _T("Skipping existing SNMP trap mapping with GUID %s"), guid.toString().cstr());
      return RCC_SUCCESS;
   }

	auto trapMapping = make_shared<SNMPTrapMapping>(trap, guid, id, eventTemplate->getCode(), nxslV5);
	if (!trapMapping->getOid().isValid())
	{
      context->log(NXLOG_ERROR, _T("ImportTrap()"), _T("SNMP trap mapping %s refers to invalid OID"), guid.toString().cstr());
		return rcc;
	}

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = (id == 0) ?
	   DBPrepare(hdb, _T("INSERT INTO snmp_trap_cfg (snmp_oid,event_code,description,user_tag,transformation_script,trap_id,guid) VALUES (?,?,?,?,?,?,?)")) :
	   DBPrepare(hdb, _T("UPDATE snmp_trap_cfg SET snmp_oid=?,event_code=?,description=?,user_tag=?,transformation_script=? WHERE trap_id=?"));

	if (hStmt != nullptr)
	{
	   TCHAR oid[1024];
	   trapMapping->getOid().toString(oid, 1024);
	   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
	   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, trapMapping->getEventCode());
	   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, trapMapping->getDescription(), DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, trapMapping->getEventTag(), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, trapMapping->getScriptSource(), DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, trapMapping->getId());
      if (id == 0)
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, trapMapping->getGuid());

      if (DBBegin(hdb))
      {
         if (DBExecute(hStmt) && trapMapping->saveParameterMapping(hdb))
         {
            AddTrapMappingToList(trapMapping);
            trapMapping->notifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_CREATED);
            rcc = RCC_SUCCESS;
            DBCommit(hdb);
         }
         else
         {
            DBRollback(hdb);
            rcc = RCC_DB_FAILURE;
         }
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
	}
	else
	{
	   rcc = RCC_DB_FAILURE;
	}

	DBConnectionPoolReleaseConnection(hdb);

	return rcc;
}

/**
 * Find (and create as necessary) parent object for imported template
 */
static shared_ptr<NetObj> FindTemplateRoot(const ConfigEntry *config)
{
	const ConfigEntry *pathRoot = config->findEntry(_T("path"));
	if (pathRoot == nullptr)
      return g_templateRoot;  // path not specified in config

   shared_ptr<NetObj> parent = g_templateRoot;
   unique_ptr<ObjectArray<ConfigEntry>> path = pathRoot->getSubEntries(_T("element#*"));
   for (int i = 0; i < path->size(); i++)
   {
      const TCHAR *name = path->get(i)->getValue();
      shared_ptr<NetObj> o = parent->findChildObject(name, OBJECT_TEMPLATEGROUP);
      if (o == nullptr)
      {
         o = make_shared<TemplateGroup>(name);
         NetObjInsert(o, true, false);
         NetObj::linkObjects(parent, o);
         o->unhide();
         o->calculateCompoundStatus();	// Force status change to NORMAL
      }
      parent = o;
   }
   return parent;
}

/**
 * Fill rule ordering array
 */
static ObjectArray<uuid> *GetRuleOrdering(const ConfigEntry *ruleOrdering)
{
   ObjectArray<uuid> *ordering = nullptr;
   if(ruleOrdering != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> rules = ruleOrdering->getOrderedSubEntries(_T("rule#*"));
      if (rules->size() > 0)
      {
         ordering = new ObjectArray<uuid>(0, 16, Ownership::True);
         for(int i = 0; i < rules->size(); i++)
         {
            ordering->add(new uuid(uuid::parse(rules->get(i)->getValue())));
         }
      }
   }

   return ordering;
}

/**
 * Delete template group if it is empty
 */
static void DeleteEmptyTemplateGroup(shared_ptr<NetObj> templateGroup)
{
   if (templateGroup->getChildCount() != 0)
      return;

   shared_ptr<NetObj> parent = templateGroup->getParents(OBJECT_TEMPLATEGROUP)->getShared(0);
   templateGroup->deleteObject();
   if (parent != nullptr)
      DeleteEmptyTemplateGroup(parent);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): template group %s [%d] with GUID %s deleted because it was empty"), templateGroup->getName(), templateGroup->getId());
}

/**
 * Import configuration
 */
uint32_t ImportConfig(const Config& config, uint32_t flags, StringBuffer **log)
{
   ImportContext *context = new ImportContext();
   *log = context;
   if (!ValidateConfig(config, flags, context))
   {
      return RCC_CONFIG_VALIDATION_ERROR;
   }

   ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot, *rulesRoot,
      *scriptsRoot, *objectToolsRoot, *summaryTablesRoot, *webServiceDefRoot,
      *actionsRoot, *assetsRoot;

   bool nxslV5 = config.getValueAsBoolean(_T("/nxslVersionV5"), false);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportConfig() called, flags=0x%04X, nxslV5=%s"), flags, nxslV5 ? _T("Yes") : _T("No"));

   uint32_t rcc = RCC_SUCCESS;
   // Import events
	eventsRoot = config.getEntry(_T("/events"));
	if (eventsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> events = eventsRoot->getSubEntries(_T("event#*"));
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("%d event templates to import"), events->size());
      for (int i = 0; i < events->size(); i++)
      {
			rcc = ImportEvent(events->get(i), (flags & CFG_IMPORT_REPLACE_EVENTS) != 0, context);
			if (rcc != RCC_SUCCESS)
				goto stop_processing;
		}

		if (events->size() > 0)
		{
			ReloadEvents();
			NotifyClientSessions(NX_NOTIFY_RELOAD_EVENT_DB, 0);
		}
		context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Event templates import completed"));
	}

	// Import traps
	trapsRoot = config.getEntry(_T("/traps"));
	if (trapsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> traps = trapsRoot->getSubEntries(_T("trap#*"));
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("%d SNMP traps to import"), traps->size());
      for (int i = 0; i < traps->size(); i++)
      {
			rcc = ImportTrapMapping(*traps->get(i), (flags & CFG_IMPORT_REPLACE_TRAPS) != 0, context, nxslV5);
			if (rcc != RCC_SUCCESS)
				goto stop_processing;
		}
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("SNMP traps import completed"));
	}

	// Import templates
	templatesRoot = config.getEntry(_T("/templates"));
	if (templatesRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> templates = templatesRoot->getSubEntries(_T("template#*"));
      for (int i = 0; i < templates->size(); i++)
      {
         ConfigEntry *tc = templates->get(i);
		   uuid guid = tc->getSubEntryValueAsUUID(_T("guid"));
		   shared_ptr<Template> object = shared_ptr<Template>();
		   if (guid.isNull())
		      guid = uuid::generate();
		   else
		      object = static_pointer_cast<Template>(FindObjectByGUID(guid, OBJECT_TEMPLATE));
		   if (object != nullptr)
		   {
		      if (flags & CFG_IMPORT_REPLACE_TEMPLATES)
		      {
		         context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Updating existing template %s [%u] with GUID %s"), object->getName(), object->getId(), guid.toString().cstr());
		         object->updateFromImport(tc, context, nxslV5);
		      }
		      else
		      {
		         context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Existing template %s [%u] with GUID %s skipped"), object->getName(), object->getId(), guid.toString().cstr());
		      }

            if (flags & CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS)
            {
               context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Existing template %s [%u] with GUID %s renamed to %s"), object->getName(), object->getId(), guid.toString().cstr(), tc->getSubEntryValue(_T("name")));

               object->setName(tc->getSubEntryValue(_T("name")));
               shared_ptr<NetObj> newParent = FindTemplateRoot(tc);
               if (!newParent->isChild(object->getId()))
               {
                  context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Existing template %s [%u] with GUID %s moved to template group \"%s\""), object->getName(), object->getId(), guid.toString().cstr(), newParent->getName());
                  unique_ptr<SharedObjectArray<NetObj>> parents = object->getParents(OBJECT_TEMPLATEGROUP);
                  if (parents->size() > 0)
                  {
                     shared_ptr<NetObj> currParent = parents->getShared(0);
                     NetObj::unlinkObjects(currParent.get(), object.get());
                     if (flags & CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS)
                     {
                        DeleteEmptyTemplateGroup(currParent);
                     }
                  }
                  NetObj::linkObjects(newParent, object);
               }
            }
		   }
		   else
		   {
		      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Template with GUID %s not found"), guid.toString().cstr());
            shared_ptr<NetObj> parent = FindTemplateRoot(tc);
            object = make_shared<Template>(tc->getSubEntryValue(_T("name"), 0, _T("Unnamed Object")), guid);
            NetObjInsert(object, true, true);
            object->updateFromImport(tc, context, nxslV5);
            NetObj::linkObjects(parent, object);
            object->unhide();
            context->log(NXLOG_INFO, _T("ImportConfig()"), _T("New template \"%s\" added"), object->getName());
		   }
      }
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Templates import completed"));
	}

   // Import actions
   actionsRoot = config.getEntry(_T("/actions"));
   if (actionsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> actions = actionsRoot->getSubEntries(_T("action#*"));
      for(int i = 0; i < actions->size(); i++)
      {
         ImportAction(actions->get(i), (flags & CFG_IMPORT_REPLACE_ACTIONS) != 0, context);
      }
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Actions import completed"));
   }

	// Import rules
	rulesRoot = config.getEntry(_T("/rules"));
	if (rulesRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> rules = rulesRoot->getOrderedSubEntries(_T("rule#*"));
      if (rules->size() > 0)
      {
         //get rule ordering
		   ObjectArray<uuid> *ruleOrdering = GetRuleOrdering(config.getEntry(_T("/ruleOrdering")));
         for(int i = 0; i < rules->size(); i++)
         {
            EPRule *rule = new EPRule(*rules->get(i), context, nxslV5);
            g_pEventPolicy->importRule(rule, (flags & CFG_IMPORT_REPLACE_EPP_RULES) != 0, ruleOrdering);
         }
         delete ruleOrdering;
         if (!g_pEventPolicy->saveToDB())
         {
            context->log(NXLOG_ERROR, _T("ImportConfig()"), _T("Unable to import event processing policy rules"));
            rcc = RCC_DB_FAILURE;
            goto stop_processing;
         }
      }
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Event processing policy import completed"));
	}

	// Import scripts
	scriptsRoot = config.getEntry(_T("/scripts"));
	if (scriptsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> scripts = scriptsRoot->getSubEntries(_T("script#*"));
      for (int i = 0; i < scripts->size(); i++)
      {
         ImportScript(scripts->get(i), (flags & CFG_IMPORT_REPLACE_SCRIPTS) != 0, context, nxslV5);
      }
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Script import completed"));
	}

	// Import object tools
	objectToolsRoot = config.getEntry(_T("/objectTools"));
	if (objectToolsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> objectTools = objectToolsRoot->getSubEntries(_T("objectTool#*"));
      for (int i = 0; i < objectTools->size(); i++)
      {
         ImportObjectTool(objectTools->get(i), (flags & CFG_IMPORT_REPLACE_OBJECT_TOOLS) != 0, context);
      }
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Object tools import completed"));
	}

	// Import summary tables
	summaryTablesRoot = config.getEntry(_T("/dciSummaryTables"));
	if (summaryTablesRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> summaryTables = summaryTablesRoot->getSubEntries(_T("table#*"));
      for (int i = 0; i < summaryTables->size(); i++)
      {
         ImportSummaryTable(summaryTables->get(i), (flags & CFG_IMPORT_REPLACE_SUMMARY_TABLES) != 0, context, nxslV5);
      }
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("DCI summary tables import completed"));
	}

	// Import web service definitions
   webServiceDefRoot = config.getEntry(_T("/webServiceDefinitions"));
   if (webServiceDefRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> webServiceDef = webServiceDefRoot->getSubEntries(_T("webServiceDefinition#*"));
      for(int i = 0; i < webServiceDef->size(); i++)
      {
         ImportWebServiceDefinition(*webServiceDef->get(i), (flags & CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS) != 0, context);
      }
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Web service definitions import completed"));
   }

   // Import asset management schema
   assetsRoot = config.getEntry(_T("/assetManagementSchema"));
   if (assetsRoot != nullptr)
   {
      ImportAssetManagementSchema(*assetsRoot, (flags & CFG_IMPORT_REPLACE_AM_DEFINITIONS) != 0, context, nxslV5);
      context->log(NXLOG_INFO, _T("ImportConfig()"), _T("Asset management schema import completed"));
   }

stop_processing:
   nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportConfig() finished, rcc = %u"), rcc);
   return rcc;
}

/**
 * Import local configuration (configuration files stored on server)
 */
void ImportLocalConfiguration(bool overwrite)
{
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   _tcscat(path, SDIR_TEMPLATES);

   int count = 0;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Import configuration files from %s"), path);
   _TDIR *dir = _topendir(path);
   if (dir != nullptr)
   {
      _tcscat(path, FS_PATH_SEPARATOR);
      int insPos = (int)_tcslen(path);

      struct _tdirent *f;
      while((f = _treaddir(dir)) != nullptr)
      {
         if (MatchString(_T("*.xml"), f->d_name, FALSE))
         {
            _tcscpy(&path[insPos], f->d_name);
            Config config(false);
            if (config.loadXmlConfig(path, "configuration"))
            {
               StringBuffer *log;
               ImportConfig(config, overwrite ? CFG_IMPORT_REPLACE_EVERYTHING : 0, &log);
               delete log;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 1, _T("Error loading configuration from %s"), path);
            }
         }
      }
      _tclosedir(dir);
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d configuration files processed"), count);
}
