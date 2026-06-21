/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: dashboards.cpp
**
** AI assistant skill for building and editing dashboards (issue #3344).
**
**/

#include "aitools.h"
#include <vector>

/**
 * Field of an element configuration schema
 */
struct ElementFieldSchema
{
   const char *key;
   const char *description;
   bool required;
};

/**
 * Descriptor for a supported dashboard element type
 */
struct ElementTypeDescriptor
{
   const char *name;          // kebab-case type name used by the assistant
   int typeCode;              // internal element type code (see DashboardElement type constants)
   const char *category;      // "reference", "chart", or "static"
   const char *description;   // one-line description
   std::vector<ElementFieldSchema> fields;
   const char *example;       // filled JSON configuration example
};

/**
 * Element type codes (mirror of org.netxms.client.dashboards.DashboardElement constants)
 */
#define DCE_LABEL                0
#define DCE_LINE_CHART           1
#define DCE_BAR_CHART            2
#define DCE_PIE_CHART            3
#define DCE_EMBEDDED_DASHBOARD   7
#define DCE_NETWORK_MAP          8
#define DCE_GEO_MAP              10
#define DCE_ALARM_VIEWER         11
#define DCE_AVAILABILITY_CHART   12
#define DCE_DIAL_CHART           13
#define DCE_WEB_PAGE             14
#define DCE_SEPARATOR            18
#define DCE_TABLE_VALUE          19
#define DCE_STATUS_MAP           20
#define DCE_DCI_SUMMARY_TABLE    21
#define DCE_SERVICE_COMPONENTS   25
#define DCE_RACK_DIAGRAM         26

/**
 * Get table of supported element type descriptors (curated set for v1, see issue #3344)
 */
static const std::vector<ElementTypeDescriptor>& GetElementTypeDescriptors()
{
   static const std::vector<ElementTypeDescriptor> descriptors =
   {
      // Reference elements (configuration is mostly an object or URL reference)
      {
         "network-map", DCE_NETWORK_MAP, "reference",
         "Display an existing network map object",
         {
            { "objectId", "ID of network map object to display", true },
            { "zoomLevel", "initial zoom level in percent (default 100)", false }
         },
         "{\"objectId\":342,\"zoomLevel\":100}"
      },
      {
         "rack-diagram", DCE_RACK_DIAGRAM, "reference",
         "Display a rack and its installed equipment",
         {
            { "objectId", "ID of rack object to display", true }
         },
         "{\"objectId\":118}"
      },
      {
         "geo-map", DCE_GEO_MAP, "reference",
         "Geographic map showing objects with location at a given coordinate/zoom",
         {
            { "rootObjectId", "ID of object whose children are shown on the map (0 for all)", false },
            { "latitude", "map center latitude", false },
            { "longitude", "map center longitude", false },
            { "zoom", "map zoom level (default 14)", false }
         },
         "{\"rootObjectId\":0,\"latitude\":47.3769,\"longitude\":8.5417,\"zoom\":12}"
      },
      {
         "status-map", DCE_STATUS_MAP, "reference",
         "Status map showing colored tiles for child objects of a container",
         {
            { "objectId", "ID of container/parent object", true },
            { "severityFilter", "bitmask of alarm severities to include (default 0xFF = all)", false },
            { "groupObjects", "group objects by parent (default false)", false }
         },
         "{\"objectId\":15,\"severityFilter\":255,\"groupObjects\":false}"
      },
      {
         "alarm-viewer", DCE_ALARM_VIEWER, "reference",
         "List of active alarms for an object subtree",
         {
            { "objectId", "ID of root object for alarm scope", true },
            { "severityFilter", "bitmask of alarm severities to include (default 0xFF = all)", false },
            { "stateFilter", "bitmask of alarm states to include (default 0xFF = all)", false }
         },
         "{\"objectId\":15,\"severityFilter\":255,\"stateFilter\":255}"
      },
      {
         "service-components", DCE_SERVICE_COMPONENTS, "reference",
         "Topology view of components of a business service or container",
         {
            { "objectId", "ID of object whose components are shown", true }
         },
         "{\"objectId\":215}"
      },
      {
         "web-page", DCE_WEB_PAGE, "reference",
         "Embed an arbitrary web page",
         {
            { "url", "URL of the page to embed", true }
         },
         "{\"url\":\"https://grafana.example.com/d/abc\"}"
      },
      {
         "embedded-dashboard", DCE_EMBEDDED_DASHBOARD, "reference",
         "Embed one or more other dashboards, optionally rotating between them",
         {
            { "dashboardObjects", "array of dashboard object IDs to embed", true },
            { "displayInterval", "rotation interval in seconds when multiple dashboards are given (default 60)", false }
         },
         "{\"dashboardObjects\":[401,402],\"displayInterval\":30}"
      },

      // DCI-driven elements
      {
         "line-chart", DCE_LINE_CHART, "chart",
         "Time-series line chart of one or more DCIs",
         {
            { "title", "chart title", false },
            { "dciList", "array of DCI references, each {nodeId, dciId, name?, color?}", true },
            { "timeRange", "number of time units to display (default 1)", false },
            { "timeUnits", "time unit: 0=minutes, 1=hours, 2=days (default 1)", false },
            { "showLegend", "show legend (default true)", false },
            { "stacked", "stack series (default false)", false },
            { "autoScale", "auto-scale Y axis (default true)", false },
            { "area", "render as area chart (default false)", false }
         },
         "{\"title\":\"CPU and memory\",\"dciList\":[{\"nodeId\":342,\"dciId\":1001,\"name\":\"CPU usage\"},{\"nodeId\":342,\"dciId\":1002,\"name\":\"Memory usage\"}],\"timeRange\":6,\"timeUnits\":1,\"showLegend\":true}"
      },
      {
         "bar-chart", DCE_BAR_CHART, "chart",
         "Bar chart comparing current values of several DCIs",
         {
            { "title", "chart title", false },
            { "dciList", "array of DCI references, each {nodeId, dciId, name?, color?}", true },
            { "showLegend", "show legend (default true)", false },
            { "transposed", "draw horizontal bars (default false)", false }
         },
         "{\"title\":\"Interface traffic\",\"dciList\":[{\"nodeId\":342,\"dciId\":2001,\"name\":\"In\"},{\"nodeId\":342,\"dciId\":2002,\"name\":\"Out\"}]}"
      },
      {
         "pie-chart", DCE_PIE_CHART, "chart",
         "Pie chart of current values of several DCIs",
         {
            { "title", "chart title", false },
            { "dciList", "array of DCI references, each {nodeId, dciId, name?, color?}", true },
            { "showLegend", "show legend (default true)", false },
            { "showTotal", "show total value (default false)", false }
         },
         "{\"title\":\"Disk usage\",\"dciList\":[{\"nodeId\":342,\"dciId\":3001,\"name\":\"Used\"},{\"nodeId\":342,\"dciId\":3002,\"name\":\"Free\"}]}"
      },
      {
         "dial-chart", DCE_DIAL_CHART, "chart",
         "Gauge/dial showing the current value of one or more DCIs that share the same scale and zones",
         {
            { "title", "gauge title", false },
            { "dciList", "array of one or more DCI references {nodeId, dciId, name?}; the scale (minValue/maxValue) and zones apply to all of them. Combine every DCI that shares the same scale into ONE dial-chart element instead of one element each - this is keyed on the numeric scale, NOT on what the metric measures, so e.g. CPU usage %, memory %, and disk usage % (all 0-100) belong in a single dial-chart element", true },
            { "minValue", "scale minimum, shared by all gauges (default 0)", false },
            { "maxValue", "scale maximum, shared by all gauges (default 100)", false },
            { "gaugeType", "0=dial, 1=bar, 2=text, 3=circular (default 0)", false }
         },
         "{\"title\":\"CPU usage\",\"dciList\":[{\"nodeId\":342,\"dciId\":1001,\"name\":\"node1\"},{\"nodeId\":343,\"dciId\":1001,\"name\":\"node2\"}],\"minValue\":0,\"maxValue\":100}"
      },
      {
         "availability-chart", DCE_AVAILABILITY_CHART, "chart",
         "Availability pie chart for a business service",
         {
            { "objectId", "ID of business service object", true },
            { "period", "period: 0=today, 2=this week, 4=this month, 6=this year, 8=custom (default 0)", false },
            { "numberOfDays", "number of days when using a custom day-based period", false }
         },
         "{\"objectId\":215,\"period\":4}"
      },
      {
         "table-value", DCE_TABLE_VALUE, "chart",
         "Show the last value of a table DCI as a grid",
         {
            { "objectId", "ID of object owning the table DCI", true },
            { "dciId", "ID of the table DCI", true }
         },
         "{\"objectId\":342,\"dciId\":4001}"
      },
      {
         "dci-summary-table", DCE_DCI_SUMMARY_TABLE, "chart",
         "Render a configured DCI summary table for an object subtree",
         {
            { "baseObjectId", "ID of base object for the summary", true },
            { "tableId", "ID of the DCI summary table definition", true }
         },
         "{\"baseObjectId\":15,\"tableId\":3}"
      },

      // Static elements
      {
         "label", DCE_LABEL, "static",
         "Static text label, useful as a section heading",
         {
            { "title", "label text", true }
         },
         "{\"title\":\"Production servers\"}"
      },
      {
         "separator", DCE_SEPARATOR, "static",
         "Horizontal separator line",
         {
         },
         "{}"
      }
   };
   return descriptors;
}

/**
 * Find element type descriptor by name (case-insensitive). Returns nullptr if not found.
 */
static const ElementTypeDescriptor *FindElementTypeDescriptor(const char *name)
{
   if (name == nullptr)
      return nullptr;
   for(const ElementTypeDescriptor& d : GetElementTypeDescriptors())
   {
      if (!stricmp(d.name, name))
         return &d;
   }
   return nullptr;
}

/**
 * Find element type descriptor by type code. Returns nullptr if not found.
 */
static const ElementTypeDescriptor *FindElementTypeDescriptorByCode(int typeCode)
{
   for(const ElementTypeDescriptor& d : GetElementTypeDescriptors())
   {
      if (d.typeCode == typeCode)
         return &d;
   }
   return nullptr;
}

/**
 * Convert wide string to UTF-8 std::string
 */
static std::string ToUtf8(const wchar_t *s)
{
   char buffer[1024];
   size_t len = wchar_to_utf8(s, -1, buffer, sizeof(buffer) - 1);
   buffer[(len < sizeof(buffer)) ? len : sizeof(buffer) - 1] = 0;
   return std::string(buffer);
}

/**
 * Build descriptor JSON for a single element type
 */
static json_t *ElementTypeToJson(const ElementTypeDescriptor& d, bool full)
{
   json_t *root = json_object();
   json_object_set_new(root, "type", json_string(d.name));
   json_object_set_new(root, "category", json_string(d.category));
   json_object_set_new(root, "description", json_string(d.description));
   if (full)
   {
      json_t *fields = json_array();
      for(const ElementFieldSchema& f : d.fields)
      {
         json_t *field = json_object();
         json_object_set_new(field, "key", json_string(f.key));
         json_object_set_new(field, "description", json_string(f.description));
         json_object_set_new(field, "required", json_boolean(f.required));
         json_array_append_new(fields, field);
      }
      json_object_set_new(root, "fields", fields);

      json_error_t error;
      json_t *example = json_loads(d.example, 0, &error);
      json_object_set_new(root, "configExample", (example != nullptr) ? example : json_object());
   }
   return root;
}

/**
 * Extract a configuration object from arguments. The assistant may pass the configuration either as a JSON
 * object or as a JSON-encoded string. Returns a new (owned) JSON object, or nullptr if absent/invalid.
 */
static json_t *ExtractConfigObject(json_t *arguments, const char *key)
{
   json_t *config = json_object_get(arguments, key);
   if (config == nullptr)
      return nullptr;
   if (json_is_string(config))
   {
      json_error_t error;
      json_t *parsed = json_loads(json_string_value(config), 0, &error);
      return (parsed != nullptr && json_is_object(parsed)) ? parsed : nullptr;
   }
   if (json_is_object(config))
      return json_deep_copy(config);
   return nullptr;
}

/**
 * Check that all required fields of a type are present and meaningful in the configuration.
 * Returns empty string on success or an error message.
 */
static std::string ValidateRequiredFields(const ElementTypeDescriptor& d, json_t *config)
{
   for(const ElementFieldSchema& f : d.fields)
   {
      if (!f.required)
         continue;

      json_t *value = json_object_get(config, f.key);
      bool missing;
      if (value == nullptr)
         missing = true;
      else if (json_is_array(value))
         missing = (json_array_size(value) == 0);
      else if (json_is_string(value))
         missing = (json_string_value(value)[0] == 0);
      else if (json_is_integer(value))
         missing = (json_integer_value(value) == 0);
      else
         missing = false;

      if (missing)
         return std::string("Missing required configuration field \"") + f.key + "\" for element type \"" + d.name + "\"";
   }
   return std::string();
}

/**
 * Validate an object reference: object must exist and be readable by the user.
 */
static std::string ValidateObjectReference(uint32_t objectId, uint32_t userId, const char *role)
{
   if (objectId == 0)
      return std::string();
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
   {
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "%u", objectId);
      return std::string("Referenced object [") + buffer + "] (" + role + ") does not exist";
   }
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      return std::string("Access denied to referenced object \"") + ToUtf8(object->getName()) + "\" (" + role + ")";
   }
   return std::string();
}

