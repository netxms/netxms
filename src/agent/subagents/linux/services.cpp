/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2025 Raden Solutions
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
**/

#include "linux_subagent.h"

#ifdef HAVE_SDBUS

#include <systemd/sd-bus.h>

/**
 * Structure to hold service information
 */
struct ServiceInfo
{
   char name[256];
   char loadState[64];
   char activeState[64];
   char subState[64];
   char description[512];
   char unitFileState[64];
   uint32_t mainPid;
   char execMainStart[512];
};

/**
 * Get service information via D-Bus for a specific service
 */
static bool GetServiceInfoViaDbus(const char *serviceName, ServiceInfo *info)
{
   // Connect to system bus
   sd_bus *bus = nullptr;
   int ret = sd_bus_open_system(&bus);
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetServiceInfoViaDbus: Failed to connect to system bus (%hs)"), strerror(-ret));
      return false;
   }

   // Initialize the structure
   memset(info, 0, sizeof(ServiceInfo));
   strlcpy(info->name, serviceName, sizeof(info->name));

   // Get unit properties
   sd_bus_error error = SD_BUS_ERROR_NULL;
   sd_bus_message *message = nullptr;
   ret = sd_bus_call_method(bus,
                           "org.freedesktop.systemd1",
                           "/org/freedesktop/systemd1",
                           "org.freedesktop.systemd1.Manager",
                           "GetUnit",
                           &error,
                           &message,
                           "s",
                           serviceName);
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetServiceInfoViaDbus: Failed to get unit %hs (%hs)"), serviceName, error.message);
      goto cleanup;
   }

   // Get unit path
   char unitPath[512];
   const char *unitPathPtr;
   if (sd_bus_message_read(message, "o", &unitPathPtr) < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetServiceInfoViaDbus: Failed to read unit path"));
      goto cleanup;
   }
   strlcpy(unitPath, unitPathPtr, sizeof(unitPath));

   sd_bus_message_unref(message);
   message = nullptr;

   // Get unit properties
   ret = sd_bus_call_method(bus,
                           "org.freedesktop.systemd1",
                           unitPath,
                           "org.freedesktop.DBus.Properties",
                           "GetAll",
                           &error,
                           &message,
                           "s",
                           "org.freedesktop.systemd1.Unit");

   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetServiceInfoViaDbus: Failed to get unit properties (%hs)"), error.message);
      goto cleanup;
   }

   // Parse properties
   ret = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, "{sv}");
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("GetServiceInfoViaDbus: Failed to enter properties container"));
      goto cleanup;
   }

   const char *propName;
   while (sd_bus_message_enter_container(message, SD_BUS_TYPE_DICT_ENTRY, "sv") > 0)
   {
      if (sd_bus_message_read(message, "s", &propName) >= 0)
      {
         if (strcmp(propName, "ActiveState") == 0)
         {
            if (sd_bus_message_enter_container(message, SD_BUS_TYPE_VARIANT, "s") >= 0)
            {
               const char *value;
               if (sd_bus_message_read(message, "s", &value) >= 0)
                  strlcpy(info->activeState, value, sizeof(info->activeState));
               sd_bus_message_exit_container(message);
            }
         }
         else if (strcmp(propName, "SubState") == 0)
         {
            if (sd_bus_message_enter_container(message, SD_BUS_TYPE_VARIANT, "s") >= 0)
            {
               const char *value;
               if (sd_bus_message_read(message, "s", &value) >= 0)
                  strlcpy(info->subState, value, sizeof(info->subState));
               sd_bus_message_exit_container(message);
            }
         }
         else if (strcmp(propName, "LoadState") == 0)
         {
            if (sd_bus_message_enter_container(message, SD_BUS_TYPE_VARIANT, "s") >= 0)
            {
               const char *value;
               if (sd_bus_message_read(message, "s", &value) >= 0)
                  strlcpy(info->loadState, value, sizeof(info->loadState));
               sd_bus_message_exit_container(message);
            }
         }
         else
         {
            sd_bus_message_skip(message, "v");
         }
      }
      sd_bus_message_exit_container(message);
   }

