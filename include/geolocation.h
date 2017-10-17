/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: geolocation.h
**
**/

#ifndef _geolocation_h_
#define _geolocation_h_

#include <nms_util.h>
#include <nxcpapi.h>

/**
 * Location types
 */
#define GL_UNSET     0
#define GL_MANUAL    1
#define GL_GPS       2
#define GL_NETWORK   3

/**
 * Geo location class
 */
class LIBNETXMS_EXPORTABLE GeoLocation
{
private:
	int m_type;
	double m_lat;
	double m_lon;
	TCHAR m_latStr[20];
	TCHAR m_lonStr[20];
	bool m_isValid;
	int m_accuracy;
	time_t m_timestamp;

	void posToString(bool isLat, double pos);

	static int getIntegerDegree(double pos);
	static int getIntegerMinutes(double pos);
	static double getDecimalSeconds(double pos);

	static double parse(const TCHAR *str, bool isLat, bool *isValid);
	bool parseLatitude(const TCHAR *lat);
	bool parseLongitude(const TCHAR *lon);

public:
	GeoLocation();
	GeoLocation(int type, double lat, double lon, int accuracy = 0, time_t timestamp = 0);
	GeoLocation(int type, const TCHAR *lat, const TCHAR *lon, int accuracy = 0, time_t timestamp = 0);
	GeoLocation(const GeoLocation &src);
	GeoLocation(NXCPMessage &msg);
	~GeoLocation();

	GeoLocation& operator =(const GeoLocation &src);

	int getType() const { return m_type; }
	double getLatitude() const { return m_lat; }
	double getLongitude() const { return m_lon; }
	const TCHAR *getLatitudeAsString() const { return m_latStr; }
	const TCHAR *getLongitudeAsString() const { return m_lonStr; }
	bool isValid() const { return m_isValid; }
   bool isManual() const { return m_type == GL_MANUAL; }
	int getAccuracy() const { return m_accuracy; }
	time_t getTimestamp() const { return m_timestamp; }

	bool equals(const GeoLocation &location) const;
	bool sameLocation(double lat, double lon, int oldAccuracy) const;
	int calculateDistance(GeoLocation &location) const;

	void fillMessage(NXCPMessage &msg) const;

	json_t *toJson() const;

	static GeoLocation parseAgentData(const TCHAR *data);
};


#endif