/**
 * Recursively validate object and DCI references inside an element configuration.
 * Returns empty string on success or a descriptive error message.
 */
static std::string ValidateReferences(json_t *node, uint32_t userId)
{
   if (json_is_array(node))
   {
      size_t i;
      json_t *value;
      json_array_foreach(node, i, value)
      {
         if (json_is_object(value) || json_is_array(value))
         {
            std::string error = ValidateReferences(value, userId);
            if (!error.empty())
               return error;
         }
      }
      return std::string();
   }

   if (!json_is_object(node))
      return std::string();

   // Validate DCI reference (requires owning object from a sibling key)
   uint32_t dciId = json_object_get_uint32(node, "dciId");
   if (dciId != 0)
   {
      uint32_t ownerId = json_object_get_uint32(node, "nodeId");
      if (ownerId == 0)
         ownerId = json_object_get_uint32(node, "objectId");
      if (ownerId == 0)
         ownerId = json_object_get_uint32(node, "baseObjectId");

      char dciBuffer[64];
      snprintf(dciBuffer, sizeof(dciBuffer), "%u", dciId);

      if (ownerId == 0)
         return std::string("DCI [") + dciBuffer + "] reference has no owning object (nodeId/objectId)";

      shared_ptr<NetObj> owner = FindObjectById(ownerId);
      if (owner == nullptr)
      {
         char ownerBuffer[64];
         snprintf(ownerBuffer, sizeof(ownerBuffer), "%u", ownerId);
         return std::string("Object [") + ownerBuffer + "] owning DCI [" + dciBuffer + "] does not exist";
      }
      if (!owner->checkAccessRights(userId, OBJECT_ACCESS_READ))
         return std::string("Access denied to object \"") + ToUtf8(owner->getName()) + "\" owning DCI [" + dciBuffer + "]";
      if (!owner->isDataCollectionTarget())
         return std::string("Object \"") + ToUtf8(owner->getName()) + "\" is not a data collection target";
      if (static_cast<DataCollectionTarget&>(*owner).getDCObjectById(dciId, userId) == nullptr)
         return std::string("DCI [") + dciBuffer + "] does not exist on object \"" + ToUtf8(owner->getName()) + "\"";
   }

   // Validate object references
   static const char *objectRefKeys[] = { "objectId", "nodeId", "baseObjectId", "rootObjectId", "contextObjectId", "drillDownObjectId", nullptr };
   for(int i = 0; objectRefKeys[i] != nullptr; i++)
   {
      json_t *value = json_object_get(node, objectRefKeys[i]);
      if (json_is_integer(value))
      {
         std::string error = ValidateObjectReference(static_cast<uint32_t>(json_integer_value(value)), userId, objectRefKeys[i]);
         if (!error.empty())
            return error;
      }
   }

   // Validate embedded dashboard references
   json_t *dashboardObjects = json_object_get(node, "dashboardObjects");
   if (json_is_array(dashboardObjects))
   {
      size_t i;
      json_t *value;
      json_array_foreach(dashboardObjects, i, value)
      {
         if (json_is_integer(value))
         {
            uint32_t id = static_cast<uint32_t>(json_integer_value(value));
            shared_ptr<NetObj> object = FindObjectById(id, OBJECT_DASHBOARD);
            if (object == nullptr)
            {
               char buffer[64];
               snprintf(buffer, sizeof(buffer), "%u", id);
               return std::string("Embedded dashboard [") + buffer + "] does not exist";
            }
            if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
               return std::string("Access denied to embedded dashboard \"") + ToUtf8(object->getName()) + "\"";
         }
      }
   }

   // Recurse into nested values
   const char *key;
   json_t *value;
   json_object_foreach(node, key, value)
   {
      if (json_is_object(value) || json_is_array(value))
      {
         std::string error = ValidateReferences(value, userId);
         if (!error.empty())
            return error;
      }
   }
   return std::string();
}

