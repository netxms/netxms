/*
** NetXMS - Network Management System
** Utility Library
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
** File: json.cpp
**/

#include "libnetxms.h"

/**
 * Create JSON string from wide character string
 */
json_t LIBNETXMS_EXPORTABLE *json_string_w(const WCHAR *s)
{
   char *us = UTF8StringFromWideString(s);
   json_t *js = json_string(us);
   free(us);
   return js;
}

/**
 * Create JSON array from integer array
 */
json_t LIBNETXMS_EXPORTABLE *json_integer_array(const int *values, int size)
{
   json_t *a = json_array();
   for(int i = 0; i < size; i++)
      json_array_append_new(a, json_integer(values[i]));
   return a;
}
