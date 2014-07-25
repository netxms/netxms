/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
	nx_strncpy(m_latStr, src.m_latStr, 20);
	nx_strncpy(m_lonStr, src.m_lonStr, 20);
	m_isValid = src.m_isValid;
   m_accuracy = src.m_accuracy;
	m_timestamp = src.m_timestamp;
}

/**
 * Create geolocation object from data in NXCP message
 */
GeoLocation::GeoLocation(CSCPMessage &msg)
{
	m_type = (int)msg.GetVariableShort(VID_GEOLOCATION_TYPE);

   if (msg.getFieldType(VID_LATITUDE) == CSCP_DT_INTEGER)
	   m_lat = (double)msg.getFieldAsInt32(VID_LATITUDE) / 1000000;
   else
	   m_lat = msg.getFieldAsDouble(VID_LATITUDE);

   if (msg.getFieldType(VID_LONGITUDE) == CSCP_DT_INTEGER)
	   m_lon = (double)msg.getFieldAsInt32(VID_LONGITUDE) / 1000000;
   else
   	m_lon = msg.getFieldAsDouble(VID_LONGITUDE);

	m_accuracy = (int)msg.GetVariableShort(VID_ACCURACY);

   m_timestamp = 0;
   int ft = msg.getFieldType(VID_GEOLOCATION_TIMESTAMP);
   if (ft == CSCP_DT_INT64)
   {
      m_timestamp = (time_t)msg.GetVariableInt64(VID_GEOLOCATION_TIMESTAMP);
   }
   else if (ft == CSCP_DT_INTEGER)
   {
      m_timestamp = (time_t)msg.GetVariableLong(VID_GEOLOCATION_TIMESTAMP);
   }
   else if (ft == CSCP_DT_STRING)
   {
      char ts[256];
      msg.GetVariableStrA(VID_GEOLOCATION_TIMESTAMP, ts, 256);

      struct tm timeBuff;
      if (strptime(ts, "%Y/%m/%d %H:%M:%S", &timeBuff) != NULL)
      {
         timeBuff.tm_isdst = -1;
         m_timestamp = timegm(&timeBuff);
      }
   }

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
	nx_strncpy(m_latStr, src.m_latStr, 20);
	nx_strncpy(m_lonStr, src.m_lonStr, 20);
	m_isValid = src.m_isValid;
	m_accuracy = src.m_accuracy;
	m_timestamp = src.m_timestamp;
	return *this;
}

/**
 * Fill NXCP message
 */
void GeoLocation::fillMessage(CSCPMessage &msg)
{
	msg.SetVariable(VID_GEOLOCATION_TYPE, (WORD)m_type);
	msg.SetVariable(VID_LATITUDE, m_lat);
	msg.SetVariable(VID_LONGITUDE, m_lon);
	msg.SetVariable(VID_ACCURACY, (WORD)m_accuracy);
	msg.SetVariable(VID_GEOLOCATION_TIMESTAMP, (QWORD)m_timestamp);
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

	_sntprintf(buffer, 18, _T("%02d") DEGREE_SIGN_STR _T(" %02d' %02.3f\""), getIntegerDegree(pos), getIntegerMinutes(pos), getDecimalSeconds(pos));
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
