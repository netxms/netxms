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
** JSON-based import functions
**
**/

#include "nxcore.h"
#include <nxcore_websvc.h>
#include <asset_management.h>
#include <jansson/jansson.h>
#include <netxms-xml.h>

#define DEBUG_TAG _T("import")

void DeleteEmptyTemplateGroup(const shared_ptr<NetObj>& parent);

/**
 * Check if given event exist either in server configuration or in configuration being imported
 */
static bool IsEventExist(const TCHAR *name, json_t *root)
{
   if (name == nullptr)
      return false;

   // Check if event exists in server configuration
   shared_ptr<EventTemplate> e = FindEventTemplateByName(name);
   if (e != nullptr)
      return true;

   // Check if event exists in JSON configuration being imported
   json_t *events = json_object_get(root, "events");
   if (json_is_array(events))
   {
      size_t index;
      json_t *event;
      json_array_foreach(events, index, event)
      {
         if (json_is_object(event))
         {
            json_t *nameObj = json_object_get(event, "name");
            if (json_is_string(nameObj))
            {
               String eventName = json_object_get_string(event, "name", _T("<unnamed>"));
               if (!eventName.isEmpty() && (_tcsicmp(eventName, name) == 0))
               {
                  return true;
               }
            }
         }
      }
   }
   return false;
}

/**
 * Validate DCI thresholds from JSON template
 */
static bool ValidateDci(json_t *root, json_t *dci, const TCHAR *templateName, ImportContext *context)
{
   json_t *thresholds = json_object_get(dci, "thresholds");
   if (!json_is_array(thresholds))
      return true;

   bool success = true;
   size_t index;
   json_t *threshold;
   json_array_foreach(thresholds, index, threshold)
   {
      if (!json_is_object(threshold))
         continue;

      // Check activation event
      String activationEventName = json_object_get_string(threshold, "activationEvent", nullptr);
      if (!activationEventName.isEmpty() && !IsEventExist(activationEventName, root))
      {
         context->log(NXLOG_ERROR, _T("ValidateDci()"),
            _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"activationEvent\" refers to unknown event"),
            templateName, json_object_get_string(dci, "description", _T("<unnamed>")).cstr(), index + 1);
         success = false;
      }

      // Check deactivation event
      String deactivationEventName = json_object_get_string(threshold, "deactivationEvent", nullptr);
      if (!deactivationEventName.isEmpty() && !IsEventExist(deactivationEventName, root))
      {
         context->log(NXLOG_ERROR, _T("ValidateDci()"),
            _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"deactivationEvent\" refers to unknown event"),
            templateName, json_object_get_string(dci, "description", _T("<unnamed>")).cstr(), index + 1);
         success = false;
      }
   }
   return success;
}

/**
 * Validate template
 */
static bool ValidateTemplate(json_t *root, json_t *templateObj, ImportContext *context)
{
   context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validating template \"%s\""), json_object_get_string(templateObj, "name", _T("<unnamed>")).cstr());

   json_t *dataCollection = json_object_get(templateObj, "dataCollection");
   if (!json_is_object(dataCollection))
      return true;

   bool success = true;
   String templateName = json_object_get_string(templateObj, "name", _T("<unnamed>"));

   // Validate DCIs
   json_t *dcis = json_object_get(dataCollection, "dcis");
   if (json_is_array(dcis))
   {
      size_t index;
      json_t *dci;
      json_array_foreach(dcis, index, dci)
      {
         if (json_is_object(dci))
         {
            if (!ValidateDci(root, dci, templateName, context))
               success = false;
         }
      }
   }

   // Validate DC Tables
   json_t *dcTables = json_object_get(dataCollection, "dcTables");
   if (json_is_array(dcTables))
   {
      size_t index;
      json_t *dctable;
      json_array_foreach(dcTables, index, dctable)
      {
         if (json_is_object(dctable))
         {
            if (!ValidateDci(root, dctable, templateName, context))
               success = false;
         }
      }
   }
   return success;
}

/**
 * Validate configuration before import
 */
