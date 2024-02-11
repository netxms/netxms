/*
** NetXMS - Network Management System
** Copyright (C) 2024 Victor Kirhenshtein
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
** File: wireless_domain.cpp
**
**/

#include "nxcore.h"

/**
 * Create empty wireless domain object
 */
WirelessDomain::WirelessDomain() : super(), Pollable(this, Pollable::STATUS | Pollable::CONFIGURATION)
{
   memset(m_apCount, 0, sizeof(m_apCount));
}

/**
 * Create new wireless domain object with given name
 */
WirelessDomain::WirelessDomain(const TCHAR *name) : super(name), Pollable(this, Pollable::STATUS | Pollable::CONFIGURATION)
{
   memset(m_apCount, 0, sizeof(m_apCount));
}

/**
 * Destructor
 */
WirelessDomain::~WirelessDomain()
{
}

/**
 * Load from database
 */
bool WirelessDomain::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   return super::loadFromDatabase(hdb, id);
}

/**
 * Save to database
 */
bool WirelessDomain::saveToDatabase(DB_HANDLE hdb)
{
   return super::saveToDatabase(hdb);
}

/**
 * Delete from database
 */
bool WirelessDomain::deleteFromDatabase(DB_HANDLE hdb)
{
   return super::deleteFromDatabase(hdb);
}

/**
 * Status poll
 */
void WirelessDomain::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t requestId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_statusPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(status);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = requestId;

   // Create polling list
   SharedObjectArray<AccessPoint> pollList(getChildList().size(), 16);
   readLockChildList();
   int i;
   for(i = 0; i < getChildList().size(); i++)
   {
      shared_ptr<NetObj> object = getChildList().getShared(i);
      if ((object->getStatus() != STATUS_UNMANAGED) && (object->getObjectClass() == OBJECT_ACCESSPOINT))
      {
         pollList.add(static_pointer_cast<AccessPoint>(object));
      }
   }
   unlockChildList();

   sendPollerMsg(_T("Reading access points status from controllers\r\n"));
   poller->setStatus(_T("access points"));

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::statusPoll(%s [%u]): polling access points"), m_name, m_id);
   HashMap<uint32_t, SNMP_Transport> snmpTransportCache(Ownership::True);
   for(i = 0; i < pollList.size(); i++)
   {
      AccessPoint *ap = pollList.get(i);
      shared_ptr<Node> controller = ap->getController();
      if (controller != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::statusPoll(%s [%u]): requesting access point \"%s\" [%u] status from controller \"%s\" [%u]"),
            m_name, m_id, ap->getName(), ap->getId(), controller->getName(), controller->getId());
         SNMP_Transport *snmpTransport = snmpTransportCache.get(controller->getId());
         if (snmpTransport == nullptr)
         {
            snmpTransport = controller->createSnmpTransport();
            if (snmpTransport != nullptr)
               snmpTransportCache.set(controller->getId(), snmpTransport);
         }
         if (snmpTransport != nullptr)
         {
            ap->statusPollFromController(session, requestId, controller.get(), snmpTransport);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::statusPoll(%s [%u]): cannot create SNMP transport for controller node \"%s\" [%u]"), m_name, m_id, controller->getName(), controller->getId());
            sendPollerMsg(POLLER_ERROR _T("   Controller for access point %s is not accessible\r\n"), ap->getName());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::statusPoll(%s [%u]): cannot find controller node for access point \"%s\" [%u]"), m_name, m_id, ap->getName(), ap->getId());
         sendPollerMsg(POLLER_ERROR _T("   Cannot find controller for access point %s\r\n"), ap->getName());
      }
   }

   sendPollerMsg(_T("Status poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::statusPoll(%s [%u]): poll finished"), m_name, m_id);

   pollerUnlock();
}

/**
 * Configuration poll
 */