/**
 * Build element layout JSON from width/height intent. Horizontal span is clamped to dashboard column count.
 * Any clamping is appended to the notes buffer.
 */
static json_t *BuildLayout(json_t *arguments, int numColumns, StringBuffer& notes)
{
   const char *width = json_object_get_string_utf8(arguments, "width", "full");

   int span;
   if (!stricmp(width, "full"))
      span = numColumns;
   else if (!stricmp(width, "half"))
      span = numColumns / 2;
   else if (!stricmp(width, "third"))
      span = numColumns / 3;
   else if (!stricmp(width, "quarter"))
      span = numColumns / 4;
   else
      span = numColumns;

   if (span < 1)
      span = 1;
   if (span > numColumns)
   {
      notes.append(L"width reduced to dashboard column count; ");
      span = numColumns;
   }

   int heightHint = json_object_get_int32(arguments, "height", -1);
   if (heightHint <= 0)
      heightHint = -1;

   bool grabVerticalSpace = json_object_get_boolean(arguments, "grabVerticalSpace", true);

   json_t *layout = json_object();
   json_object_set_new(layout, "horizontalSpan", json_integer(span));
   json_object_set_new(layout, "verticalSpan", json_integer(1));
   json_object_set_new(layout, "grabVerticalSpace", json_boolean(grabVerticalSpace));
   json_object_set_new(layout, "heightHint", json_integer(heightHint));
   return layout;
}

