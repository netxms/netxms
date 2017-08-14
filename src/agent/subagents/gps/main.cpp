/*
** NetXMS GPS receiver subagent
** Copyright (C) 2006-2016 Raden Solutions
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
** File: main.cpp
**
**/

#include <nms_agent.h>
#include <geolocation.h>
#include <math.h>
#include "nmea.h"

/**
 * Port name
 */
static TCHAR s_device[MAX_PATH];

/**
 * UERE (User Equivalent Range Error)
 */
static INT32 s_uere = 32;

/**
 * NMEA info
 */
static nmeaINFO s_nmeaInfo;

/**
 * Parsed geolocation object
 */
static GeoLocation s_geolocation;

/**
 * Lock for NMEA info
 */
static MUTEX s_nmeaInfoLock = MutexCreate();

/**
 * Serial port
 */
static Serial s_serial;

/**
 * Initialize serial port
 *
 * s_device format: portname,speed,databits,parity,stopbits
 */
static bool InitSerialPort()
{
   bool success = false;
   TCHAR *portName;

   if (s_device[0] == 0)
   {
#ifdef _WIN32
      portName = _tcsdup(_T("COM1:"));
#else
      portName = _tcsdup(_T("/dev/ttyS0"));
#endif
   }
   else
   {
      portName = _tcsdup(s_device);
   }

   AgentWriteDebugLog(1, _T("GPS: using serial port configuration \"%s\""), portName);

   TCHAR *p;
   const TCHAR *parityAsText;
   int portSpeed = 4800;
   int dataBits = 8;
   int parity = NOPARITY;
   int stopBits = ONESTOPBIT;

   if ((p = _tcschr(portName, _T(','))) != NULL)
   {
      *p = 0; p++;
      int tmp = _tcstol(p, NULL, 10);
      if (tmp != 0)
      {
         portSpeed = tmp;

         if ((p = _tcschr(p, _T(','))) != NULL)
         {
            *p = 0; p++;
            tmp = _tcstol(p, NULL, 10);
            if (tmp >= 5 && tmp <= 8)
            {
               dataBits = tmp;

               // parity
               if ((p = _tcschr(p, _T(','))) != NULL)
               {
                  *p = 0; p++;
                  switch (tolower((char)*p))
                  {
                     case 'n': // none
                        parity = NOPARITY;
                        break;
                     case 'o': // odd
                        parity = ODDPARITY;
                        break;
                     case 'e': // even
                        parity = EVENPARITY;
                        break;
                  }

                  // stop bits
                  if ((p = _tcschr(p, _T(','))) != NULL)
                  {
                     *p = 0; p++;

                     if (*p == _T('2'))
                     {
                        stopBits = TWOSTOPBITS;
                     }
                  }
               }
            }
         }
      }
   }

   switch (parity)
   {
      case ODDPARITY:
         parityAsText = _T("ODD");
         break;
      case EVENPARITY:
         parityAsText = _T("EVEN");
         break;
      default:
         parityAsText = _T("NONE");
         break;
   }
   AgentWriteDebugLog(1, _T("GPS: initialize for port=\"%s\", speed=%d, data=%d, parity=%s, stop=%d"),
                      portName, portSpeed, dataBits, parityAsText, stopBits == TWOSTOPBITS ? 2 : 1);

   if (s_serial.open(portName))
   {
      AgentWriteDebugLog(5, _T("GPS: port opened"));
      s_serial.setTimeout(2000);

      if (s_serial.set(portSpeed, dataBits, parity, stopBits))
      {
         success = true;
      }
      else
      {
         AgentWriteDebugLog(5, _T("GPS: cannot set port parameters"));
      }

      AgentWriteLog(NXLOG_INFO, _T("GPS: serial port initialized"));
   }
   else
   {
      AgentWriteLog(NXLOG_WARNING, _T("GPS: Unable to open serial port"));
   }

   free(portName);
   return success;
}

/**
 * Convert NMEA NDEG field ([dd][mm].[s/60]) to degrees
 */
inline double NMEA_TO_DEG(double nm)
{
   double n = fabs(nm);
   int d = (int)(n / 100.0);
   int m = (int)(n - d * 100);
   double s = n - (d * 100 + m);
   double r = d + m / 60.0 + s / 60.0;
   return (nm < 0) ? -r : r;
}

/**
 * Poller thread
 */
static THREAD_RESULT THREAD_CALL PollerThread(void *arg)
{
   AgentWriteDebugLog(3, _T("GPS: poller thread started"));

   nmeaPARSER parser;
   nmea_zero_INFO(&s_nmeaInfo);
   nmea_parser_init(&parser);

   while(!AgentSleepAndCheckForShutdown(30))
   {
      if (!s_serial.restart())
      {
         AgentWriteDebugLog(7, _T("GPS: cannot open serial port"));
         continue;
      }

      while(!AgentSleepAndCheckForShutdown(0))
      {
         static const char *marks[] = { "\r\n", NULL };
         char *occ, buffer[128];
         if (s_serial.readToMark(buffer, 128, marks, &occ) <= 0)
         {
            AgentWriteDebugLog(8, _T("GPS: serial port read failure"));
            break;
         }

         if (occ != NULL)
         {
            MutexLock(s_nmeaInfoLock);
            int count = nmea_parse(&parser, buffer, (int)strlen(buffer), &s_nmeaInfo);
            if (count > 0)
            {
               s_geolocation = GeoLocation(GL_GPS, NMEA_TO_DEG(s_nmeaInfo.lat), NMEA_TO_DEG(s_nmeaInfo.lon),
                                           (int)(s_nmeaInfo.HDOP * s_uere), time(NULL));
            }
            MutexUnlock(s_nmeaInfoLock);
         }
      }
   }

   nmea_parser_destroy(&parser);
   AgentWriteDebugLog(3, _T("GPS: poller thread stopped"));
   return THREAD_OK;
}