static bool ValidateConfig(json_t *root, uint32_t flags, ImportContext *context)
{
   bool success = true;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("ValidateConfig() called, flags = 0x%04X"), flags);

   // Validate events
   json_t *events = json_object_get(root, "events");
   if (json_is_array(events))
   {
      size_t index;
      json_t *event;
      json_array_foreach(events, index, event)
      {
         if (!json_is_object(event))
            continue;

         context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validating event \"%s\""), json_object_get_string(event, "name", _T("<unnamed>")).cstr());

         uint32_t code = json_object_get_uint32(event, "code");         
         if ((code >= FIRST_USER_EVENT_ID) || (code == 0))
         {
            json_t *nameObj = json_object_get(event, "name");
            if (nameObj == nullptr || !json_is_string(nameObj) || (json_string_value(nameObj)[0] == 0))
            {
               context->log(NXLOG_ERROR, _T("ValidateConfig()"), _T("Mandatory attribute \"name\" missing for event template"));
               success = false;
            }
         }
      }
   }

   // Validate traps
   json_t *traps = json_object_get(root, "traps");
   if (json_is_array(traps))
   {
      size_t index;
      json_t *trap;
      json_array_foreach(traps, index, trap)
      {
         if (!json_is_object(trap))
            continue;

         context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validating SNMP trap definition \"%s\""), json_object_get_string(trap, "description", _T("<unnamed>")).cstr());

         json_t *eventObj = json_object_get(trap, "event");
         if (json_is_string(eventObj))
         {
            String eventName = json_object_get_string(trap, "event", _T("<unnamed>"));
            if (!eventName.isEmpty())
            {
               if (!IsEventExist(eventName, root))
               {
                  context->log(NXLOG_ERROR, _T("ValidateConfig()"), _T("SNMP trap definition \"%s\" refers to unknown event"), json_object_get_string(trap, "description", _T("")).cstr());
                  success = false;
               }
            }
         }
      }
   }

   // Validate templates
   json_t *templates = json_object_get(root, "templates");
   if (json_is_array(templates))
   {
      size_t index;
      json_t *templateObj;
      json_array_foreach(templates, index, templateObj)
      {
         if (json_is_object(templateObj))
         {
            if (!ValidateTemplate(root, templateObj, context))
               success = false;
         }
      }
   }

   context->log(NXLOG_INFO, _T("ValidateConfig()"), _T("Validation %s"), success ? _T("successfully finished") : _T("failed"));
   return success;
}

/**
 * Configuration format types
 */
enum ConfigurationFormat
{
   CONFIG_FORMAT_UNKNOWN = 0,
   CONFIG_FORMAT_XML = 1,
   CONFIG_FORMAT_JSON = 2
};

/**
 * Detect configuration format from content
 * @param content Configuration file content (null-terminated string)
 * @return Configuration format type
 */
static ConfigurationFormat DetectConfigurationFormat(const char* content)
{
   if (content == nullptr || *content == 0)
      return CONFIG_FORMAT_UNKNOWN;
      
   // Skip leading whitespace
   const char* ptr = content;
   while (isspace(*ptr))
      ptr++;
      
   if (*ptr == 0)
      return CONFIG_FORMAT_UNKNOWN;
      
   // Check first non-whitespace character
   switch (*ptr)
   {
      case '{':
         return CONFIG_FORMAT_JSON;
      case '<':
         return CONFIG_FORMAT_XML;
      default:
         return CONFIG_FORMAT_UNKNOWN;
   }
}

/**
 * Import event
 */
static uint32_t ImportEvent(json_t *event, bool overwrite, ImportContext *context)
{
   String name = json_object_get_string(event, "name", _T("<unnamed>"));

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   UINT32 code = 0;
   bool checkByName = false;
   
   uuid guid = json_object_get_uuid(event, "guid");
   
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
         code = json_object_get_uint32(event, "code");
         
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

   // Get other event properties
   String msg = json_object_get_string(event, "message", name);
   String descr = json_object_get_string(event, "description", _T(""));
   String tags = json_object_get_string(event, "tags", _T(""));
   int severity = json_object_get_int32(event, "severity", EVENT_SEVERITY_NORMAL);
   int flags = json_object_get_int32(event, "flags", 0);

   TCHAR query[8192];
   if ((code != 0) && IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), code))
   {
      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Found existing event with code %d (%s)"), code, overwrite ? _T("updating") : _T("skipping"));
      if (overwrite)
      {
         _sntprintf(query, 8192, _T("UPDATE event_cfg SET event_name=%s,severity=%d,flags=%d,message=%s,description=%s,tags=%s WHERE event_code=%d"),
                    (const TCHAR *)DBPrepareString(hdb, name), severity, flags,
                    (const TCHAR *)DBPrepareString(hdb, msg),
                    (const TCHAR *)DBPrepareString(hdb, descr),
                    (const TCHAR *)DBPrepareString(hdb, tags));
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
                    severity, flags, (const TCHAR *)DBPrepareString(hdb, msg),
                    (const TCHAR *)DBPrepareString(hdb, descr),
                    (const TCHAR *)DBPrepareString(hdb, tags),
                    (const TCHAR *)DBPrepareString(hdb, name));
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
                 code, (const TCHAR *)DBPrepareString(hdb, name), severity, flags,
                 (const TCHAR *)DBPrepareString(hdb, msg),
                 (const TCHAR *)DBPrepareString(hdb, descr),
                 (const TCHAR *)guid.toString(),
                 (const TCHAR *)DBPrepareString(hdb, tags));
      context->log(NXLOG_INFO, _T("ImportEvent()"), _T("Added new event: code=%d, name=%s, guid=%s"), code, name.cstr(), guid.toString().cstr());
   }
   
   uint32_t rcc = (query[0] != 0) ? (DBQuery(hdb, query) ? RCC_SUCCESS : RCC_DB_FAILURE) : RCC_SUCCESS;

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Import JSON events directly
 */
