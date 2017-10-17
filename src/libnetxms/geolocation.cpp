/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: geolocation.cpp
**
**/

#include "libnetxms.h"
#include <geolocation.h>
#include <math.h>

static const double ROUND_OFF = 0.00000001;

#ifdef UNICODE
#define DEGREE_SIGN_CHR L'\x00B0'
#define DEGREE_SIGN_STR L"\x00B0"
#else
#define DEGREE_SIGN_CHR '\xF8'
#define DEGREE_SIGN_STR "\xF8"
#endif

/**
 * Default constructor - create location of type UNSET
 */
GeoLocation::GeoLocation()
{
	m_type = GL_UNSET;
	m_lat = 0;
	m_lon = 0;
	posToString(true, 0);
	posToString(false, 0);
	m_isValid = true;
	m_accuracy = 0;
	m_timestamp = 0;
}

/**
 * Constructor - create location from double lat/lon values
 */
GeoLocation::GeoLocation(int type, double lat, double lon, int accuracy, time_t timestamp)
{
	m_type = type;
	m_lat = lat;
	m_lon = lon;
	posToString(true, lat);
	posToString(false, lon);
	m_isValid = true;
   m_accuracy = accuracy;
	m_timestamp = timestamp;
}

/**
 * Constructor - create location from string lat/lon values
 */
GeoLocation::GeoLocation(int type, const TCHAR *lat, const TCHAR *lon, int accuracy, time_t timestamp)
{
	m_type = type;
	m_isValid = parseLatitude(lat) && parseLongitude(lon);
	posToString(true, m_lat);
	posToString(false, m_lon);
   m_accuracy = accuracy;
	m_timestamp = timestamp;
}

/**
 * Copy constructor
 */
GeoLocation::GeoLocation(const GeoLocation &src)
{
	m_type = src.m_type;
	m_lat = src.m_lat;
	m_lon = src.m_lon;
	_tcslcpy(m_latStr, src.m_latStr, 20);
	_tcslcpy(m_lonStr, src.m_lonStr, 20);
	m_isValid = src.m_isValid;
   m_accuracy = src.m_accuracy;
	m_timestamp = src.m_timestamp;
}

/**
 * Create geolocation object from data in NXCP message
 */
GeoLocation::GeoLocation(NXCPMessage &msg)
{
	m_type = (int)msg.getFieldAsUInt16(VID_GEOLOCATION_TYPE);

   if (msg.getFieldType(VID_LATITUDE) == NXCP_DT_INT32)
	   m_lat = (double)msg.getFieldAsInt32(VID_LATITUDE) / 1000000;
   else
	   m_lat = msg.getFieldAsDouble(VID_LATITUDE);

   if (msg.getFieldType(VID_LONGITUDE) == NXCP_DT_INT32)
	   m_lon = (double)msg.getFieldAsInt32(VID_LONGITUDE) / 1000000;
   else
   	m_lon = msg.getFieldAsDouble(VID_LONGITUDE);

	m_accuracy = (int)msg.getFieldAsUInt16(VID_ACCURACY);

   m_timestamp = 0;
   int ft = msg.getFieldType(VID_GEOLOCATION_TIMESTAMP);
   if (ft == NXCP_DT_INT64)
   {
      m_timestamp = (time_t)msg.getFieldAsUInt64(VID_GEOLOCATION_TIMESTAMP);
   }
   else if (ft == NXCP_DT_INT32)
   {
      m_timestamp = (time_t)msg.getFieldAsUInt32(VID_GEOLOCATION_TIMESTAMP);
   }
   else if (ft == NXCP_DT_STRING)
   {
      char ts[256];
      msg.getFieldAsMBString(VID_GEOLOCATION_TIMESTAMP, ts, 256);

      struct tm timeBuff;
      if (strptime(ts, "%Y/%m/%d %H:%M:%S", &timeBuff) != NULL)
      {
         timeBuff.tm_isdst = -1;
         m_timestamp = timegm(&timeBuff);
      }
   }
   if(m_timestamp == 0)
      m_timestamp = time(0);

	posToString(true, m_lat);
	posToString(false, m_lon);
	m_isValid = true;
}

/**
 * Destructor
 */
GeoLocation::~GeoLocation()
{
}

/**
 * Assignment operator
 */