cleanup:
   sd_bus_error_free(&error);
   sd_bus_message_unref(message);
   sd_bus_unref(bus);

   return (ret >= 0);
}

/**
 * List all systemd services via D-Bus (including inactive ones)
 */
static bool ListServicesViaDbus(ObjectArray<ServiceInfo> *services)
{
   // Connect to system bus
   sd_bus *bus = nullptr;
   int ret = sd_bus_open_system(&bus);
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ListServicesViaDbus: Failed to connect to system bus (%hs)"), strerror(-ret));
      return false;
   }

   // Use ListUnitFiles to get all service files (including inactive ones)
   sd_bus_error error = SD_BUS_ERROR_NULL;
   sd_bus_message *message = nullptr;
   ret = sd_bus_call_method(bus,
                           "org.freedesktop.systemd1",
                           "/org/freedesktop/systemd1",
                           "org.freedesktop.systemd1.Manager",
                           "ListUnitFiles",
                           &error,
                           &message,
                           "");
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ListServicesViaDbus: Failed to list unit files (%hs)"), error.message);
      goto cleanup;
   }

   // Parse the array of unit files - signature is a(ss)
   ret = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, "(ss)");
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ListServicesViaDbus: Failed to open message container (%hs)"), strerror(-ret));
      goto cleanup;
   }

   const char *unitPath, *enablementState;
   while(sd_bus_message_read(message, "(ss)", &unitPath, &enablementState) > 0)
   {
      // Extract unit name from path and check if it's a service
      const char *unitName = strrchr(unitPath, '/');
      if (unitName != nullptr)
         unitName++; // Skip the '/'
      else
         unitName = unitPath;

      // Only process .service units
      if (strstr(unitName, ".service") != nullptr)
      {
         ServiceInfo *info = new ServiceInfo();
         memset(info, 0, sizeof(ServiceInfo));
         
         strlcpy(info->name, unitName, sizeof(info->name));
         strlcpy(info->unitFileState, enablementState, sizeof(info->unitFileState));
         
         // Try to get the unit object path - this tells us if the service is loaded
         char actualUnitPath[512] = "";
         sd_bus_message *unitMessage = nullptr;
         sd_bus_error unitError = SD_BUS_ERROR_NULL;
         int unitRet = sd_bus_call_method(bus,
                                        "org.freedesktop.systemd1",
                                        "/org/freedesktop/systemd1",
                                        "org.freedesktop.systemd1.Manager",
                                        "GetUnit",
                                        &unitError,
                                        &unitMessage,
                                        "s",
                                        unitName);
         
         bool isLoaded = false;
         if (unitRet >= 0)
         {
            const char *pathPtr;
            if (sd_bus_message_read(unitMessage, "o", &pathPtr) >= 0)
            {
               strlcpy(actualUnitPath, pathPtr, sizeof(actualUnitPath));
               isLoaded = true;
            }
         }
         sd_bus_error_free(&unitError);
         sd_bus_message_unref(unitMessage);

         if (isLoaded)
         {
            // Service is loaded - get all properties from the unit object
            // Get Unit properties (including Description, ActiveState, SubState, LoadState)
            sd_bus_message *unitPropMessage = nullptr;
            sd_bus_error unitPropError = SD_BUS_ERROR_NULL;
            int unitPropRet = sd_bus_call_method(bus,
                                               "org.freedesktop.systemd1",
                                               actualUnitPath,
                                               "org.freedesktop.DBus.Properties",
                                               "GetAll",
                                               &unitPropError,
                                               &unitPropMessage,
                                               "s",
                                               "org.freedesktop.systemd1.Unit");

            if (unitPropRet >= 0)
            {
               if (sd_bus_message_enter_container(unitPropMessage, SD_BUS_TYPE_ARRAY, "{sv}") >= 0)
               {
                  const char *propName;
                  while (sd_bus_message_enter_container(unitPropMessage, SD_BUS_TYPE_DICT_ENTRY, "sv") > 0)
                  {
                     if (sd_bus_message_read(unitPropMessage, "s", &propName) >= 0)
                     {
                        if (strcmp(propName, "Description") == 0)
                        {
                           if (sd_bus_message_enter_container(unitPropMessage, SD_BUS_TYPE_VARIANT, "s") >= 0)
                           {
                              const char *desc;
                              if (sd_bus_message_read(unitPropMessage, "s", &desc) >= 0)
                                 strlcpy(info->description, desc, sizeof(info->description));
                              sd_bus_message_exit_container(unitPropMessage);
                           }
                        }
                        else if (strcmp(propName, "ActiveState") == 0)
                        {
                           if (sd_bus_message_enter_container(unitPropMessage, SD_BUS_TYPE_VARIANT, "s") >= 0)
                           {
                              const char *state;
                              if (sd_bus_message_read(unitPropMessage, "s", &state) >= 0)
                                 strlcpy(info->activeState, state, sizeof(info->activeState));
                              sd_bus_message_exit_container(unitPropMessage);
                           }
                        }
                        else if (strcmp(propName, "SubState") == 0)
                        {
                           if (sd_bus_message_enter_container(unitPropMessage, SD_BUS_TYPE_VARIANT, "s") >= 0)
                           {
                              const char *state;
                              if (sd_bus_message_read(unitPropMessage, "s", &state) >= 0)
                                 strlcpy(info->subState, state, sizeof(info->subState));
                              sd_bus_message_exit_container(unitPropMessage);
                           }
                        }
                        else if (strcmp(propName, "LoadState") == 0)
                        {
                           if (sd_bus_message_enter_container(unitPropMessage, SD_BUS_TYPE_VARIANT, "s") >= 0)
                           {
                              const char *state;
                              if (sd_bus_message_read(unitPropMessage, "s", &state) >= 0)
                                 strlcpy(info->loadState, state, sizeof(info->loadState));
                              sd_bus_message_exit_container(unitPropMessage);
                           }
                        }
                        else
                        {
                           sd_bus_message_skip(unitPropMessage, "v");
                        }
                     }
                     sd_bus_message_exit_container(unitPropMessage);
                  }
               }
            }
            sd_bus_error_free(&unitPropError);
            sd_bus_message_unref(unitPropMessage);

            // Get Service-specific properties (MainPID, ExecStart)
            sd_bus_message *serviceMessage = nullptr;
            sd_bus_error serviceError = SD_BUS_ERROR_NULL;
            int serviceRet = sd_bus_call_method(bus,
                                              "org.freedesktop.systemd1",
                                              actualUnitPath,
                                              "org.freedesktop.DBus.Properties",
                                              "GetAll",
                                              &serviceError,
                                              &serviceMessage,
                                              "s",
                                              "org.freedesktop.systemd1.Service");

            if (serviceRet >= 0)
            {
               if (sd_bus_message_enter_container(serviceMessage, SD_BUS_TYPE_ARRAY, "{sv}") >= 0)
               {
                  const char *propName;
                  while (sd_bus_message_enter_container(serviceMessage, SD_BUS_TYPE_DICT_ENTRY, "sv") > 0)
                  {
                     if (sd_bus_message_read(serviceMessage, "s", &propName) >= 0)
                     {
                        if (strcmp(propName, "MainPID") == 0)
                        {
                           if (sd_bus_message_enter_container(serviceMessage, SD_BUS_TYPE_VARIANT, "u") >= 0)
                           {
                              sd_bus_message_read(serviceMessage, "u", &info->mainPid);
                              sd_bus_message_exit_container(serviceMessage);
                           }
                        }
                        else if (strcmp(propName, "ExecStart") == 0)
                        {
                           if (sd_bus_message_enter_container(serviceMessage, SD_BUS_TYPE_VARIANT, "a(sasbttttuii)") >= 0)
                           {
                              if (sd_bus_message_enter_container(serviceMessage, SD_BUS_TYPE_ARRAY, "(sasbttttuii)") >= 0)
                              {
                                 // Try to read the first element of the ExecStart array
                                 if (sd_bus_message_enter_container(serviceMessage, SD_BUS_TYPE_STRUCT, "sasbttttuii") > 0)
                                 {
                                    const char *path;
                                    // Read the first field (executable path)
                                    if (sd_bus_message_read(serviceMessage, "s", &path) >= 0)
                                    {
                                       strlcpy(info->execMainStart, path, sizeof(info->execMainStart));
                                    }
                                    sd_bus_message_exit_container(serviceMessage); // Exit struct
                                 }
                                 sd_bus_message_exit_container(serviceMessage); // Exit array
                              }
                              sd_bus_message_exit_container(serviceMessage); // Exit variant
                           }
                        }
                        else
                        {
                           sd_bus_message_skip(serviceMessage, "v");
                        }
                     }
                     sd_bus_message_exit_container(serviceMessage);
                  }
               }
            }
            sd_bus_error_free(&serviceError);
            sd_bus_message_unref(serviceMessage);
         }
         else
         {
            // Service is not loaded - set default states and try basic unit file operations
            strlcpy(info->activeState, "inactive", sizeof(info->activeState));
            strlcpy(info->subState, "dead", sizeof(info->subState));
            strlcpy(info->loadState, "not-loaded", sizeof(info->loadState));

            // Extract service name without extension for operations
            char serviceNameWithoutExtension[256];
            strlcpy(serviceNameWithoutExtension, unitName, sizeof(serviceNameWithoutExtension));
            char *dot = strrchr(serviceNameWithoutExtension, '.');
            if (dot != nullptr && strcmp(dot, ".service") == 0)
               *dot = 0;

            // Try to get more accurate unit file state
            sd_bus_message *fileStateMessage = nullptr;
            sd_bus_error fileStateError = SD_BUS_ERROR_NULL;
            int fileStateRet = sd_bus_call_method(bus,
                                                "org.freedesktop.systemd1",
                                                "/org/freedesktop/systemd1",
                                                "org.freedesktop.systemd1.Manager",
                                                "GetUnitFileState",
                                                &fileStateError,
                                                &fileStateMessage,
                                                "s",
                                                serviceNameWithoutExtension);
            if (fileStateRet >= 0)
            {
               const char *state;
               if (sd_bus_message_read(fileStateMessage, "s", &state) >= 0)
               {
                  strlcpy(info->unitFileState, state, sizeof(info->unitFileState));
               }
            }
            sd_bus_error_free(&fileStateError);
            sd_bus_message_unref(fileStateMessage);

            // Try to load the unit temporarily to get description and ExecStart
            sd_bus_message *loadMessage = nullptr;
            sd_bus_error loadError = SD_BUS_ERROR_NULL;
            int loadRet = sd_bus_call_method(bus,
                                           "org.freedesktop.systemd1",
                                           "/org/freedesktop/systemd1",
                                           "org.freedesktop.systemd1.Manager",
                                           "LoadUnit",
                                           &loadError,
                                           &loadMessage,
                                           "s",
                                           unitName);
            
            if (loadRet >= 0)
            {
               const char *tempUnitPath;
               if (sd_bus_message_read(loadMessage, "o", &tempUnitPath) >= 0)
               {
                  // Get Description from the temporarily loaded unit
                  sd_bus_message *tempUnitPropMessage = nullptr;
                  sd_bus_error tempUnitPropError = SD_BUS_ERROR_NULL;
                  int tempUnitPropRet = sd_bus_call_method(bus,
                                                         "org.freedesktop.systemd1",
                                                         tempUnitPath,
                                                         "org.freedesktop.DBus.Properties",
                                                         "Get",
                                                         &tempUnitPropError,
                                                         &tempUnitPropMessage,
                                                         "ss",
                                                         "org.freedesktop.systemd1.Unit",
                                                         "Description");

                  if (tempUnitPropRet >= 0)
                  {
                     if (sd_bus_message_enter_container(tempUnitPropMessage, SD_BUS_TYPE_VARIANT, "s") >= 0)
                     {
                        const char *desc;
                        if (sd_bus_message_read(tempUnitPropMessage, "s", &desc) >= 0)
                           strlcpy(info->description, desc, sizeof(info->description));
                        sd_bus_message_exit_container(tempUnitPropMessage);
                     }
                  }
                  sd_bus_error_free(&tempUnitPropError);
                  sd_bus_message_unref(tempUnitPropMessage);

                  // Get ExecStart from the temporarily loaded unit
                  sd_bus_message *tempServiceMessage = nullptr;
                  sd_bus_error tempServiceError = SD_BUS_ERROR_NULL;
                  int tempServiceRet = sd_bus_call_method(bus,
                                                        "org.freedesktop.systemd1",
                                                        tempUnitPath,
                                                        "org.freedesktop.DBus.Properties",
                                                        "Get",
                                                        &tempServiceError,
                                                        &tempServiceMessage,
                                                        "ss",
                                                        "org.freedesktop.systemd1.Service",
                                                        "ExecStart");

                  if (tempServiceRet >= 0)
                  {
                     if (sd_bus_message_enter_container(tempServiceMessage, SD_BUS_TYPE_VARIANT, "a(sasbttttuii)") >= 0)
                     {
                        if (sd_bus_message_enter_container(tempServiceMessage, SD_BUS_TYPE_ARRAY, "(sasbttttuii)") >= 0)
                        {
                           if (sd_bus_message_enter_container(tempServiceMessage, SD_BUS_TYPE_STRUCT, "sasbttttuii") > 0)
                           {
                              const char *path;
                              if (sd_bus_message_read(tempServiceMessage, "s", &path) >= 0)
                              {
                                 strlcpy(info->execMainStart, path, sizeof(info->execMainStart));
                              }
                              sd_bus_message_exit_container(tempServiceMessage);
                           }
                           sd_bus_message_exit_container(tempServiceMessage);
                        }
                        sd_bus_message_exit_container(tempServiceMessage);
                     }
                  }
                  sd_bus_error_free(&tempServiceError);
                  sd_bus_message_unref(tempServiceMessage);
               }
            }
            sd_bus_error_free(&loadError);
            sd_bus_message_unref(loadMessage);

            // Set fallback description if still empty
            if (info->description[0] == '\0')
            {
               snprintf(info->description, sizeof(info->description), "systemd service: %s", serviceNameWithoutExtension);
            }
         }

         // Remove .service suffix from name for display
         char *dot = strrchr(info->name, '.');
         if (dot != nullptr && strcmp(dot, ".service") == 0)
            *dot = 0;

         services->add(info);
      }
   }

