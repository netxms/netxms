/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool ServiceRoot::showThresholdSummary() const
{
	return true;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *ServiceRoot::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslServiceRootClass, new shared_ptr<ServiceRoot>(self())));
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
 * Redefined status calculation for template root
 */
void TemplateRoot::calculateCompoundStatus(bool forcedRecalc)
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
 * Redefined status calculation for network maps root
 */
void NetworkMapRoot::calculateCompoundStatus(bool forcedRecalc)
{
   super::calculateCompoundStatus(forcedRecalc);
   if (m_status == STATUS_UNKNOWN)
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
 * Redefined status calculation for dashboard tree root
 */
void DashboardRoot::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Dashboard tree root class default constructor
 */
AssetRoot::AssetRoot() : super()
{
   m_id = BUILTIN_OID_ASSETROOT;
   _tcscpy(m_name, _T("Assets"));
   m_status = STATUS_NORMAL;
}

/**
 * Redefined status calculation for dashboard tree root
 */
void AssetRoot::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Dashboard tree root class default constructor
 */
BusinessServiceRoot::BusinessServiceRoot() : super()
{
   m_id = BUILTIN_OID_BUSINESSSERVICEROOT;
	_tcscpy(m_name, _T("Business Services"));
	m_status = STATUS_NORMAL;
}

/**
 * Redefined status calculation for dashboard tree root
 */
void BusinessServiceRoot::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}
