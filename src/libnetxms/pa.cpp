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
   safe_free(m_country);
   safe_free(m_city);
   safe_free(m_streetAddress);
   safe_free(m_postcode);
}