GeoLocation& GeoLocation::operator =(const GeoLocation &src)
{
	m_type = src.m_type;
	m_lat = src.m_lat;
	m_lon = src.m_lon;
	_tcslcpy(m_latStr, src.m_latStr, 20);
	_tcslcpy(m_lonStr, src.m_lonStr, 20);
	m_isValid = src.m_isValid;
	m_accuracy = src.m_accuracy;
	m_timestamp = src.m_timestamp;
	return *this;
}

/**
 * Fill NXCP message
 */
void GeoLocation::fillMessage(NXCPMessage &msg) const
{
	msg.setField(VID_GEOLOCATION_TYPE, (UINT16)m_type);
	msg.setField(VID_LATITUDE, m_lat);
	msg.setField(VID_LONGITUDE, m_lon);
	msg.setField(VID_ACCURACY, (UINT16)m_accuracy);
	msg.setField(VID_GEOLOCATION_TIMESTAMP, (UINT64)m_timestamp);
}

/**
 * Convert to JSON
 */
json_t *GeoLocation::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "type", json_integer(m_type));
   json_object_set_new(root, "latitude", json_real(m_lat));
   json_object_set_new(root, "longitude", json_real(m_lon));
   json_object_set_new(root, "accuracy", json_integer(m_accuracy));
   json_object_set_new(root, "timestamp", json_integer(m_timestamp));
   return root;
}

/**
 * Getters degree from double value
 */
int GeoLocation::getIntegerDegree(double pos)
{
	return (int)(fabs(pos) + ROUND_OFF);
}

/**
 * Getters minutes from double value
 */
int GeoLocation::getIntegerMinutes(double pos)
{
	double d = fabs(pos) + ROUND_OFF;
	return (int)((d - (double)((int)d)) * 60.0);
}

/**
 * Getters seconds from double value
 */
double GeoLocation::getDecimalSeconds(double pos)
{
	double d = fabs(pos) * 60.0 + ROUND_OFF;
	return (d - (double)((int)d)) * 60.0;
}

/**
 * Convert position to string
 */
void GeoLocation::posToString(bool isLat, double pos)
{
	TCHAR *buffer = isLat ? m_latStr : m_lonStr;

	// Check range
	if ((pos < -180.0) || (pos > 180.0))
	{
		_tcscpy(buffer, _T("<invalid>"));
		return;
	}

	// Encode hemisphere
	if (isLat)
	{
		*buffer = (pos < 0) ? _T('S') : _T('N');
	}
	else
	{
		*buffer = (pos < 0) ? _T('W') : _T('E');
	}
	buffer++;
	*buffer++ = _T(' ');

	_sntprintf(buffer, 18, _T("%02d") DEGREE_SIGN_STR _T(" %02d' %06.3f\""), getIntegerDegree(pos), getIntegerMinutes(pos), getDecimalSeconds(pos));
}

/**
 * Parse latitude/longitude string
 */
double GeoLocation::parse(const TCHAR *str, bool isLat, bool *isValid)
{
	*isValid = false;

	// Prepare input string
	TCHAR *in = _tcsdup(str);
	StrStrip(in);

	// Check if string given is just double value
	TCHAR *eptr;
	double value = _tcstod(in, &eptr);
	if (*eptr == 0)
	{
		*isValid = true;
	}
	else   // If not a valid double, check if it's in DMS form
	{
		// Check for invalid characters
		if (_tcsspn(in, isLat ? _T("0123456789.,'\" NS") DEGREE_SIGN_STR : _T("0123456789.,'\" EW") DEGREE_SIGN_STR) == _tcslen(in))
		{
         TranslateStr(in, _T(","), _T("."));
			TCHAR *curr = in;

			int sign = 0;
			if ((*curr == _T('N')) || (*curr == _T('E')))
			{
				sign = 1;
				curr++;
			}
			else if ((*curr == _T('S')) || (*curr == _T('W')))
			{
				sign = -1;
				curr++;
			}

			while(*curr == _T(' '))
				curr++;

			double deg = 0.0, min = 0.0, sec = 0.0;

			deg = _tcstod(curr, &eptr);
			if (*eptr == 0)	// End of string
				goto finish_parsing;
			if ((*eptr != DEGREE_SIGN_CHR) && (*eptr != _T(' ')))
				goto cleanup;	// Unexpected character
			curr = eptr + 1;
			while(*curr == _T(' '))
				curr++;

			min = _tcstod(curr, &eptr);
			if (*eptr == 0)	// End of string
				goto finish_parsing;
			if (*eptr != _T('\''))
				goto cleanup;	// Unexpected character
			curr = eptr + 1;
			while(*curr == _T(' '))
				curr++;

			sec = _tcstod(curr, &eptr);
			if (*eptr == 0)	// End of string
				goto finish_parsing;
			if (*eptr != _T('"'))
				goto cleanup;	// Unexpected character
			curr = eptr + 1;
			while(*curr == _T(' '))
				curr++;

			if ((*curr == _T('N')) || (*curr == _T('E')))
			{
				sign = 1;
				curr++;
			}
			else if ((*curr == _T('S')) || (*curr == _T('W')))
			{
				sign = -1;
				curr++;
			}

			if (sign == 0)
				goto cleanup;	// Hemisphere was not specified

finish_parsing:
			value = sign * (deg + min / 60.0 + sec / 3600.0);
			*isValid = true;
		}
	}

cleanup:
	free(in);
	return value;
}