cleanup:
   sd_bus_error_free(&error);
   sd_bus_message_unref(message);
   sd_bus_unref(bus);

   return (ret >= 0);
}

/**
 * Handler for System.Services list
 */
LONG H_ServiceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   ObjectArray<ServiceInfo> services(128, 128, Ownership::True);
   if (!ListServicesViaDbus(&services))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_ServiceList: Failed to list services via D-Bus"));
      return SYSINFO_RC_ERROR;
   }

   // Use D-Bus results
   for (int i = 0; i < services.size(); i++)
   {
      ServiceInfo *info = services.get(i);
#ifdef UNICODE
      value->addMBString(info->name);
#else
      value->add(info->name);
#endif
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Services table
 */
LONG H_ServiceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   ObjectArray<ServiceInfo> services(128, 128, Ownership::True);
   if (!ListServicesViaDbus(&services))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_ServiceTable: Failed to list services via D-Bus"));
      return SYSINFO_RC_ERROR;
   }

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("DISPNAME"), DCI_DT_STRING, _T("Display name"));
   value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
   value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
   value->addColumn(_T("STARTUP"), DCI_DT_STRING, _T("Startup"));
   value->addColumn(_T("RUN_AS"), DCI_DT_STRING, _T("Run As"));
   value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"));
   value->addColumn(_T("BINARY"), DCI_DT_STRING, _T("Binary"));
   value->addColumn(_T("DEPENDENCIES"), DCI_DT_STRING, _T("Dependencies"));

   // Use D-Bus results
   for (int i = 0; i < services.size(); i++)
   {
      ServiceInfo *info = services.get(i);
      value->addRow();

      value->set(0, info->name); // Service name
      value->set(1, info->description); // Display name (use description)
      value->set(2, _T("Own")); // Type (always "Own" to mimic Windows services)

      // State (map systemd states to Windows-like states)
      const TCHAR *state = _T("Unknown");
      if (strcmp(info->activeState, "active") == 0)
      {
         if (strcmp(info->subState, "running") == 0)
            state = _T("Running");
         else if (strcmp(info->subState, "exited") == 0)
            state = _T("Stopped");
         else
            state = _T("Active");
      }
      else if (strcmp(info->activeState, "inactive") == 0)
      {
         state = _T("Stopped");
      }
      else if (strcmp(info->activeState, "failed") == 0)
      {
         state = _T("Failed");
      }
      else if (strcmp(info->activeState, "activating") == 0)
      {
         state = _T("Starting");
      }
      else if (strcmp(info->activeState, "deactivating") == 0)
      {
         state = _T("Stopping");
      }
      value->set(3, state);

      // Startup type (map systemd enabled states)
      const TCHAR *startup = _T("Unknown");
      if (strcmp(info->unitFileState, "enabled") == 0)
         startup = _T("Auto");
      else if (strcmp(info->unitFileState, "disabled") == 0)
         startup = _T("Manual");
      else if (strcmp(info->unitFileState, "static") == 0)
         startup = _T("Static");
      else if (strcmp(info->unitFileState, "masked") == 0)
         startup = _T("Disabled");
      else if (strcmp(info->unitFileState, "indirect") == 0)
         startup = _T("Indirect");
      value->set(4, startup);

      // Run As (systemd services typically run as root or specified user)
      value->set(5, _T("root")); // Default, could be enhanced to parse User= from unit file

      // PID
      if (info->mainPid != 0)
      {
         value->set(6, info->mainPid);
      }

      // Binary path
      if (info->execMainStart[0] != 0)
      {
         value->set(7, info->execMainStart);
      }

      // Dependencies (could be enhanced to parse actual dependencies)
      value->set(8, _T(""));
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ServiceState parameter
 */
LONG H_ServiceState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char serviceName[256];
   if (!AgentGetParameterArgA(cmd, 1, serviceName, sizeof(serviceName)))
      return SYSINFO_RC_UNSUPPORTED;

   // Add .service suffix if not present
   char fullServiceName[256];
   if (strstr(serviceName, ".service") == nullptr)
   {
      snprintf(fullServiceName, sizeof(fullServiceName), "%s.service", serviceName);
   }
   else
   {
      strlcpy(fullServiceName, serviceName, sizeof(fullServiceName));
   }

   ServiceInfo info;
   if (!GetServiceInfoViaDbus(fullServiceName, &info))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ServiceState: Failed to get service info for %hs"), serviceName);
      return SYSINFO_RC_ERROR;
   }

   int state = -1;  // Unknown by default

   // Map systemd states to numeric values:
   // 0=running, 2=starting, 5=stopping, 6=stopped, 7=failed
   if (strcmp(info.activeState, "active") == 0)
   {
      if (strcmp(info.subState, "running") == 0)
         state = 0; // running
      else if (strcmp(info.subState, "exited") == 0)
         state = 6; // stopped (service that ran and exited successfully)
      else
         state = 0; // consider other active substates as running
   }
   else if (strcmp(info.activeState, "inactive") == 0)
   {
      state = 6; // stopped
   }
   else if (strcmp(info.activeState, "failed") == 0)
   {
      state = 7; // failed
   }
   else if (strcmp(info.activeState, "activating") == 0)
   {
      state = 2; // starting
   }
   else if (strcmp(info.activeState, "deactivating") == 0)
   {
      state = 5; // stopping
   }
   else
   {
      // Unknown state, check load state
      if (!strcmp(info.loadState, "not-found"))
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ServiceState: Service %hs not found"), serviceName);
         return SYSINFO_RC_NO_SUCH_INSTANCE;
      }
      state = -1; // Unknown
   }

   ret_int(value, state);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for service control actions
 */