static uint32_t ImportJsonEvents(json_t *events, uint32_t flags, ImportContext *context)
{
   size_t count = json_array_size(events);
   context->log(NXLOG_INFO, _T("ImportJsonEvents()"), _T("%d event templates to import"), count);
   
   size_t index;
   json_t *event;
   json_array_foreach(events, index, event)
   {
      if (!json_is_object(event))
         continue;
         
      uint32_t rcc = ImportEvent(event, (flags & CFG_IMPORT_REPLACE_EVENTS) != 0, context);
      if (rcc != RCC_SUCCESS)
         return rcc;
   }
   
   if (count > 0)
   {
      ReloadEvents();
      NotifyClientSessions(NX_NOTIFY_RELOAD_EVENT_DB, 0);
   }
   context->log(NXLOG_INFO, _T("ImportJsonEvents()"), _T("Event templates import completed"));
   return RCC_SUCCESS;
}

/**
 * Import trap from JSON object - completely without ConfigEntry
 */
static uint32_t ImportTrap(json_t *trap, bool overwrite, ImportContext *context)
{
   shared_ptr<EventTemplate> eventTemplate = FindEventTemplateByName(json_object_get_string(trap, "event", _T("")));
   if (eventTemplate == nullptr)
      return RCC_INTERNAL_ERROR;

   // Get GUID
   uuid guid = json_object_get_uuid(trap, "guid");
   
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

   // Create trap mapping using JSON constructor
   auto trapMapping = make_shared<SNMPTrapMapping>(trap, guid, id, eventTemplate->getCode(), true);
   if (!trapMapping->getOid().isValid())
	{
      context->log(NXLOG_ERROR, _T("ImportTrap()"), _T("SNMP trap mapping %s refers to invalid OID"), guid.toString().cstr());
		return RCC_INTERNAL_ERROR;
	}

   uint32_t rcc = RCC_INTERNAL_ERROR;
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
 * Find (and create as necessary) parent object for imported template (JSON version)
 */
static shared_ptr<NetObj> FindTemplateRootJson(json_t *templateJson)
{
   json_t *pathArray = json_object_get(templateJson, "path");
   if (!json_is_array(pathArray) || json_array_size(pathArray) == 0)
      return g_templateRoot;  // path not specified in config

   shared_ptr<NetObj> parent = g_templateRoot;
   size_t pathSize = json_array_size(pathArray);
   for (size_t i = 0; i < pathSize; i++)
   {
      json_t *pathElement = json_array_get(pathArray, i);
      
      // Path elements
      if (!json_is_string(pathElement))
         continue;
         
      TCHAR *name = TStringFromUTF8String(json_string_value(pathElement));
      if (name == nullptr)
         continue;

      shared_ptr<NetObj> o = parent->findChildObject(name, OBJECT_TEMPLATEGROUP);
      if (o == nullptr)
      {
         o = make_shared<TemplateGroup>(name);
         NetObjInsert(o, true, false);
         NetObj::linkObjects(parent, o);
         o->publish();
         o->calculateCompoundStatus();	// Force status change to NORMAL
      }
      parent = o;
      MemFree(name);
   }
   return parent;
}/**
 * Import JSON templates directly
 */
static uint32_t ImportJsonTemplates(json_t *templates, uint32_t flags, ImportContext *context)
{
   size_t count = json_array_size(templates);
   context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("%d templates to import"), (int)count);
   
   size_t index;
   json_t *templateJson;
   json_array_foreach(templates, index, templateJson)
   {
      if (!json_is_object(templateJson))
         continue;
         
      // Get template GUID and name
      uuid guid = json_object_get_uuid(templateJson, "guid");
      String name = json_object_get_string(templateJson, "name", _T("<unnamed>"));
      
      shared_ptr<Template> templateObj;
      if (guid.isNull())
         guid = uuid::generate();
      else
         templateObj = static_pointer_cast<Template>(FindObjectByGUID(guid, OBJECT_TEMPLATE));
         
      if (templateObj != nullptr)
      {
         if (flags & CFG_IMPORT_REPLACE_TEMPLATES)
         {
            context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("Updating existing template %s [%u] with GUID %s"), 
                        templateObj->getName(), templateObj->getId(), guid.toString().cstr());
            templateObj->updateFromImport(templateJson, context);
         }
         else
         {
            context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("Existing template %s [%u] with GUID %s skipped"), 
                        templateObj->getName(), templateObj->getId(), guid.toString().cstr());
         }

         if (flags & CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS)
         {
            context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("Existing template %s [%u] with GUID %s renamed to %s"), 
                        templateObj->getName(), templateObj->getId(), guid.toString().cstr(), name.cstr());

            templateObj->setName(name);
            shared_ptr<NetObj> newParent = FindTemplateRootJson(templateJson);
            if (!newParent->isChild(templateObj->getId()))
            {
               context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("Existing template %s [%u] with GUID %s moved to template group \"%s\""), 
                           templateObj->getName(), templateObj->getId(), guid.toString().cstr(), newParent->getName());
               unique_ptr<SharedObjectArray<NetObj>> parents = templateObj->getParents(OBJECT_TEMPLATEGROUP);
               if (parents->size() > 0)
               {
                  shared_ptr<NetObj> currParent = parents->getShared(0);
                  NetObj::unlinkObjects(currParent.get(), templateObj.get());
                  if (flags & CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS)
                  {
                     DeleteEmptyTemplateGroup(currParent);
                  }
               }
               NetObj::linkObjects(newParent, templateObj);
            }
         }
      }
      else
      {
         context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("Template with GUID %s not found"), guid.toString().cstr());
         shared_ptr<NetObj> parent = FindTemplateRootJson(templateJson);
         templateObj = make_shared<Template>(name, guid);
         NetObjInsert(templateObj, true, true);
         templateObj->updateFromImport(templateJson, context);
         NetObj::linkObjects(parent, templateObj);
         templateObj->publish();
         context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("New template \"%s\" added"), templateObj->getName());
      }
   }
   
   context->log(NXLOG_INFO, _T("ImportJsonTemplates()"), _T("Templates import completed"));
   return RCC_SUCCESS;
}

