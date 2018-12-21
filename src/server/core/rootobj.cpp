/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: rootobj.cpp
**
**/

#include "nxcore.h"

/**
 * Service root class default constructor
 */
ServiceRoot::ServiceRoot() : super()
{
   m_id = BUILTIN_OID_SERVICEROOT;
   _tcscpy(m_name, _T("Infrastructure Services"));
}

/**
 * Service root class destructor
 */
ServiceRoot::~ServiceRoot()
{
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool ServiceRoot::showThresholdSummary()
{
	return true;
}

/**
 * Template root class default constructor
 */
TemplateRoot::TemplateRoot() : super()
{
   m_id = BUILTIN_OID_TEMPLATEROOT;
   _tcscpy(m_name, _T("Templates"));
	m_status = STATUS_NORMAL;
}

/**
 * Template root class destructor
 */
TemplateRoot::~TemplateRoot()
{
}

/**
 * Redefined status calculation for template root
 */
void TemplateRoot::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Network maps root class default constructor
 */
NetworkMapRoot::NetworkMapRoot() : super()
{
   m_id = BUILTIN_OID_NETWORKMAPROOT;
   _tcscpy(m_name, _T("Network Maps"));
	m_status = STATUS_NORMAL;
}

/**
 * Network maps root class destructor
 */
NetworkMapRoot::~NetworkMapRoot()
{
}

/**
 * Redefined status calculation for network maps root
 */
void NetworkMapRoot::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Dashboard tree root class default constructor
 */
DashboardRoot::DashboardRoot() : super()
{
   m_id = BUILTIN_OID_DASHBOARDROOT;
   _tcscpy(m_name, _T("Dashboards"));
	m_status = STATUS_NORMAL;
}

/**
 * Dashboard tree root class destructor
 */
DashboardRoot::~DashboardRoot()
{
}

/**
 * Redefined status calculation for dashboard tree root
 */
void DashboardRoot::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}
