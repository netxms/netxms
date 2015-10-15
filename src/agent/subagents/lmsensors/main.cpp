/* ** NetXMS PING subagent
** Copyright (C) 2012 Alex Kirhenshtein
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

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sensors/sensors.h>
#include <sensors/error.h>


//
// Static data
//

static bool m_temperatureInFahrenheit = false;
static TCHAR m_configFileName[256] = _T("");

//
// Support methods
//
static double celsiusToFahrenheit(double celsius) {
   return celsius * (9.0F / 5.0F) + 32.0F;
}

//
// lm_sensors interface
//

static bool getSensorValue(char *chipName, char *featureName, double *result) {
   bool ret = false;
   sensors_chip_name match;

   if (sensors_parse_chip_name(chipName, &match) == 0) {
      const sensors_chip_name *chip;
      int chipNumber = 0;

      while ((chip = sensors_get_detected_chips(&match, &chipNumber)) != NULL) {
         const sensors_feature *feature;
         int featureNumber = 0;

         while ((feature = sensors_get_features(chip, &featureNumber)) != NULL) {
            char *label = sensors_get_label(chip, feature);
            if (label == NULL) {
               continue;
            }

            if (strcmp(featureName, label)) {
               free(label);
               continue;
            }
            free(label);

            sensors_subfeature_type types[2];
            types[1] = SENSORS_SUBFEATURE_UNKNOWN;
            switch(feature->type)
            {
               case SENSORS_FEATURE_TEMP:
                  types[0] = SENSORS_SUBFEATURE_TEMP_INPUT; // Celsius
                  break;
               case SENSORS_FEATURE_IN:
                  types[0] = SENSORS_SUBFEATURE_IN_INPUT; // Volts
                  break;
               case SENSORS_FEATURE_FAN:
                  types[0] = SENSORS_SUBFEATURE_FAN_INPUT; // RPM
                  break;
               case SENSORS_FEATURE_VID:
                  types[0] = SENSORS_SUBFEATURE_VID; // Volts
                  break;
               case SENSORS_FEATURE_POWER:
                  /* From sensors example:
                   *   Power sensors come in 2 flavors: instantaneous and averaged.
                   *   To keep things simple, we assume that each sensor only implements
                   *   one flavor.
                   */
                  types[0] = SENSORS_SUBFEATURE_POWER_INPUT; // Watts
                  types[1] = SENSORS_SUBFEATURE_POWER_AVERAGE; // Watts
                  break;
               case SENSORS_FEATURE_ENERGY:
                  types[0] = SENSORS_SUBFEATURE_ENERGY_INPUT; // Joules
                  break;
               case SENSORS_FEATURE_CURR:
                  types[0] = SENSORS_SUBFEATURE_CURR_INPUT; // Amps
                  break;
               default:
                  types[0] = SENSORS_SUBFEATURE_UNKNOWN;
                  break;
            }

            const sensors_subfeature *subFeature = sensors_get_subfeature(chip, feature, types[0]);
            if (subFeature == NULL && types[1] != SENSORS_SUBFEATURE_UNKNOWN) {
               subFeature = sensors_get_subfeature(chip, feature, types[1]);
            }
            if (subFeature != NULL) {
               double val;
               if (sensors_get_value(chip, subFeature->number, &val) == 0) {
                  ret = true;
                  if (feature->type == SENSORS_FEATURE_TEMP && m_temperatureInFahrenheit) {
                     *result = celsiusToFahrenheit(val);
                  }
                  else {
                     *result = val;
                  }
               }
            }
         }
      }

      sensors_free_chip_name(&match);
   }

   return ret;
}


//
// Hanlder for immediate ping request
//

static LONG H_GetValue(const TCHAR *parameters, const TCHAR *arg, TCHAR *value, AbstractCommSession *session) {
   int ret = SYSINFO_RC_ERROR;

   char chipName[256];
   char featureName[256];

   if (AgentGetParameterArgA(parameters, 1, chipName, 256) && AgentGetParameterArgA(parameters, 2, featureName, 256)) {
      StrStripA(chipName);
      StrStripA(featureName);

      double result;
      if (getSensorValue(chipName, featureName, &result)) {
         ret_double(value, result);
         ret = SYSINFO_RC_SUCCESS;
      }
   }

   return ret;
}


//
// Configuration file template
//

static NX_CFG_TEMPLATE m_cfgTemplate[] = {
   { _T("UseFahrenheit"), CT_BOOLEAN, 0, 0, 1, 0, &m_temperatureInFahrenheit },
   { _T("ConfigFile"), CT_STRING, 0, 0, 256, 0, &m_configFileName },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Subagent initialization
//

static BOOL SubagentInit(Config *config) {
   bool ret;

   ret = config->parseTemplate(_T("lmsensors"), m_cfgTemplate);
   if (ret) {
      FILE *configFile = NULL;
      if (m_configFileName[0] != 0) {
         configFile = _tfopen(m_configFileName, _T("rb"));

         if (configFile == NULL) {
            AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("Unable to open lm_sensors config file \"%s\""), m_configFileName);
            ret = false;
         }
      }

      if (ret) {
         int err = sensors_init(configFile);
         if (err != 0) {
            AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("sensors_init() failed: %hs"), sensors_strerror(err));
            ret = false;
         }
      }

      if (configFile != NULL) {
         fclose(configFile);
      }
   }

   return ret;
}

//
//
// Called by master agent at unload
//

static void SubagentShutdown()
{
   sensors_cleanup();
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] = 
{
   { _T("LMSensors.Value(*)"), H_GetValue, NULL, DCI_DT_FLOAT, _T("Sensor {instance}") },
};

static NETXMS_SUBAGENT_INFO m_info = 
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("LMSENSORS"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, NULL,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   0, NULL, // enums
   0, NULL,	// tables
   0, NULL,	// actions
   0, NULL	// push parameters
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(LMSENSORS)
{
   *ppInfo = &m_info;
   return TRUE;
}
