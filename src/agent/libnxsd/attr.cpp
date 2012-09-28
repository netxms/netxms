/* 
** NetXMS storage discovery API
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: attr.cpp
**
**/

#include "libnxsd.h"


/**
 * Create attribute set
 */
ATTRIBUTE_SET LIBNXSD_EXPORTABLE *CreateAttributeSet(SGUID *guid)
{
	ATTRIBUTE_SET *set = (ATTRIBUTE_SET *)malloc(sizeof(ATTRIBUTE_SET));
	memcpy(&set->guid, guid, sizeof(SGUID));
	set->count = 0;
	set->allocated = 64;
	set->names = (char **)malloc(sizeof(char *) * set->allocated);
	set->values = (char **)malloc(sizeof(char *) * set->allocated);
	return set;
}

/**
 * Delete attribute set
 */
void LIBNXSD_EXPORTABLE DeleteAttributeSet(ATTRIBUTE_SET *set)
{
	if (set == NULL)
		return;
	for(int i = 0; i < set->count; i++)
	{
		safe_free(set->names[i]);
		safe_free(set->values[i]);
	}
	safe_free(set->names);
	safe_free(set->values);
	free(set);
}
