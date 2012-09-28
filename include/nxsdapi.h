/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: nxsdapi.h
**
**/

#ifndef _nxsdapi_h_
#define _nxsdapi_h_

#ifdef _WIN32
#ifdef LIBNXSD_EXPORTS
#define LIBNXSD_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSD_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSD_EXPORTABLE
#endif


#include "nms_common.h"


/**
 * Storage element GUID structure
 */
#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

typedef struct __sguid
{
	BYTE typeAndFlags;
	BYTE data[31];
} SGUID;

#ifdef __HP_aCC
#pragma pack
#else
#pragma pack()
#endif

/**
 * Attribute set
 */
typedef struct __attribute_set
{
	SGUID guid;
	int count;
	int allocated;
	char **names;
	char **values;
} ATTRIBUTE_SET;

/**
 * API functions
 */

#ifdef __cplusplus
extern "C" {
#endif

void LIBNXSD_EXPORTABLE SerialToSGUID(const char *serial, BOOL isFake, SGUID *guid);
void LIBNXSD_EXPORTABLE DevicePathToSGUID(const char *path, SGUID *guid);
void LIBNXSD_EXPORTABLE ParseSGUID(SGUID *guid, int *type, char *data, int *isFake);

int LIBNXSD_EXPORTABLE CreateStorageElement(SGUID *guid, int deviceType, const char *name, const char *serial, const char *path);
int LIBNXSD_EXPORTABLE RemoveStorageElement(SGUID *guid);

ATTRIBUTE_SET LIBNXSD_EXPORTABLE *CreateAttributeSet(SGUID *guid);
void LIBNXSD_EXPORTABLE SetAttribute(ATTRIBUTE_SET *set, const char *attr, const char *value);
void LIBNXSD_EXPORTABLE SetAttributeInt32(ATTRIBUTE_SET *set, const char *attr, LONG value);
void LIBNXSD_EXPORTABLE SetAttributeUInt32(ATTRIBUTE_SET *set, const char *attr, DWORD value);
void LIBNXSD_EXPORTABLE SetAttributeInt64(ATTRIBUTE_SET *set, const char *attr, INT64 value);
void LIBNXSD_EXPORTABLE SetAttributeUInt64(ATTRIBUTE_SET *set, const char *attr, QWORD value);
int LIBNXSD_EXPORTABLE UpdateAttributes(ATTRIBUTE_SET *set);
void LIBNXSD_EXPORTABLE DeleteAttributeSet(ATTRIBUTE_SET *set);

int LIBNXSD_EXPORTABLE LinkStorageElement(SGUID *element, SGUID *parent);
int LIBNXSD_EXPORTABLE UnlinkStorageElement(SGUID *element, SGUID *parent);

#ifdef __cplusplus
}
#endif

#endif