/**
 * Resolve dashboard from arguments, enforcing object class and access rights.
 * On failure, sets errorOut and returns nullptr.
 */
static shared_ptr<Dashboard> ResolveDashboard(json_t *arguments, uint32_t userId, uint32_t requiredAccess, std::string *errorOut)
{
   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "dashboard", OBJECT_DASHBOARD);
   if (object == nullptr)
   {
      *errorOut = "Dashboard not found";
      return shared_ptr<Dashboard>();
   }
   if (object->getObjectClass() != OBJECT_DASHBOARD)
   {
      *errorOut = "Object is not a dashboard";
      return shared_ptr<Dashboard>();
   }
   if (!object->checkAccessRights(userId, requiredAccess))
   {
      *errorOut = "Access denied";
      return shared_ptr<Dashboard>();
   }
   return static_pointer_cast<Dashboard>(object);
}

/**
 * List supported dashboard element types
 */
std::string F_ListDashboardElementTypes(json_t *arguments, uint32_t userId)
{
   json_t *output = json_array();
   for(const ElementTypeDescriptor& d : GetElementTypeDescriptors())
      json_array_append_new(output, ElementTypeToJson(d, false));
   return JsonToString(output);
}

/**
 * Describe a single dashboard element type (full schema + example)
 */
std::string F_DescribeDashboardElementType(json_t *arguments, uint32_t userId)
{
   const char *name = json_object_get_string_utf8(arguments, "type", nullptr);
   const ElementTypeDescriptor *d = FindElementTypeDescriptor(name);
   if (d == nullptr)
      return std::string("Unknown element type. Use list-dashboard-element-types to enumerate supported types.");
   return JsonToString(ElementTypeToJson(*d, true));
}

