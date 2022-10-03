/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
   m_country = nullptr;
   m_region = nullptr;
   m_city = nullptr;
   m_district = nullptr;
   m_streetAddress = nullptr;
   m_postcode = nullptr;
}

/**
 * Create postal address with given values
 */
PostalAddress::PostalAddress(const TCHAR *country, const TCHAR *region, const TCHAR *city, const TCHAR *district, const TCHAR *streetAddress, const TCHAR *postcode)
{
   m_country = Trim(MemCopyString(country));
   m_region = Trim(MemCopyString(region));
   m_city = Trim(MemCopyString(city));
   m_district = Trim(MemCopyString(district));
   m_streetAddress = Trim(MemCopyString(streetAddress));
   m_postcode = Trim(MemCopyString(postcode));
}

/**
 * Copy constructor
 */
PostalAddress::PostalAddress(const PostalAddress& src)
{
   m_country = MemCopyString(src.m_country);
   m_region = MemCopyString(src.m_region);
   m_city = MemCopyString(src.m_city);
   m_district = MemCopyString(src.m_district);
   m_streetAddress = MemCopyString(src.m_streetAddress);
   m_postcode = MemCopyString(src.m_postcode);
}

/**
 * Destructor
 */
PostalAddress::~PostalAddress()
{
   MemFree(m_country);
   MemFree(m_region);
   MemFree(m_city);
   MemFree(m_district);
   MemFree(m_streetAddress);
   MemFree(m_postcode);
}

/**
 * Assignment operator
 */
PostalAddress& PostalAddress::operator =(const PostalAddress& src)
{
   if (&src == this)
      return *this;

   MemFree(m_country);
   MemFree(m_region);
   MemFree(m_city);
   MemFree(m_district);
   MemFree(m_streetAddress);
   MemFree(m_postcode);
   m_country = MemCopyString(src.m_country);
   m_region = MemCopyString(src.m_region);
   m_city = MemCopyString(src.m_city);
   m_district = MemCopyString(src.m_district);
   m_streetAddress = MemCopyString(src.m_streetAddress);
   m_postcode = MemCopyString(src.m_postcode);
   return *this;
}

/**
 * Serialize as JSON
 */
json_t *PostalAddress::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "country", json_string_t(m_country));
   json_object_set_new(root, "region", json_string_t(m_region));
   json_object_set_new(root, "city", json_string_t(m_city));
   json_object_set_new(root, "district", json_string_t(m_district));
   json_object_set_new(root, "streetAddress", json_string_t(m_streetAddress));
   json_object_set_new(root, "postcode", json_string_t(m_postcode));
   return root;
}