void WirelessDomain::configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t requestId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = requestId;

   poller->setStatus(_T("access points"));
   unique_ptr<SharedObjectArray<Node>> controllers = getControllers();
   ObjectArray<AccessPointInfo> accessPoints(0, 128, Ownership::True);

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("WirelessDomain::configurationPoll(%s [%u]): poll started (%d controllers)"), m_name, m_id, controllers->size());
   sendPollerMsg(POLLER_INFO _T("   %d wireless controllers in domain\r\n"), controllers->size());
   for(int i = 0; i < controllers->size(); i++)
   {
      Node *controller = controllers->get(i);
      ObjectArray<AccessPointInfo> *controllerAccessPoints = controller->getAccessPoints();
      if (controllerAccessPoints != nullptr)
      {
         accessPoints.addAll(*controllerAccessPoints);
         controllerAccessPoints->setOwner(Ownership::False);
         delete controllerAccessPoints;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("WirelessDomain::configurationPoll(%s [%u]): cannot read access point list from controller \"%s\" [%u]"),
            m_name, m_id, controller->getName(), controller->getId());
      }
   }

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("WirelessDomain::configurationPoll(%s [%u]): %d access points"), m_name, m_id, accessPoints.size());
   sendPollerMsg(_T("   %d wireless access points found\r\n"), accessPoints.size());

   int apCount[3] = { 0, 0, 0 };
   for(int i = 0; i < accessPoints.size(); i++)
   {
      AccessPointInfo *info = accessPoints.get(i);
      apCount[info->getState()]++;

      bool newAp = false;
      shared_ptr<AccessPoint> ap;
      if (info->getMacAddr().isValid())
      {
         ap = findAccessPointByMAC(info->getMacAddr());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::configurationPoll(%s [%u]): Invalid MAC address on access point %s"), m_name, m_id, info->getName());
         ap = static_pointer_cast<AccessPoint>(findChildObject(info->getName(), OBJECT_ACCESSPOINT));
      }

      if (ap == nullptr)
      {
         StringBuffer name;
         if (info->getName() != nullptr)
         {
            name = info->getName();
         }
         else
         {
            for(int j = 0; j < info->getRadioInterfaces().size(); j++)
            {
               if (j > 0)
                  name.append(_T("/"));
               name.append(info->getRadioInterfaces().get(j)->name);
            }
         }
         ap = make_shared<AccessPoint>(name.cstr(), info->getIndex(), info->getMacAddr());
         NetObjInsert(ap, true, false);
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::configurationPoll(%s [%u]): created new access point object %s [%u]"), m_name, m_id, ap->getName(), ap->getId());
         sendPollerMsg(POLLER_INFO _T("      Created new access point %s\r\n"), ap->getName());
         newAp = true;
      }
      ap->attachToDomain(m_id, info->getControllerId());
      ap->setIpAddress(info->getIpAddr());
      if ((info->getState() == AP_UP) || newAp)
      {
         ap->updateRadioInterfaces(info->getRadioInterfaces());
         ap->updateInfo(info->getVendor(), info->getModel(), info->getSerial());
      }
      ap->unhide();
      ap->updateState(info->getState());
      if (ap->getGracePeriodStartTime() != 0)
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::configurationPoll(%s [%u]): access point %s [%u] recovered from disappeared state"), m_name, m_id, ap->getName(), ap->getId());
         ap->unmarkAsDisappeared();
      }
   }

   // Delete access points no longer reported by controllers
   uint32_t retentionTime = ConfigReadULong(_T("Objects.AccessPoints.RetentionTime"), 72) * 3600;
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::configurationPoll(%s [%u]): access point retention time is %u seconds"), m_name, m_id, retentionTime);
   unique_ptr<SharedObjectArray<NetObj>> apList = getChildren(OBJECT_ACCESSPOINT);
   for(int i = 0; i < apList->size(); i++)
   {
      auto ap = static_cast<AccessPoint*>(apList->get(i));
      bool found = false;
      for(int j = 0; j < accessPoints.size(); j++)
      {
         AccessPointInfo *info = accessPoints.get(j);
         if (ap->getMacAddress().equals(info->getMacAddr()) && !_tcscmp(ap->getName(), info->getName()))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         if ((retentionTime == 0) || ((ap->getGracePeriodStartTime() != 0) && (ap->getGracePeriodStartTime() + retentionTime < time(nullptr))))
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::configurationPoll(%s [%u]): deleting non-existent access point %s [%u]"), m_name, m_id, ap->getName(), ap->getId());
            sendPollerMsg(POLLER_WARNING _T("      Access point %s deleted\r\n"), ap->getName());
            ap->deleteObject();
         }
         else if (ap->getGracePeriodStartTime() == 0)
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("WirelessDomain::configurationPoll(%s [%u]): access point %s [%u] disappeared, starting grace period"), m_name, m_id, ap->getName(), ap->getId());
            sendPollerMsg(POLLER_WARNING _T("      Access point %s is no longer reported by controller\r\n"), ap->getName());
            ap->markAsDisappeared();
         }
      }
   }

   lockProperties();
   memcpy(m_apCount, apCount, sizeof(apCount));
   unlockProperties();

   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   sendPollerMsg(_T("Configuration poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("WirelessDomain::configurationPoll(%s [%u]): poll finished"), m_name, m_id);

   lockProperties();
   m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   unlockProperties();

   pollerUnlock();
}

/**
 * Get all controllers in this domain
 */
unique_ptr<SharedObjectArray<Node>> WirelessDomain::getControllers() const
{
   auto list = new SharedObjectArray<Node>(0, 8);
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if ((object->getObjectClass() == OBJECT_NODE) && static_cast<Node*>(object)->isWirelessController())
         list->add(static_cast<Node*>(object)->self());
   }
   unlockChildList();
   return unique_ptr<SharedObjectArray<Node>>(list);
}