/**
 * Get rule ordering array from JSON
 */
static ObjectArray<uuid> *GetRuleOrderingFromJson(json_t *root)
{
   ObjectArray<uuid> *ordering = nullptr;
   json_t *ruleOrderingArray = json_object_get(root, "ruleOrdering");
   if (json_is_array(ruleOrderingArray))
   {
      size_t count = json_array_size(ruleOrderingArray);
      if (count > 0)
      {
         ordering = new ObjectArray<uuid>(0, 16, Ownership::True);
         size_t index;
         json_t *guidObj;
         json_array_foreach(ruleOrderingArray, index, guidObj)
         {
            if (json_is_string(guidObj))
            {
               String guidStr(json_string_value(guidObj), "utf8");
               if (!guidStr.isEmpty())
               {
                  ordering->add(new uuid(uuid::parse(guidStr)));
               }
            }
         }
      }
   }
   return ordering;
}

/**
 * Import JSON rules directly
 */
static uint32_t ImportJsonRules(json_t *rules, json_t *root, uint32_t flags, ImportContext *context)
{
   size_t count = json_array_size(rules);
   context->log(NXLOG_INFO, _T("ImportJsonRules()"), _T("%d event processing rules to import"), (int)count);
   
   if (count == 0)
   {
      context->log(NXLOG_INFO, _T("ImportJsonRules()"), _T("Event processing rules import completed"));
      return RCC_SUCCESS;
   }
   
   // Get rule ordering from the root JSON object
   ObjectArray<uuid> *ruleOrdering = GetRuleOrderingFromJson(root);
   
   size_t index;
   json_t *rule;
   json_array_foreach(rules, index, rule)
   {
      if (!json_is_object(rule))
         continue;
         
      // Get rule GUID
      uuid guid = json_object_get_uuid(rule, "guid");
      
      if (guid.isNull())
         guid = uuid::generate();

      context->log(NXLOG_INFO, _T("ImportJsonRules()"), _T("Processing event processing rule with GUID %s"), guid.toString().cstr());
      EPRule *newRule = new EPRule(rule, context);
      GetEventProcessingPolicy()->importRule(newRule, (flags & CFG_IMPORT_REPLACE_EPP_RULES) != 0, ruleOrdering);
   }
   
   // Clean up rule ordering
   delete ruleOrdering;
   
   // Save policy to database
   if (!GetEventProcessingPolicy()->saveToDB())
   {
      context->log(NXLOG_ERROR, _T("ImportJsonRules()"), _T("Unable to save event processing policy rules to database"));
      return RCC_DB_FAILURE;
   }
   
   context->log(NXLOG_INFO, _T("ImportJsonRules()"), _T("Event processing rules import completed"));
   return RCC_SUCCESS;
}

/**
 * Import JSON scripts directly
 */
static uint32_t ImportJsonScripts(json_t *scripts, uint32_t flags, ImportContext *context)
{
   size_t count = json_array_size(scripts);
   context->log(NXLOG_INFO, _T("ImportJsonScripts()"), _T("%d scripts to import"), (int)count);
   
   size_t index;
   json_t *script;
   json_array_foreach(scripts, index, script)
   {
      if (!json_is_object(script))
         continue;
         
      ImportScript(script, (flags & CFG_IMPORT_REPLACE_SCRIPTS) != 0, context);
   }
   
   context->log(NXLOG_INFO, _T("ImportJsonScripts()"), _T("Scripts import completed"));
   return RCC_SUCCESS;
}

/**
 * Import JSON actions directly
 */
static uint32_t ImportJsonActions(json_t *actions, uint32_t flags, ImportContext *context)
{
   size_t count = json_array_size(actions);
   context->log(NXLOG_INFO, _T("ImportJsonActions()"), _T("%d actions to import"), (int)count);
   
   size_t index;
   json_t *action;
   json_array_foreach(actions, index, action)
   {
      if (!json_is_object(action))
         continue;
         
      if (!ImportAction(action, (flags & CFG_IMPORT_REPLACE_ACTIONS) != 0, context))
         return RCC_INTERNAL_ERROR;
   }
   
   context->log(NXLOG_INFO, _T("ImportJsonActions()"), _T("Actions import completed"));
   return RCC_SUCCESS;
}

/**
 * Import JSON object tools directly
 */