/**
 * Create a new (empty) dashboard
 */
std::string F_CreateDashboard(json_t *arguments, uint32_t userId)
{
   wchar_t name[MAX_OBJECT_NAME];
   utf8_to_wchar(json_object_get_string_utf8(arguments, "name", ""), -1, name, MAX_OBJECT_NAME);
   name[MAX_OBJECT_NAME - 1] = 0;
   if ((name[0] == 0) || !IsValidObjectName(name, TRUE))
      return std::string("A valid dashboard name must be provided");

   int columns = json_object_get_int32(arguments, "columns", 2);
   if (columns < 1)
      columns = 1;
   else if (columns > 12)
      columns = 12;

   // Resolve parent (dashboard group or dashboard root)
   shared_ptr<NetObj> parent;
   if (json_object_get(arguments, "group") != nullptr)
   {
      parent = FindObjectByNameOrId(arguments, "group");
      if (parent == nullptr)
         return std::string("Dashboard group not found");
      if ((parent->getObjectClass() != OBJECT_DASHBOARDGROUP) && (parent->getObjectClass() != OBJECT_DASHBOARDROOT))
         return std::string("Specified group is not a dashboard group");
   }
   else
   {
      parent = g_dashboardRoot;
   }

   if (!parent->checkAccessRights(userId, OBJECT_ACCESS_CREATE))
      return std::string("Access denied: no create permission on parent");

   // Optional object to associate the dashboard with
   shared_ptr<NetObj> associatedObject;
   if (json_object_get(arguments, "associateWith") != nullptr)
   {
      associatedObject = FindObjectByNameOrId(arguments, "associateWith");
      if (associatedObject == nullptr)
         return std::string("Object specified in associateWith not found");
      if (!associatedObject->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
         return std::string("Access denied to object specified in associateWith");
   }

   auto dashboard = make_shared<Dashboard>(name);
   NetObjInsert(dashboard, true, false);
   dashboard->setColumnCount(columns);
   NetObj::linkObjects(parent, dashboard);
   parent->calculateCompoundStatus();
   dashboard->publish();

   if (associatedObject != nullptr)
      associatedObject->addDashboard(dashboard->getId());

   WriteAuditLog(AUDIT_OBJECTS, true, userId, nullptr, 0, dashboard->getId(),
      L"AI assistant created dashboard \"%s\" [%u]", dashboard->getName(), dashboard->getId());

   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(dashboard->getId()));
   json_object_set_new(output, "guid", dashboard->getGuid().toJson());
   json_object_set_new(output, "name", json_string_t(dashboard->getName()));
   json_object_set_new(output, "numColumns", json_integer(columns));
   return JsonToString(output);
}