/**
 * Parse latitude
 */
bool GeoLocation::parseLatitude(const TCHAR *lat)
{
	bool isValid;

	m_lat = parse(lat, true, &isValid);
	if (!isValid)
		m_lat = 0.0;
	return isValid;
}

/**
 * Parse longitude
 */
bool GeoLocation::parseLongitude(const TCHAR *lon)
{
	bool isValid;

	m_lon = parse(lon, false, &isValid);
	if (!isValid)
		m_lon = 0.0;
	return isValid;
}

/**
 * Check if two geolocation objects are equal
 */
bool GeoLocation::equals(const GeoLocation &location) const
{
   return (location.m_accuracy == m_accuracy) &&
            (location.m_lat == m_lat) &&
            (location.m_lon == m_lon) &&
            (location.m_type == m_type);
}

/**
 * Convert degrees to radians
 */
#define DegreesToRadians(a) ((a) * 3.14159265 / 180.0)

/**
 * Check if this locations is (almost) same as given location
 */
bool GeoLocation::sameLocation(double lat, double lon, int oldAccuracy) const
{
   const double R = 6371000; // Earth radius in meters

   double f1 = DegreesToRadians(lat);
   double f2 = DegreesToRadians(m_lat);
   double df = DegreesToRadians(m_lat - lat);
   double dl = DegreesToRadians(m_lon - lon);

   double a = pow(sin(df / 2), 2) + cos(f1) * cos(f2) * pow(sin(dl / 2), 2);
   double c = 2 * atan2(sqrt(a), sqrt(1 - a));

   double distance = R * c;
   return distance <= std::min(oldAccuracy, m_accuracy);
}

/**
 * Parse data sent by agent (signal,fix,lat,lon,accuracy,elevation,speed,direction,HDOP,time)
 */
GeoLocation GeoLocation::parseAgentData(const TCHAR *data)
{
   double lat, lon;
   int acc, signal, fix;
   time_t timestamp;

   TCHAR buffer[256];
   _tcslcpy(buffer, data, 256);

   int pos = 0;
   TCHAR *curr = buffer;
   TCHAR *next;
   do
   {
      next = _tcschr(curr, _T(','));
      if (next != NULL)
         *next = 0;
      switch(pos)
      {
         case 0:
            signal = _tcstol(curr, NULL, 10);
            break;
         case 1:
            fix = _tcstol(curr, NULL, 10);
            break;
         case 2:
            lat = _tcstod(curr, NULL);
            break;
         case 3:
            lon = _tcstod(curr, NULL);
            break;
         case 4:
            acc = _tcstol(curr, NULL, 10);
            break;
         case 9:
            timestamp = (time_t)_tcstoll(curr, NULL, 10);
            break;
         default: // ignore the rest
            break;
      }
      pos++;
      curr = next + 1;
   }
   while(next != NULL);

   if ((pos < 10) || (signal == 0) || (fix == 0))
      return GeoLocation();   // parsing error or location is unknown
   return GeoLocation(GL_GPS, lat, lon, acc, timestamp);
}

int GeoLocation::calculateDistance(GeoLocation &location) const
{
   const double R = 6371000; // Earth radius in meters

   double f1 = DegreesToRadians(location.m_lat);
   double f2 = DegreesToRadians(m_lat);
   double df = DegreesToRadians(m_lat - location.m_lat);
   double dl = DegreesToRadians(m_lon - location.m_lon);

   double a = pow(sin(df / 2), 2) + cos(f1) * cos(f2) * pow(sin(dl / 2), 2);
   double c = 2 * atan2(sqrt(a), sqrt(1 - a));

   double distance = R * c;
   return (int)(distance+0.5);
}