static uint32_t ImportJsonObjectTools(json_t *tools, uint32_t flags, ImportContext *context)
{
   size_t count = json_array_size(tools);
   context->log(NXLOG_INFO, _T("ImportJsonObjectTools()"), _T("%d object tools to import"), (int)count);
   
   size_t index;
   json_t *tool;
   json_array_foreach(tools, index, tool)
   {
      if (!json_is_object(tool))
         continue;
         
      if (!ImportObjectTool(tool, (flags & CFG_IMPORT_REPLACE_OBJECT_TOOLS) != 0, context))
         return RCC_INTERNAL_ERROR;
   }
   
   context->log(NXLOG_INFO, _T("ImportJsonObjectTools()"), _T("Object tools import completed"));
   return RCC_SUCCESS;
}

/**
 * Import JSON traps directly
 */
static uint32_t ImportJsonTraps(json_t *traps, uint32_t flags, ImportContext *context)
{
   size_t count = json_array_size(traps);
   context->log(NXLOG_INFO, _T("ImportJsonTraps()"), _T("%d SNMP traps to import"), (int)count);
   
   size_t index;
   json_t *trap;
   json_array_foreach(traps, index, trap)
   {
      if (!json_is_object(trap))
         continue;
         
      uint32_t rcc = ImportTrap(trap, (flags & CFG_IMPORT_REPLACE_TRAPS) != 0, context);
      if (rcc != RCC_SUCCESS)
         return rcc;
   }
   
   context->log(NXLOG_INFO, _T("ImportJsonTraps()"), _T("SNMP traps import completed"));
   return RCC_SUCCESS;
}

/**
 * Find rule index by GUID in XML rules node
 * @param rulesNode XML node containing rules
 * @param guid GUID to find
 * @param shift Index shift to apply if found (similar to EPP logic)
 * @return Rule index or -1 if not found
 */
static int FindLogParserRuleIndexByGuid(pugi::xml_node rulesNode, const char *guid, int shift = 0)
{
   int index = 0;
   for (pugi::xml_node rule = rulesNode.first_child(); rule; rule = rule.next_sibling())
   {
      if (strcmp(rule.name(), "rule") == 0)
      {
         pugi::xml_attribute guidAttr = rule.attribute("guid");
         if (guidAttr && strcmp(guidAttr.value(), guid) == 0)
         {
            return index + shift;
         }
         index++;
      }
   }
   return -1;
}

/**
 * Insert XML rule at specific position in rules node
 * @param rulesNode XML node to insert into
 * @param newRuleXml XML content of new rule
 * @param insertIndex Position to insert at (-1 for append)
 * @return true if successful
 */
static bool InsertLogParserRuleAtPosition(pugi::xml_node rulesNode, const char *newRuleXml, int insertIndex)
{
   pugi::xml_document ruleDoc;
   if (!ruleDoc.load_string(newRuleXml))
      return false;

   pugi::xml_node newRule = ruleDoc.first_child();
   if (!newRule)
      return false;

   if (insertIndex == -1)
   {
      // Append at end
      rulesNode.append_copy(newRule);
      return true;
   }

   // Find the position to insert before
   int currentIndex = 0;
   for (pugi::xml_node rule = rulesNode.first_child(); rule; rule = rule.next_sibling())
   {
      if (strcmp(rule.name(), "rule") == 0)
      {
         if (currentIndex == insertIndex)
         {
            rulesNode.insert_copy_before(newRule, rule);
            return true;
         }
         currentIndex++;
      }
   }

   // If we get here, insertIndex was beyond the end, so append
   rulesNode.append_copy(newRule);
   return true;
}

/**
 * Import log parser rules from JSON object (unified for syslog and winlog)
 */
