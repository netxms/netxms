/*
 ** LoraWAN subagent
 ** Copyright (C) 2009 - 2017 Raden Solutions
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

#include "lorawan.h"

/**
 * MQTT Mesage handler
 */
void MqttMessageHandler(const char *payload, char *topic)
{
   json_error_t error;
   json_t *root = json_loads(payload, 0, &error);

   json_t *tmp = json_object_get(root, "appargs");
   if (json_is_string(tmp))
   {
      TCHAR buffer[64];
#ifdef UNICODE
      MultiByteToWideChar(CP_UTF8, 0, json_string_value(tmp), -1, buffer, 64);
#else
      nx_strncpy(buffer, json_string_value(tmp), 64);
#endif
      LoraDeviceData *data = FindDevice(uuid::parse(buffer));

      if (data != NULL)
      {
         MacAddress rxDevAddr;
         MacAddress rxDevEui;

         tmp = json_object_get(root, "deveui");
         if (json_is_string(tmp))
            rxDevEui = MacAddress::parse(json_string_value(tmp));
         tmp = json_object_get(root, "devaddr");
         if (json_is_string(tmp))
            rxDevAddr = MacAddress::parse(json_string_value(tmp));

         if ((data->getDevAddr().length() != 0 && data->getDevAddr().equals(rxDevAddr))
             || (data->getDevEui().length() != 0 && data->getDevEui().equals(rxDevEui)))
         {
            if (data->getDevAddr().length() == 0 && rxDevAddr.length() != 0)
               data->setDevAddr(rxDevAddr);

            tmp = json_object_get(root, "data");
            if (json_is_string(tmp))
               data->setPayload(json_string_value(tmp));

            tmp = json_object_get(root, "fcnt");
            if (json_is_integer(tmp))
               data->setFcnt(json_integer_value(tmp));

            tmp = json_object_get(root, "port");
            if (json_is_integer(tmp))
               data->setPort(json_integer_value(tmp));

            tmp = json_object_get(root, "rxq");
            if (json_is_object(tmp))
            {
               json_t *tmp2 = json_object_get(tmp, "datr");
               if (json_is_string(tmp2))
                  data->setDataRate(json_string_value(tmp2));

               tmp2 = json_object_get(tmp, "freq");
               if (json_is_real(tmp2))
                  data->setFreq(json_real_value(tmp2));

               tmp2 = json_object_get(tmp, "lsnr");
               if (json_is_real(tmp2))
                  data->setSnr(json_real_value(tmp2));

               tmp2 = json_object_get(tmp, "rssi");
               if (json_is_integer(tmp2))
                  data->setRssi(json_integer_value(tmp2));

               free(tmp2);
            }
            else
               nxlog_debug(6, _T("LoraWAN Module:[MqttMessageHandler] No RX quality data received..."));

            NXMBDispatcher *dispatcher = NXMBDispatcher::getInstance();
            if (!dispatcher->call(_T("NOTIFY_DECODERS"), data, NULL))
               nxlog_debug(6, _T("LoraWAN Module:[MqttMessageHandler] Call to NXMBDispacher failed..."));

            data->updateLastContact();
         }
         else
            nxlog_debug(6, _T("LoraWAN Module:[MqttMessageHandler] Neither the devAddr nor the devEUI returned a match..."));
      }
   }
   else
      nxlog_debug(6, _T("LoraWAN Module:[MqttMessageHandler] No GUID found due to missing \"appargs\" field in received JSON"));

   free(root);
   free(tmp);
}

/**
 * Handler for communication parameters
 */
LONG H_Communication(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR guid[38];
   if (!AgentGetParameterArg(param, 1, guid, 38))
      return SYSINFO_RC_ERROR;

   LoraDeviceData *data = FindDevice(uuid::parse(guid));
   if (data == NULL)
      return SYSINFO_RC_ERROR;

   switch(*arg)
   {
      case 'A':
         ret_string(value, data->getDevAddr().toString(MAC_ADDR_FLAT_STRING));
         break;
      case 'C':
         ret_int(value, data->getLastContact());
         break;
      case 'D':
         ret_mbstring(value, data->getDataRate());
         break;
      case 'F':
         ret_double(value, data->getFreq(), 1);
         break;
      case 'M':
         ret_uint(value, data->getFcnt());
         break;
      case 'R':
         ret_int(value, data->getRssi());
         break;
      case 'S':
         ret_double(value, data->getSnr(), 1);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}