/**
 * Poller thread handle
 */
static THREAD s_pollerThread = INVALID_THREAD_HANDLE;

/**
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
	// Parse configuration
   s_uere = config->getValueAsInt(_T("/GPS/UERE"), s_uere);
	const TCHAR *value = config->getValue(_T("/GPS/Device"));
	if (value != NULL)
	{
		nx_strncpy(s_device, value, MAX_PATH);
		InitSerialPort();
		s_pollerThread = ThreadCreateEx(PollerThread, 0, NULL);
	}
	else
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("GPS: device not specified"));
	}

	return value != NULL;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   ThreadJoin(s_pollerThread);
}

/**
 * Handler for GPS.SerialConfig
 */
static LONG H_Config(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_string(value, s_device);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for location information parameters
 */
static LONG H_LocationInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_nmeaInfoLock);
   switch(*arg)
   {
      case 'A': // latitude as text
         ret_string(value, s_geolocation.getLatitudeAsString());
         break;
      case 'a': // latitude
         ret_double(value, s_geolocation.getLatitude());
         break;
      case 'D': // direction
         ret_double(value, s_nmeaInfo.direction);
         break;
      case 'E': // elevation
         ret_double(value, s_nmeaInfo.elv);
         break;
      case 'F': // Fix
         ret_int(value, s_nmeaInfo.fix);
         break;
      case 'H': // HDOP
         ret_double(value, s_nmeaInfo.HDOP);
         break;
      case 'L': // location as text
         _sntprintf(value, MAX_RESULT_LENGTH, _T("%s %s"), s_geolocation.getLatitudeAsString(), s_geolocation.getLongitudeAsString());
         break;
      case 'l': // full location data
         _sntprintf(value, MAX_RESULT_LENGTH, _T("%d,%d,%f,%f,%d,%f,%f,%f,%f,") INT64_FMT,
                    s_nmeaInfo.sig, s_nmeaInfo.fix, s_geolocation.getLatitude(), s_geolocation.getLongitude(),
                    s_geolocation.getAccuracy(), s_nmeaInfo.elv, s_nmeaInfo.speed, s_nmeaInfo.direction,
                    s_nmeaInfo.HDOP, (INT64)s_geolocation.getTimestamp());
         break;
      case 'O': // longitude as text
         ret_string(value, s_geolocation.getLongitudeAsString());
         break;
      case 'o': // longitude
         ret_double(value, s_geolocation.getLongitude());
         break;
      case 'Q': // quality indicator
         ret_int(value, s_nmeaInfo.sig);
         break;
      case 's': // number of satellites in view
         ret_int(value, s_nmeaInfo.satinfo.inview);
         break;
      case 'S': // number of satellites in use
         ret_int(value, s_nmeaInfo.satinfo.inuse);
         break;
      case 'V': // VDOP
         ret_double(value, s_nmeaInfo.VDOP);
         break;
      case 'X': // speed
         ret_double(value, s_nmeaInfo.speed);
         break;
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   MutexUnlock(s_nmeaInfoLock);
   return rc;
}

/**
 * Subagent parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("GPS.Direction"), H_LocationInfo, _T("D"), DCI_DT_FLOAT, _T("GPS: direction") },
   { _T("GPS.Elevation"), H_LocationInfo, _T("E"), DCI_DT_FLOAT, _T("GPS: elevation") },
   { _T("GPS.Fix"), H_LocationInfo, _T("F"), DCI_DT_INT, _T("GPS: fix (operating mode)") },
   { _T("GPS.HDOP"), H_LocationInfo, _T("H"), DCI_DT_FLOAT, _T("GPS: horizontal delusion of precision") },
	{ _T("GPS.Latitude"), H_LocationInfo, _T("a"), DCI_DT_FLOAT, _T("GPS: latitude") },
   { _T("GPS.LatitudeText"), H_LocationInfo, _T("A"), DCI_DT_STRING, _T("GPS: latitude (as text)") },
   { _T("GPS.Location"), H_LocationInfo, _T("L"), DCI_DT_STRING, _T("GPS: location") },
   { _T("GPS.LocationData"), H_LocationInfo, _T("l"), DCI_DT_STRING, _T("GPS: full location data") },
   { _T("GPS.Longitude"), H_LocationInfo, _T("o"), DCI_DT_FLOAT, _T("GPS: longitude") },
   { _T("GPS.LongitudeText"), H_LocationInfo, _T("O"), DCI_DT_STRING, _T("GPS: longitude (as text)") },
   { _T("GPS.QualityIndicator"), H_LocationInfo, _T("Q"), DCI_DT_INT, _T("GPS: position quality indicator") },
   { _T("GPS.Satellites.InUse"), H_LocationInfo, _T("S"), DCI_DT_INT, _T("GPS: satellites in use") },
   { _T("GPS.Satellites.InView"), H_LocationInfo, _T("s"), DCI_DT_INT, _T("GPS: satellites in view") },
   { _T("GPS.Speed"), H_LocationInfo, _T("X"), DCI_DT_FLOAT, _T("GPS: ground speed") },
	{ _T("GPS.SerialConfig"), H_Config, NULL, DCI_DT_STRING, _T("GPS: serial port configuration") },
   { _T("GPS.VDOP"), H_LocationInfo, _T("V"), DCI_DT_FLOAT, _T("GPS: vertical delusion of precision") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("GPS"), NETXMS_BUILD_TAG,
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, NULL,	// lists
	0, NULL,	// tables
   0, NULL, // actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(GPS)
{
	*ppInfo = &m_info;
	return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