static bool ImportLogParser(json_t *parserObj, uint32_t flags, ImportContext *context, 
                           const TCHAR *parserName, const TCHAR *configKey)
{
   if (!json_is_object(parserObj))
      return true; // No rules to import

   json_t *rulesArray = json_object_get(parserObj, "rules");
   json_t *macrosObj = json_object_get(parserObj, "macros");
   json_t *orderArray = json_object_get(parserObj, "order");

   if (!json_is_array(rulesArray) && !json_is_object(macrosObj))
      return true; // No rules or macros to import

   context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("Importing %s parser rules"), parserName);

   char *existingConfigUtf8 = ConfigReadCLOBUTF8(configKey, "<parser></parser>");
   pugi::xml_document xml;
   if (!xml.load_buffer(existingConfigUtf8, strlen(existingConfigUtf8)))
   {
      xml.load_string("<parser></parser>");
   }

   pugi::xml_node parserNode = xml.first_child();
   if (!parserNode || strcmp(parserNode.name(), "parser") != 0)
   {
      xml.remove_children();
      parserNode = xml.append_child("parser");
   }

   // Get or create macros node
   pugi::xml_node macrosNode = parserNode.child("macros");
   if (!macrosNode)
      macrosNode = parserNode.append_child("macros");

   // Get or create rules node
   pugi::xml_node rulesNode = parserNode.child("rules");
   if (!rulesNode)
      rulesNode = parserNode.append_child("rules");

   // Process macros
   if (json_is_object(macrosObj))
   {
      const char *macroName;
      json_t *macroValue;
      json_object_foreach(macrosObj, macroName, macroValue)
      {
         if (json_is_string(macroValue))
         {
            // Find existing macro or create new one
            pugi::xml_node macroNode = macrosNode.find_child_by_attribute("macro", "name", macroName);
            bool isNew = !macroNode;
            
            if (macroNode && !(flags & CFG_IMPORT_REPLACE_LOGPARSER_MACROS))
            {
               context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("Existing %s parser macro '%hs' skipped"), parserName, macroName);
               continue;
            }
            
            if (!macroNode)
            {
               macroNode = macrosNode.append_child("macro");
               macroNode.append_attribute("name") = macroName;
            }
            
            macroNode.text().set(json_string_value(macroValue));
            
            if (isNew)
               context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("Adding new %s parser macro '%hs'"), parserName, macroName);
            else
               context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("Updating existing %s parser macro '%hs'"), parserName, macroName);
         }
      }
   }

   size_t ruleIndex;
   json_t *ruleObj;
   json_array_foreach(rulesArray, ruleIndex, ruleObj)
   {
      if (!json_is_object(ruleObj))
         continue;
         
      const char *guidUtf8 = json_object_get_string_utf8(ruleObj, "guid", nullptr);
      const char *ruleXmlUtf8 = json_object_get_string_utf8(ruleObj, "content", nullptr);
      if (guidUtf8 == nullptr || ruleXmlUtf8 == nullptr)
         continue;
      
      pugi::xml_node existingRule = rulesNode.find_child_by_attribute("rule", "guid", guidUtf8);
      if (existingRule)
      {
         bool shouldReplace = false;
         if (_tcsicmp(configKey, _T("SyslogParser")) == 0)
            shouldReplace = (flags & CFG_IMPORT_REPLACE_SYSLOG_PARSERS) != 0;
         else if (_tcsicmp(configKey, _T("WindowsEventParser")) == 0)
            shouldReplace = (flags & CFG_IMPORT_REPLACE_WINDOWS_LOG_PARSERS) != 0;

         if (shouldReplace)
         {
            context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("Updating existing %s parser rule with GUID %hs"), parserName, guidUtf8);
            // Replace existing rule content in place
            pugi::xml_document ruleDoc;
            if (ruleDoc.load_string(ruleXmlUtf8))
            {
               pugi::xml_node newRuleContent = ruleDoc.first_child();
               if (newRuleContent)
               {
                  // Clear existing rule content and copy new content
                  existingRule.remove_children();
                  for (pugi::xml_attribute attr = existingRule.first_attribute(); attr; )
                  {
                     pugi::xml_attribute next = attr.next_attribute();
                     existingRule.remove_attribute(attr);
                     attr = next;
                  }
                  
                  // Copy attributes and children from new rule
                  for (pugi::xml_attribute attr = newRuleContent.first_attribute(); attr; attr = attr.next_attribute())
                  {
                     existingRule.append_attribute(attr.name()) = attr.value();
                  }
                  for (pugi::xml_node child = newRuleContent.first_child(); child; child = child.next_sibling())
                  {
                     existingRule.append_copy(child);
                  }
               }
            }
            continue; // Rule updated in place, skip insertion logic
         }
         else
         {
            context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("Existing %s parser rule with GUID %hs skipped"), parserName, guidUtf8);
            continue;
         }
      }
      else
      {
         context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("Adding new %s parser rule with GUID %hs"), parserName, guidUtf8);
      }
      
      int insertPosition = -1; 
      if (json_is_array(orderArray))
      {
         // Find this rule's position in the ordering array
         int newRuleOrderIndex = -1;
         size_t orderIndex;
         json_t *guidValue;
         json_array_foreach(orderArray, orderIndex, guidValue)
         {
            if (json_is_string(guidValue) && strcmp(json_string_value(guidValue), guidUtf8) == 0)
            {
               newRuleOrderIndex = (int)orderIndex;
               break;
            }
         }
         
         if (newRuleOrderIndex != -1)
         {
            // Try to find rule immediately before this position that exists
            if (newRuleOrderIndex > 0)
            {
               json_t *prevGuidValue = json_array_get(orderArray, newRuleOrderIndex - 1);
               if (json_is_string(prevGuidValue))
               {
                  insertPosition = FindLogParserRuleIndexByGuid(rulesNode, json_string_value(prevGuidValue), 1);
               }
            }
            
            // If rule before not found, try rule after this position
            if (insertPosition == -1 && (newRuleOrderIndex + 1) < (int)json_array_size(orderArray))
            {
               json_t *nextGuidValue = json_array_get(orderArray, newRuleOrderIndex + 1);
               if (json_is_string(nextGuidValue))
               {
                  insertPosition = FindLogParserRuleIndexByGuid(rulesNode, json_string_value(nextGuidValue));
               }
            }
            
            // Check if any rule before this position exists (scan backwards)
            for (int j = newRuleOrderIndex - 2; insertPosition == -1 && j >= 0; j--)
            {
               json_t *searchGuidValue = json_array_get(orderArray, j);
               if (json_is_string(searchGuidValue))
               {
                  insertPosition = FindLogParserRuleIndexByGuid(rulesNode, json_string_value(searchGuidValue), 1);
               }
            }
            
            // Check if any rule after this position exists (scan forwards)
            for (int j = newRuleOrderIndex + 2; insertPosition == -1 && j < (int)json_array_size(orderArray); j++)
            {
               json_t *searchGuidValue = json_array_get(orderArray, j);
               if (json_is_string(searchGuidValue))
               {
                  insertPosition = FindLogParserRuleIndexByGuid(rulesNode, json_string_value(searchGuidValue));
               }
            }
         }
      }
      
      // Insert the rule at the determined position
      if (!InsertLogParserRuleAtPosition(rulesNode, ruleXmlUtf8, insertPosition))
      {
         context->log(NXLOG_ERROR, _T("ImportLogParser()"), _T("Failed to parse/insert %s parser rule with GUID %hs"), parserName, guidUtf8);
      }
   }
   
   // Clean up
   MemFree(existingConfigUtf8);
   
   // Convert XML document to string
   xml_string_writer writer;
   xml.print(writer);
   ConfigWriteCLOB(configKey, writer.result, true);   
   context->log(NXLOG_INFO, _T("ImportLogParser()"), _T("%s parser rules import completed"), parserName);
   return true;
}