/**
 * Read dashboard structure including ordered elements with their GUIDs
 */
std::string F_GetDashboard(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Dashboard> dashboard = ResolveDashboard(arguments, userId, OBJECT_ACCESS_READ, &error);
   if (dashboard == nullptr)
      return error;

   json_t *elements = dashboard->getElementsAsJson();

   // Enrich each element with its type name
   size_t i;
   json_t *element;
   json_array_foreach(elements, i, element)
   {
      const ElementTypeDescriptor *d = FindElementTypeDescriptorByCode(json_object_get_int32(element, "type"));
      if (d != nullptr)
         json_object_set_new(element, "typeName", json_string(d->name));
   }

   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(dashboard->getId()));
   json_object_set_new(output, "name", json_string_t(dashboard->getName()));
   json_object_set_new(output, "numColumns", json_integer(dashboard->getColumnCount()));
   json_object_set_new(output, "elements", elements);
   return JsonToString(output);
}

/**
 * Append an element to a dashboard
 */
std::string F_AddDashboardElement(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Dashboard> dashboard = ResolveDashboard(arguments, userId, OBJECT_ACCESS_MODIFY, &error);
   if (dashboard == nullptr)
      return error;

   const ElementTypeDescriptor *d = FindElementTypeDescriptor(json_object_get_string_utf8(arguments, "type", nullptr));
   if (d == nullptr)
      return std::string("Unknown element type. Use list-dashboard-element-types to enumerate supported types.");

   json_t *config = ExtractConfigObject(arguments, "config");
   if (config == nullptr)
      config = json_object();   // static elements may have empty configuration

   std::string fieldError = ValidateRequiredFields(*d, config);
   if (!fieldError.empty())
   {
      json_decref(config);
      return fieldError;
   }

   std::string refError = ValidateReferences(config, userId);
   if (!refError.empty())
   {
      json_decref(config);
      return refError;
   }

   StringBuffer notes;
   json_t *layout = BuildLayout(arguments, dashboard->getColumnCount(), notes);

   uuid guid = dashboard->addElement(d->typeCode, config, layout);

   json_t *output = json_object();
   json_object_set_new(output, "result", json_string("element added"));
   json_object_set_new(output, "guid", guid.toJson());
   if (!notes.isEmpty())
      json_object_set_new(output, "notes", json_string_t(notes));
   return JsonToString(output);
}