/**
 * Get all access points in this domain
 */
unique_ptr<SharedObjectArray<AccessPoint>> WirelessDomain::getAccessPoints() const
{
   readLockChildList();
   int count = getChildList().size();
   auto list = new SharedObjectArray<AccessPoint>(count);
   for(int i = 0; i < count; i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_ACCESSPOINT)
         list->add(static_cast<AccessPoint*>(object)->self());
   }
   unlockChildList();
   return unique_ptr<SharedObjectArray<AccessPoint>>(list);
}

/**
 * Find attached access point by MAC address
 */
shared_ptr<AccessPoint> WirelessDomain::findAccessPointByMAC(const MacAddress& macAddr) const
{
   shared_ptr<AccessPoint> ap;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_ACCESSPOINT) && static_cast<AccessPoint*>(curr)->getMacAddress().equals(macAddr))
      {
         ap = static_pointer_cast<AccessPoint>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Find access point by radio ID (radio interface index)
 */
shared_ptr<AccessPoint> WirelessDomain::findAccessPointByRadioId(uint32_t rfIndex) const
{
   shared_ptr<AccessPoint> ap;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_ACCESSPOINT) && static_cast<AccessPoint*>(curr)->isMyRadio(rfIndex))
      {
         ap = static_pointer_cast<AccessPoint>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Find attached access point by BSSID
 */
shared_ptr<AccessPoint> WirelessDomain::findAccessPointByBSSID(const BYTE *bssid) const
{
   shared_ptr<AccessPoint> ap;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_ACCESSPOINT) &&
          (static_cast<AccessPoint*>(curr)->getMacAddress().equals(bssid) || static_cast<AccessPoint*>(curr)->isMyRadio(bssid)))
      {
         ap = static_pointer_cast<AccessPoint>(getChildList().getShared(i));
         break;
      }
   }
   unlockChildList();
   return ap;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *WirelessDomain::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslWirelessDomainClass, new shared_ptr<WirelessDomain>(self())));
}