/**
 * Import configuration from JSON content  
 * @param content JSON configuration content
 * @param flags Import flags  
 * @param log Import log buffer
 * @return RCC status code
 */
uint32_t ImportConfigFromJson(const char* content, uint32_t flags, StringBuffer **log)
{
   ImportContext *context = new ImportContext();
   *log = context;
   
   json_error_t error;
   json_t* root = json_loads(content, 0, &error);
   if (root == nullptr)
   {
      context->log(NXLOG_ERROR, _T("ImportConfigFromJson()"), 
         _T("JSON parse error: %hs at line %d column %d"), error.text, error.line, error.column);
      return RCC_CONFIG_PARSE_ERROR;
   }

   // Validate root object structure
   if (!json_is_object(root))
   {
      context->log(NXLOG_ERROR, _T("ImportConfigFromJson()"), _T("JSON root is not an object"));
      json_decref(root);
      return RCC_CONFIG_PARSE_ERROR;
   }

   // Validate configuration before import
   if (!ValidateConfig(root, flags, context))
   {
      context->log(NXLOG_ERROR, _T("ImportConfigFromJson()"), _T("Configuration validation failed"));
      json_decref(root);
      return RCC_CONFIG_VALIDATION_ERROR;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportConfigFromJson() called, flags=0x%04X"), flags);

   uint32_t rcc = RCC_SUCCESS;
   json_t *events, *traps, *templates, *actions, *rules, *scripts, *objectTools, *summaryTables, *webServices, *assets, *syslog, *winlog;
   
   // Import events
   events = json_object_get(root, "events");
   if (json_is_array(events))
   {
      rcc = ImportJsonEvents(events, flags, context);
      if (rcc != RCC_SUCCESS)
         goto cleanup;
   }

   // Import traps  
   traps = json_object_get(root, "traps");
   if (json_is_array(traps))
   {
      rcc = ImportJsonTraps(traps, flags, context);
      if (rcc != RCC_SUCCESS)
         goto cleanup;
   }

   // Import templates
   templates = json_object_get(root, "templates");
   if (json_is_array(templates))
   {
      rcc = ImportJsonTemplates(templates, flags, context);
      if (rcc != RCC_SUCCESS)
         goto cleanup;
   }

   // Import actions
   actions = json_object_get(root, "actions");
   if (json_is_array(actions))
   {
      rcc = ImportJsonActions(actions, flags, context);
      if (rcc != RCC_SUCCESS)
         goto cleanup;
   }

   // Import rules
   rules = json_object_get(root, "rules");
   if (json_is_array(rules))
   {
      rcc = ImportJsonRules(rules, root, flags, context);
      if (rcc != RCC_SUCCESS)
         goto cleanup;
   }

   // Import scripts
   scripts = json_object_get(root, "scripts");
   if (json_is_array(scripts))
   {
      rcc = ImportJsonScripts(scripts, flags, context);
      if (rcc != RCC_SUCCESS)
         goto cleanup;
   }

   // Import object tools
   objectTools = json_object_get(root, "objectTools");
   if (json_is_array(objectTools))
   {
      rcc = ImportJsonObjectTools(objectTools, flags, context);
      if (rcc != RCC_SUCCESS)
         goto cleanup;
   }

   // Import DCI summary tables
   summaryTables = json_object_get(root, "dciSummaryTables");
   if (json_is_array(summaryTables))
   {
      size_t count = json_array_size(summaryTables);
      context->log(NXLOG_INFO, _T("ImportConfigFromJson()"), _T("%d DCI summary tables to import"), (int)count);
      
      size_t index;
      json_t *table;
      json_array_foreach(summaryTables, index, table)
      {
         if (json_is_object(table))
         {
            if (!ImportSummaryTable(table, (flags & CFG_IMPORT_REPLACE_SUMMARY_TABLES) != 0, context))
            {
               context->log(NXLOG_ERROR, _T("ImportConfigFromJson()"), _T("Failed to import DCI summary table"));
            }
         }
      }
      context->log(NXLOG_INFO, _T("ImportConfigFromJson()"), _T("DCI summary tables import completed"));
   }

   // Import web service definitions
   webServices = json_object_get(root, "webServiceDefinitions");
   if (json_is_array(webServices))
   {
      size_t count = json_array_size(webServices);
      context->log(NXLOG_INFO, _T("ImportConfigFromJson()"), _T("%d web service definitions to import"), (int)count);
      
      size_t index;
      json_t *webService;
      json_array_foreach(webServices, index, webService)
      {
         if (json_is_object(webService))
         {
            if (!ImportWebServiceDefinition(webService, (flags & CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS) != 0, context))
            {
               context->log(NXLOG_ERROR, _T("ImportConfigFromJson()"), _T("Failed to import web service definition"));
            }
         }
      }
      context->log(NXLOG_INFO, _T("ImportConfigFromJson()"), _T("Web service definitions import completed"));
   }

   // Import asset management schema
   assets = json_object_get(root, "assetManagementSchema");
   if (json_is_array(assets))
   {
      context->log(NXLOG_INFO, _T("ImportConfigFromJson()"), _T("Importing asset management schema"));
      ImportAssetManagementSchema(assets, (flags & CFG_IMPORT_REPLACE_AM_DEFINITIONS) != 0, context);
      context->log(NXLOG_INFO, _T("ImportConfigFromJson()"), _T("Asset management schema import completed"));
   }

   // Import syslog parser rules
   syslog = json_object_get(root, "syslog");
   if (json_is_object(syslog))
   {
      if (!ImportLogParser(syslog, flags, context, _T("Syslog"), _T("SyslogParser")))
      {
         context->log(NXLOG_ERROR, _T("ImportConfigFromJson()"), _T("Failed to import syslog parser rules"));
      }
   }

   // Import winlog parser rules
   winlog = json_object_get(root, "winlog");
   if (json_is_object(winlog))
   {
      if (!ImportLogParser(winlog, flags, context, _T("WindowsEventLog"), _T("WindowsEventParser")))
      {
         context->log(NXLOG_ERROR, _T("ImportConfigFromJson()"), _T("Failed to import winlog parser rules"));
      }
   }
   
   context->log(NXLOG_INFO, _T("ImportConfigFromJson()"), _T("JSON configuration import completed"));

cleanup:
   json_decref(root);
   return rcc;
}

/**
 * Import configuration from raw content (auto-detect format)
 * @param content Configuration content
 * @param flags Import flags
 * @param log Import log buffer  
 * @return RCC status code
 */
uint32_t ImportConfigFromContent(const char* content, uint32_t flags, StringBuffer **log)
{
   ConfigurationFormat format = DetectConfigurationFormat(content);
   
   switch (format)
   {
      case CONFIG_FORMAT_JSON:
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Importing JSON configuration"));
         return ImportConfigFromJson(content, flags, log);
         
      case CONFIG_FORMAT_XML:
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Importing XML configuration (legacy)"));
            Config config(false);
            if (config.loadXmlConfigFromMemory(content, strlen(content), nullptr, "configuration", false))
            {
               return ImportConfig(config, flags, log);
            }
            else
            {
               ImportContext *context = new ImportContext();
               *log = context;
               context->log(NXLOG_ERROR, _T("ImportConfigFromContent()"), _T("XML configuration parse error"));
               return RCC_CONFIG_PARSE_ERROR;
            }
         }
         
      default:
         {
            ImportContext *context = new ImportContext();
            *log = context;
            context->log(NXLOG_ERROR, _T("ImportConfigFromContent()"), _T("Unknown configuration format"));
            return RCC_CONFIG_PARSE_ERROR;
         }
   }
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
         if (MatchString(_T("*.xml"), f->d_name, FALSE) || MatchString(_T("*.json"), f->d_name, FALSE))
         {
            _tcscpy(&path[insPos], f->d_name);
            
            // Read file content
            FILE *file = _tfopen(path, _T("r"));
            if (file != nullptr)
            {
               fseek(file, 0, SEEK_END);
               long size = ftell(file);
               fseek(file, 0, SEEK_SET);
               
               char *content = (char*)malloc(size + 1);
               if (content != nullptr)
               {
                  size_t bytesRead = fread(content, 1, size, file);
                  content[bytesRead] = 0;
                  
                  StringBuffer *log;
                  uint32_t result = ImportConfigFromContent(content, overwrite ? CFG_IMPORT_REPLACE_EVERYTHING : 0, &log);
                  if (result != RCC_SUCCESS)
                  {
                     nxlog_debug_tag(DEBUG_TAG, 1, _T("Error importing configuration from %s (RCC=%u)"), path, result);
                  }
                  else
                  {
                     count++;
                     nxlog_debug_tag(DEBUG_TAG, 3, _T("Configuration imported from %s"), path);
                  }
                  delete log;
                  MemFree(content);
               }
               fclose(file);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 1, _T("Error opening configuration file %s"), path);
            }
         }
      }
      _tclosedir(dir);
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d configuration files processed"), count);
}
