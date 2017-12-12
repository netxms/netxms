/**
 * NetXMS - Network Management System
 * Driver for Rittal CMC and LCP devices
 * Copyright (C) 2017 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * File: data.cpp
 */

#include "rittal.h"

/**
 * Constructor
 */
RittalDriverData::RittalDriverData() : DriverData()
{
   m_cacheTimestamp = 0;
}

/**
 * Destructor
 */
RittalDriverData::~RittalDriverData()
{
}

/**
 * Get OID for metric
 */
const TCHAR *RittalDriverData::getMetricOid(const TCHAR *name, SNMP_Transport *snmp)
{
   if ((m_cacheTimestamp == 0) || (time(NULL) - m_cacheTimestamp > 3600))
   {
      updateOidCache(snmp);
      m_cacheTimestamp = time(NULL);
   }
   return m_oids.get(name);
}

/**
 * Callback for reading OID cache
 */
UINT32 RittalDriverData::cacheUpdateWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp)
{
   SNMP_ObjectId oid = v->getName();

   return SNMP_ERR_SUCCESS;
}

/**
 * Update OID cache
 */
void RittalDriverData::updateOidCache(SNMP_Transport *snmp)
{
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.2606.7.4.2.2.1.3"), this, &RittalDriverData::cacheUpdateWalkCallback);
}
