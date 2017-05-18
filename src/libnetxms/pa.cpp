/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: pa.cpp
**
**/

#include "libnetxms.h"

/**
 * Create empty postal address
 */
PostalAddress::PostalAddress()
{
   m_country = NULL;
   m_city = NULL;
   m_streetAddress = NULL;
   m_postcode = NULL;
}

/**
 * Create postal address with given values
 */
PostalAddress::PostalAddress(const TCHAR *country, const TCHAR *city, const TCHAR *streetAddress, const TCHAR *postcode)
{
   m_country = Trim(_tcsdup_ex(country));
   m_city = Trim(_tcsdup_ex(city));
   m_streetAddress = Trim(_tcsdup_ex(streetAddress));
   m_postcode = Trim(_tcsdup_ex(postcode));
}

/**
 * Destructor
 */
PostalAddress::~PostalAddress()
{
   free(m_country);
   free(m_city);
   free(m_streetAddress);
   free(m_postcode);
}

/**
 * Serialize as JSON
 */
json_t *PostalAddress::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "country", json_string_t(m_country));
   json_object_set_new(root, "city", json_string_t(m_city));
   json_object_set_new(root, "streetAddress", json_string_t(m_streetAddress));
   json_object_set_new(root, "postcode", json_string_t(m_postcode));
   return root;
}