/**
 * Replace configuration of an existing element identified by GUID
 */
std::string F_UpdateDashboardElement(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Dashboard> dashboard = ResolveDashboard(arguments, userId, OBJECT_ACCESS_MODIFY, &error);
   if (dashboard == nullptr)
      return error;

   uuid guid = uuid::parseA(json_object_get_string_utf8(arguments, "guid", ""));
   if (guid.isNull())
      return std::string("A valid element GUID must be provided");

   json_t *config = ExtractConfigObject(arguments, "config");
   if (config == nullptr)
      return std::string("A valid configuration object must be provided");

   // Determine element type to validate required fields (look up current element type)
   json_t *elements = dashboard->getElementsAsJson();
   const ElementTypeDescriptor *d = nullptr;
   size_t i;
   json_t *element;
   json_array_foreach(elements, i, element)
   {
      uuid elementGuid = uuid::parseA(json_object_get_string_utf8(element, "guid", ""));
      if (elementGuid.equals(guid))
      {
         d = FindElementTypeDescriptorByCode(json_object_get_int32(element, "type"));
         break;
      }
   }
   json_decref(elements);

   if (d == nullptr)
   {
      json_decref(config);
      return std::string("Element with specified GUID not found on dashboard");
   }

   std::string fieldError = ValidateRequiredFields(*d, config);
   if (!fieldError.empty())
   {
      json_decref(config);
      return fieldError;
   }

   std::string refError = ValidateReferences(config, userId);
   if (!refError.empty())
   {
      json_decref(config);
      return refError;
   }

   // Optionally update layout if width/height provided
   json_t *layout = nullptr;
   StringBuffer notes;
   if ((json_object_get(arguments, "width") != nullptr) || (json_object_get(arguments, "height") != nullptr) ||
       (json_object_get(arguments, "grabVerticalSpace") != nullptr))
      layout = BuildLayout(arguments, dashboard->getColumnCount(), notes);

   if (!dashboard->updateElement(guid, config, layout))
      return std::string("Element with specified GUID not found on dashboard");

   json_t *output = json_object();
   json_object_set_new(output, "result", json_string("element updated"));
   if (!notes.isEmpty())
      json_object_set_new(output, "notes", json_string_t(notes));
   return JsonToString(output);
}

/**
 * Remove an element identified by GUID
 */
std::string F_RemoveDashboardElement(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Dashboard> dashboard = ResolveDashboard(arguments, userId, OBJECT_ACCESS_MODIFY, &error);
   if (dashboard == nullptr)
      return error;

   uuid guid = uuid::parseA(json_object_get_string_utf8(arguments, "guid", ""));
   if (guid.isNull())
      return std::string("A valid element GUID must be provided");

   if (!dashboard->removeElement(guid))
      return std::string("Element with specified GUID not found on dashboard");

   return std::string("Element removed");
}

/**
 * Move an element identified by GUID to a new position
 */
std::string F_MoveDashboardElement(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Dashboard> dashboard = ResolveDashboard(arguments, userId, OBJECT_ACCESS_MODIFY, &error);
   if (dashboard == nullptr)
      return error;

   uuid guid = uuid::parseA(json_object_get_string_utf8(arguments, "guid", ""));
   if (guid.isNull())
      return std::string("A valid element GUID must be provided");

   if (json_object_get(arguments, "position") == nullptr)
      return std::string("Target position must be provided");
   int position = json_object_get_int32(arguments, "position", 0);

   if (!dashboard->moveElement(guid, position))
      return std::string("Element with specified GUID not found on dashboard");

   return std::string("Element moved");
}