uint32_t H_ServiceControl(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   const TCHAR *serviceName = context->getArg(0);
   if (serviceName == nullptr)
      return ERR_BAD_ARGUMENTS;

   TCHAR action = *context->getData<TCHAR>();

   // Convert service name to char*
   char serviceNameA[256];
#ifdef UNICODE
   wchar_to_mb(serviceName, -1, serviceNameA, sizeof(serviceNameA));
#else
   strlcpy(serviceNameA, serviceName, sizeof(serviceNameA));
#endif

   // Add .service suffix if not present
   if (strstr(serviceNameA, ".service") == nullptr)
   {
      strlcat(serviceNameA, ".service", sizeof(serviceNameA));
   }

   // Determine D-Bus method based on action
   const char *method = nullptr;
   if (action == 'S')
   {
      method = "StartUnit";
   }
   else if (action == 'T')
   {
      method = "StopUnit";
   }
   else if (action == 'R')
   {
      method = "RestartUnit";
   }
   else if (action == 'L')
   {
      method = "ReloadUnit";
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_ServiceControl: Unsupported action '%c' for service '%s'"), action, serviceName);
      return ERR_INTERNAL_ERROR;
   }

   // Connect to system bus
   sd_bus *bus = nullptr;
   int ret = sd_bus_open_system(&bus);
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ServiceControl: Failed to connect to system bus (%hs)"), strerror(-ret));
      return ERR_INTERNAL_ERROR;
   }

   // Execute the service control command
   sd_bus_error error = SD_BUS_ERROR_NULL;
   sd_bus_message *message = nullptr;
   ret = sd_bus_call_method(bus,
                           "org.freedesktop.systemd1",
                           "/org/freedesktop/systemd1",
                           "org.freedesktop.systemd1.Manager",
                           method,
                           &error,
                           &message,
                           "ss",
                           serviceNameA,
                           "replace");  // Job mode: replace any existing job

   uint32_t result = ERR_SUCCESS;
   if (ret < 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ServiceControl: Failed to execute action %hs on service \"%s\" (%hs)"),
         method, serviceName, error.message);

      // Map common systemd errors to NetXMS error codes
      if (strstr(error.message, "not found") != nullptr)
      {
         result = ERR_BAD_ARGUMENTS;  // Service not found
      }
      else if (strstr(error.message, "access denied") != nullptr || strstr(error.message, "permission") != nullptr)
      {
         result = ERR_ACCESS_DENIED;
      }
      else
      {
         result = ERR_INTERNAL_ERROR;
      }
   }
   else
   {
      // Read the job path from response (for logging purposes)
      const char *jobPath;
      if (sd_bus_message_read(message, "o", &jobPath) >= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ServiceControl: Successfully initiated action %hs for service \"%s\", job path: %hs"),
            method, serviceName, jobPath);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ServiceControl: Successfully initiated action %hs for service \"%s\""), method, serviceName);
      }
   }

   // Cleanup
   sd_bus_error_free(&error);
   sd_bus_message_unref(message);
   sd_bus_unref(bus);

   return result;
}

#else /* HAVE_SDBUS */

/**
 * Handler for System.Services list
 */
LONG H_ServiceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("H_ServiceList: sd-bus support not compiled"));
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Handler for System.Services table
 */
LONG H_ServiceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("H_ServiceTable: sd-bus support not compiled"));
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Handler for System.ServiceState parameter
 */
LONG H_ServiceState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("H_ServiceState: sd-bus support not compiled"));
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Handler for service control actions
 */
uint32_t H_ServiceControl(const shared_ptr<ActionExecutionContext>& context)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("H_ServiceControl: sd-bus support not compiled"));
   return ERR_NOT_IMPLEMENTED;
}

#endif /* HAVE_SDBUS */
