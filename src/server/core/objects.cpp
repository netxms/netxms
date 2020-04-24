/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
** File: objects.cpp
**
**/

#include "nxcore.h"
#include <netxms-regex.h>

/**
 * Global data
 */
BOOL g_bModificationsLocked = FALSE;

shared_ptr<Network> NXCORE_EXPORTABLE g_entireNetwork;
shared_ptr<ServiceRoot> NXCORE_EXPORTABLE g_infrastructureServiceRoot;
shared_ptr<TemplateRoot> NXCORE_EXPORTABLE g_templateRoot;
shared_ptr<NetworkMapRoot> NXCORE_EXPORTABLE g_mapRoot;
shared_ptr<DashboardRoot> NXCORE_EXPORTABLE g_dashboardRoot;
shared_ptr<BusinessServiceRoot> NXCORE_EXPORTABLE g_businessServiceRoot;

UINT32 NXCORE_EXPORTABLE g_dwMgmtNode = 0;

ObjectQueue<TemplateUpdateTask> g_templateUpdateQueue(256, Ownership::True);

ObjectIndex g_idxObjectById;
HashIndex<uuid> g_idxObjectByGUID;
ObjectIndex g_idxSubnetById;
InetAddressIndex g_idxSubnetByAddr;
InetAddressIndex g_idxInterfaceByAddr;
ObjectIndex g_idxZoneByUIN;
ObjectIndex g_idxNodeById;
InetAddressIndex g_idxNodeByAddr;
ObjectIndex g_idxClusterById;
ObjectIndex g_idxMobileDeviceById;
ObjectIndex g_idxAccessPointById;
ObjectIndex g_idxConditionById;
ObjectIndex g_idxServiceCheckById;
ObjectIndex g_idxNetMapById;
ObjectIndex g_idxChassisById;
ObjectIndex g_idxSensorById;

/**
 * Static data
 */
static int m_iStatusCalcAlg = SA_CALCULATE_MOST_CRITICAL;
static int m_iStatusPropAlg = SA_PROPAGATE_UNCHANGED;
static int m_iFixedStatus;        // Status if propagation is "Fixed"
static int m_iStatusShift;        // Shift value for "shifted" status propagation
static int m_iStatusTranslation[4];
static int m_iStatusSingleThreshold;
static int m_iStatusThresholds[4];
static THREAD s_mapUpdateThread = INVALID_THREAD_HANDLE;
static THREAD s_applyTemplateThread = INVALID_THREAD_HANDLE;

/**
 * Thread which apply template updates
 */
static THREAD_RESULT THREAD_CALL ApplyTemplateThread(void *pArg)
{
   ThreadSetName("ApplyTemplates");
	DbgPrintf(1, _T("Apply template thread started"));
   while(!IsShutdownInProgress())
   {
      TemplateUpdateTask *task = g_templateUpdateQueue.getOrBlock();
      if (task == INVALID_POINTER_VALUE)
         break;

		DbgPrintf(5, _T("ApplyTemplateThread: template=%d(%s) updateType=%d target=%d removeDci=%s"),
		         task->source->getId(), task->source->getName(), task->updateType, task->targetId, task->removeDCI ? _T("true") : _T("false"));
      BOOL bSuccess = FALSE;
      shared_ptr<NetObj> dcTarget = FindObjectById(task->targetId);
      if ((dcTarget != nullptr) && dcTarget->isDataCollectionTarget())
      {
         switch(task->updateType)
         {
            case APPLY_TEMPLATE:
               task->source->applyToTarget(static_pointer_cast<DataCollectionTarget>(dcTarget));
               static_cast<DataCollectionTarget*>(dcTarget.get())->applyDCIChanges();
               bSuccess = TRUE;
               break;
            case REMOVE_TEMPLATE:
               static_cast<DataCollectionTarget*>(dcTarget.get())->unbindFromTemplate(task->source, task->removeDCI);
               static_cast<DataCollectionTarget*>(dcTarget.get())->applyDCIChanges();
               bSuccess = TRUE;
               break;
            default:
               bSuccess = TRUE;
               break;
         }
      }

      if (bSuccess)
      {
			DbgPrintf(8, _T("ApplyTemplateThread: success"));
         delete task;
      }
      else
      {
			DbgPrintf(8, _T("ApplyTemplateThread: failed"));
         g_templateUpdateQueue.put(task);    // Requeue
         ThreadSleepMs(500);
      }
   }

	DbgPrintf(1, _T("Apply template thread stopped"));
   return THREAD_OK;
}

/**
 * Update DCI cache for all data collection targets referenced by given index
 */
static void UpdateDataCollectionCache(ObjectIndex *idx)
{
	SharedObjectArray<NetObj> *objects = idx->getObjects();
   for(int i = 0; i < objects->size(); i++)
   {
      DataCollectionTarget *t = static_cast<DataCollectionTarget*>(objects->get(i));
      t->updateDciCache();
   }
	delete objects;
}

/**
 * DCI cache loading thread
 */
static THREAD_RESULT THREAD_CALL CacheLoadingThread(void *pArg)
{
   ThreadSetName("CacheLoader");
   DbgPrintf(1, _T("Started caching of DCI values"));

	UpdateDataCollectionCache(&g_idxNodeById);
	UpdateDataCollectionCache(&g_idxClusterById);
	UpdateDataCollectionCache(&g_idxMobileDeviceById);
	UpdateDataCollectionCache(&g_idxAccessPointById);
   UpdateDataCollectionCache(&g_idxChassisById);
   UpdateDataCollectionCache(&g_idxSensorById);

   DbgPrintf(1, _T("Finished caching of DCI values"));
   return THREAD_OK;
}

/**
 * Callback for map update thread
 */
static void UpdateMapCallback(NetObj *object, void *data)
{
   if (IsShutdownInProgress())
      return;
	static_cast<NetworkMap*>(object)->updateContent();
	static_cast<NetworkMap*>(object)->calculateCompoundStatus();
}

/**
 * Map update thread
 */
static THREAD_RESULT THREAD_CALL MapUpdateThread(void *pArg)
{
   ThreadSetName("MapUpdate");
	nxlog_debug_tag(_T("obj.netmap"), 2, _T("Map update thread started"));
	while(!SleepAndCheckForShutdown(60))
	{
	   nxlog_debug_tag(_T("obj.netmap"), 6, _T("Updating maps..."));
		g_idxNetMapById.forEach(UpdateMapCallback, nullptr);
		nxlog_debug_tag(_T("obj.netmap"), 6, _T("Map update completed"));
	}
	nxlog_debug_tag(_T("obj.netmap"), 2, _T("Map update thread stopped"));
	return THREAD_OK;
}

/**
 * Initialize objects infrastructure
 */
void ObjectsInit()
{
   // Load default status calculation info
   m_iStatusCalcAlg = ConfigReadInt(_T("StatusCalculationAlgorithm"), SA_CALCULATE_MOST_CRITICAL);
   m_iStatusPropAlg = ConfigReadInt(_T("StatusPropagationAlgorithm"), SA_PROPAGATE_UNCHANGED);
   m_iFixedStatus = ConfigReadInt(_T("FixedStatusValue"), STATUS_NORMAL);
   m_iStatusShift = ConfigReadInt(_T("StatusShift"), 0);
   ConfigReadByteArray(_T("StatusTranslation"), m_iStatusTranslation, 4, STATUS_WARNING);
   m_iStatusSingleThreshold = ConfigReadInt(_T("StatusSingleThreshold"), 75);
   ConfigReadByteArray(_T("StatusThresholds"), m_iStatusThresholds, 4, 50);

   // Create "Entire Network" object
   g_entireNetwork = MakeSharedNObject<Network>();
   NetObjInsert(g_entireNetwork, false, false);

   // Create "Service Root" object
   g_infrastructureServiceRoot = MakeSharedNObject<ServiceRoot>();
   NetObjInsert(g_infrastructureServiceRoot, false, false);

   // Create "Template Root" object
   g_templateRoot = MakeSharedNObject<TemplateRoot>();
   NetObjInsert(g_templateRoot, false, false);

	// Create "Network Maps Root" object
   g_mapRoot = MakeSharedNObject<NetworkMapRoot>();
   NetObjInsert(g_mapRoot, false, false);

	// Create "Dashboard Root" object
   g_dashboardRoot = MakeSharedNObject<DashboardRoot>();
   NetObjInsert(g_dashboardRoot, false, false);

   // Create "Business Service Root" object
   g_businessServiceRoot = MakeSharedNObject<BusinessServiceRoot>();
   NetObjInsert(g_businessServiceRoot, false, false);

	DbgPrintf(1, _T("Built-in objects created"));
}

/**
 * Insert new object into network
 */
void NetObjInsert(const shared_ptr<NetObj>& object, bool newObject, bool importedObject)
{
   if (newObject)
   {
      // Assign unique ID to new object
      object->setId(CreateUniqueId(IDG_NETWORK_OBJECT));
      if (!importedObject && object->getGuid().isNull()) // imported objects already have valid GUID
         object->generateGuid();

      // Create tables for storing data collection values
      if (object->isDataCollectionTarget() && !(g_flags & AF_SINGLE_TABLE_PERF_DATA))
      {
         TCHAR szQuery[256], szQueryTemplate[256];
         UINT32 i;

         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         MetaDataReadStr(_T("IDataTableCreationCommand"), szQueryTemplate, 255, _T(""));
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, object->getId());
         DBQuery(hdb, szQuery);

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("IDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, object->getId(), object->getId());
               DBQuery(hdb, szQuery);
            }
         }

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("TDataTableCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, object->getId(), object->getId());
               DBQuery(hdb, szQuery);
            }
         }

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("TDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, object->getId(), object->getId());
               DBQuery(hdb, szQuery);
            }
         }

         DBConnectionPoolReleaseConnection(hdb);
		}
   }

	g_idxObjectById.put(object->getId(), object);
	g_idxObjectByGUID.put(object->getGuid(), object);

   if (!object->isDeleted())
   {
      switch(object->getObjectClass())
      {
         case OBJECT_GENERIC:
         case OBJECT_NETWORK:
         case OBJECT_CONTAINER:
         case OBJECT_SERVICEROOT:
         case OBJECT_NETWORKSERVICE:
         case OBJECT_TEMPLATE:
         case OBJECT_TEMPLATEGROUP:
         case OBJECT_TEMPLATEROOT:
			case OBJECT_VPNCONNECTOR:
			case OBJECT_NETWORKMAPROOT:
			case OBJECT_NETWORKMAPGROUP:
			case OBJECT_DASHBOARDROOT:
         case OBJECT_DASHBOARDGROUP:
			case OBJECT_DASHBOARD:
			case OBJECT_BUSINESSSERVICEROOT:
			case OBJECT_BUSINESSSERVICE:
			case OBJECT_NODELINK:
			case OBJECT_RACK:
            break;
         case OBJECT_NODE:
				g_idxNodeById.put(object->getId(), object);
            if (!(static_cast<Node&>(*object).getFlags() & NF_REMOTE_AGENT))
            {
			      if (IsZoningEnabled())
			      {
				      shared_ptr<Zone> zone = FindZoneByUIN(static_cast<Node&>(*object).getZoneUIN());
				      if (zone != nullptr)
				      {
					      zone->addToIndex(static_pointer_cast<Node>(object));
				      }
				      else
				      {
					      nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for node object %s [%u]"),
					               static_cast<Node&>(*object).getZoneUIN(), object->getName(), object->getId());
				      }
               }
               else
               {
                  if (static_cast<Node&>(*object).getIpAddress().isValidUnicast())
					      g_idxNodeByAddr.put(static_cast<Node&>(*object).getIpAddress(), object);
               }
            }
            break;
			case OBJECT_CLUSTER:
            g_idxClusterById.put(object->getId(), object);
            break;
			case OBJECT_MOBILEDEVICE:
				g_idxMobileDeviceById.put(object->getId(), object);
            break;
			case OBJECT_ACCESSPOINT:
				g_idxAccessPointById.put(object->getId(), object);
            MacDbAddAccessPoint(static_pointer_cast<AccessPoint>(object));
            break;
         case OBJECT_CHASSIS:
            g_idxChassisById.put(object->getId(), object);
            break;
         case OBJECT_SUBNET:
            g_idxSubnetById.put(object->getId(), object);
            if (static_cast<Subnet&>(*object).getIpAddress().isValidUnicast())
            {
					if (IsZoningEnabled())
					{
						shared_ptr<Zone> zone = FindZoneByUIN(static_cast<Subnet&>(*object).getZoneUIN());
						if (zone != nullptr)
						{
							zone->addToIndex(static_pointer_cast<Subnet>(object));
						}
						else
						{
						   nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for subnet object %s [%u]"),
						            static_cast<Subnet&>(*object).getZoneUIN(), object->getName(), object->getId());
						}
					}
					else
					{
						g_idxSubnetByAddr.put(static_cast<Subnet&>(*object).getIpAddress(), object);
					}
               if (newObject)
               {
                  InetAddress addr = static_cast<Subnet&>(*object).getIpAddress();
                  PostSystemEvent(EVENT_SUBNET_ADDED, g_dwMgmtNode, "isAd", object->getId(), object->getName(), &addr, addr.getMaskBits());
               }
            }
            break;
         case OBJECT_INTERFACE:
            if (!static_cast<Interface&>(*object).isExcludedFromTopology())
            {
					if (IsZoningEnabled())
					{
					   shared_ptr<Zone> zone = FindZoneByUIN(static_cast<Interface&>(*object).getZoneUIN());
						if (zone != nullptr)
						{
							zone->addToIndex(static_pointer_cast<Interface>(object));
						}
						else
						{
							nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for interface object %s [%u]"),
							         static_cast<Interface&>(*object).getZoneUIN(), object->getName(), object->getId());
						}
					}
					else
					{
						g_idxInterfaceByAddr.put(static_cast<Interface&>(*object).getIpAddressList(), object);
					}
            }
            MacDbAddInterface(static_pointer_cast<Interface>(object));
            break;
         case OBJECT_ZONE:
				g_idxZoneByUIN.put(static_cast<Zone&>(*object).getUIN(), object);
            break;
         case OBJECT_CONDITION:
				g_idxConditionById.put(object->getId(), object);
            break;
			case OBJECT_SLMCHECK:
				g_idxServiceCheckById.put(object->getId(), object);
            break;
			case OBJECT_NETWORKMAP:
				g_idxNetMapById.put(object->getId(), object);
            break;
			case OBJECT_SENSOR:
				g_idxSensorById.put(object->getId(), object);
            break;
         default:
				{
					bool processed = false;
					ENUMERATE_MODULES(pfNetObjInsert)
					{
                  if (CURRENT_MODULE.pfNetObjInsert(object))
                     processed = true;
					}
					if (!processed)
						nxlog_write(NXLOG_ERROR, _T("Internal error: invalid object class %d"), object->getObjectClass());
				}
            break;
      }
   }

	// Notify modules about object creation
	if (newObject)
	{
      CALL_ALL_MODULES(pfPostObjectCreate, (object));
      object->executeHookScript(_T("PostObjectCreate"));
	}
   else
   {
      CALL_ALL_MODULES(pfPostObjectLoad, (object));
   }
}

/**
 * Delete object from indexes
 * If object has an IP address, this function will delete it from
 * appropriate index. Normally this function should be called from
 * NetObj::deleteObject() method.
 */
void NetObjDeleteFromIndexes(const NetObj& object)
{
   g_idxObjectByGUID.remove(object.getGuid());
   switch(object.getObjectClass())
   {
      case OBJECT_GENERIC:
      case OBJECT_NETWORK:
      case OBJECT_CONTAINER:
      case OBJECT_SERVICEROOT:
      case OBJECT_NETWORKSERVICE:
      case OBJECT_TEMPLATE:
      case OBJECT_TEMPLATEGROUP:
      case OBJECT_TEMPLATEROOT:
		case OBJECT_VPNCONNECTOR:
		case OBJECT_NETWORKMAPROOT:
		case OBJECT_NETWORKMAPGROUP:
		case OBJECT_DASHBOARDROOT:
      case OBJECT_DASHBOARDGROUP:
		case OBJECT_DASHBOARD:
		case OBJECT_BUSINESSSERVICEROOT:
		case OBJECT_BUSINESSSERVICE:
		case OBJECT_NODELINK:
		case OBJECT_RACK:
			break;
      case OBJECT_NODE:
			g_idxNodeById.remove(object.getId());
         if (!(static_cast<const Node&>(object).getFlags() & NF_REMOTE_AGENT))
         {
			   if (IsZoningEnabled())
			   {
				   shared_ptr<Zone> zone = FindZoneByUIN(static_cast<const Node&>(object).getZoneUIN());
				   if (zone != nullptr)
				   {
					   zone->removeFromIndex(static_cast<const Node&>(object));
				   }
				   else
				   {
					   nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for node object %s [%u]"),
					            static_cast<const Node&>(object).getZoneUIN(), object.getName(), object.getId());
				   }
            }
            else
            {
			      if (static_cast<const Node&>(object).getIpAddress().isValidUnicast())
				      g_idxNodeByAddr.remove(static_cast<const Node&>(object).getIpAddress());
            }
         }
         break;
		case OBJECT_CLUSTER:
			g_idxClusterById.remove(object.getId());
         break;
      case OBJECT_MOBILEDEVICE:
			g_idxMobileDeviceById.remove(object.getId());
         break;
		case OBJECT_ACCESSPOINT:
			g_idxAccessPointById.remove(object.getId());
         MacDbRemove(static_cast<const AccessPoint&>(object).getMacAddr());
         break;
		case OBJECT_CHASSIS:
         g_idxChassisById.remove(object.getId());
         break;
      case OBJECT_SUBNET:
         g_idxSubnetById.remove(object.getId());
         if (static_cast<const Subnet&>(object).getIpAddress().isValidUnicast())
         {
				if (IsZoningEnabled())
				{
					shared_ptr<Zone> zone = FindZoneByUIN(static_cast<const Subnet&>(object).getZoneUIN());
					if (zone != nullptr)
					{
						zone->removeFromIndex(static_cast<const Subnet&>(object));
					}
					else
					{
					   nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for subnet object %s [%u]"),
					            static_cast<const Subnet&>(object).getZoneUIN(), object.getName(), object.getId());
					}
				}
				else
				{
					g_idxSubnetByAddr.remove(static_cast<const Subnet&>(object).getIpAddress());
				}
         }
         break;
      case OBJECT_INTERFACE:
			if (IsZoningEnabled())
			{
				shared_ptr<Zone> zone = FindZoneByUIN(static_cast<const Interface&>(object).getZoneUIN());
				if (zone != nullptr)
				{
					zone->removeFromIndex(static_cast<const Interface&>(object));
				}
				else
				{
					nxlog_write(NXLOG_WARNING, _T("Cannot find zone object with UIN %u for interface object %s [%u]"),
					         static_cast<const Interface&>(object).getZoneUIN(), object.getName(), object.getId());
				}
			}
			else
			{
            const ObjectArray<InetAddress> *list = static_cast<const Interface&>(object).getIpAddressList()->getList();
            for(int i = 0; i < list->size(); i++)
            {
               InetAddress *addr = list->get(i);
               if (addr->isValidUnicast())
               {
				      shared_ptr<NetObj> o = g_idxInterfaceByAddr.get(*addr);
				      if ((o != nullptr) && (o->getId() == object.getId()))
				      {
					      g_idxInterfaceByAddr.remove(*addr);
				      }
               }
            }
			}
         MacDbRemove(static_cast<const Interface&>(object).getMacAddr());
         break;
      case OBJECT_ZONE:
			g_idxZoneByUIN.remove(static_cast<const Zone&>(object).getUIN());
         break;
      case OBJECT_CONDITION:
			g_idxConditionById.remove(object.getId());
         break;
      case OBJECT_SLMCHECK:
			g_idxServiceCheckById.remove(object.getId());
         break;
		case OBJECT_NETWORKMAP:
			g_idxNetMapById.remove(object.getId());
         break;
		case OBJECT_SENSOR:
			g_idxSensorById.remove(object.getId());
         break;
      default:
			{
				bool processed = false;
				ENUMERATE_MODULES(pfNetObjDelete)
				{
               if (CURRENT_MODULE.pfNetObjDelete(object))
                  processed = true;
				}
				if (!processed)
               nxlog_write(NXLOG_ERROR, _T("Internal error: invalid object class %d"), object.getObjectClass());
			}
         break;
   }
}

/**
 * Find access point by MAC address
 */
shared_ptr<AccessPoint> NXCORE_EXPORTABLE FindAccessPointByMAC(const MacAddress& macAddr)
{
	shared_ptr<NetObj> object = MacDbFind(macAddr);
	if ((object == nullptr) || (object->getObjectClass() != OBJECT_ACCESSPOINT))
	   return shared_ptr<AccessPoint>();
   return static_pointer_cast<AccessPoint>(object);
}

/**
 * Mobile device id comparator
 */
static bool DeviceIdComparator(NetObj *object, void *deviceId)
{
	return ((object->getObjectClass() == OBJECT_MOBILEDEVICE) && !object->isDeleted() &&
		     !_tcscmp((const TCHAR *)deviceId, ((MobileDevice *)object)->getDeviceId()));
}

/**
 * Find mobile device by device ID
 */
shared_ptr<MobileDevice> NXCORE_EXPORTABLE FindMobileDeviceByDeviceID(const TCHAR *deviceId)
{
	if ((deviceId == nullptr) || (*deviceId == 0))
		return shared_ptr<MobileDevice>();

	return static_pointer_cast<MobileDevice>(g_idxMobileDeviceById.find(DeviceIdComparator, (void *)deviceId));
}

/**
 * Find node by IP address - internal implementation
 */
static shared_ptr<Node> FindNodeByIPInternal(int32_t zoneUIN, const InetAddress& ipAddr)
{
   shared_ptr<Zone> zone = IsZoningEnabled() ? FindZoneByUIN(zoneUIN) : shared_ptr<Zone>();

   shared_ptr<Node> node;
   if (IsZoningEnabled())
   {
      if (zone != nullptr)
      {
         node = zone->getNodeByAddr(ipAddr);
      }
   }
   else
   {
      node = static_pointer_cast<Node>(g_idxNodeByAddr.get(ipAddr));
   }
   if (node != nullptr)
      return node;

   shared_ptr<Interface> iface;
   if (IsZoningEnabled())
   {
      if (zone != nullptr)
      {
         iface = zone->getInterfaceByAddr(ipAddr);
      }
   }
   else
   {
      iface = static_pointer_cast<Interface>(g_idxInterfaceByAddr.get(ipAddr));
   }
   return (iface != nullptr) ? iface->getParentNode() : shared_ptr<Node>();
}

/**
 * Data for node find callback
 */
struct NodeFindCBData
{
   InetAddress addr;
   shared_ptr<Node> node;

   NodeFindCBData(const InetAddress& _addr) : addr(_addr) { }
};

/**
 * Callback for finding node in all zones
 */
static bool NodeFindCB(NetObj *zone, NodeFindCBData *data)
{
   shared_ptr<Node> node = static_cast<Zone*>(zone)->getNodeByAddr(data->addr);
   if (node == nullptr)
   {
      shared_ptr<Interface> iface = static_cast<Zone*>(zone)->getInterfaceByAddr(data->addr);
      if (iface != nullptr)
         node = iface->getParentNode();
   }

   if (node == nullptr)
      return false;

   data->node = node;
   return true;
}

/**
 * Find node by IP address
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByIP(int32_t zoneUIN, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return shared_ptr<Node>();

   if ((zoneUIN == ALL_ZONES) && IsZoningEnabled())
   {
      NodeFindCBData data(ipAddr);
      g_idxZoneByUIN.find(NodeFindCB, &data);
      return data.node;
   }
   else
   {
      return FindNodeByIPInternal(zoneUIN, ipAddr);
   }
}

/**
 * Find node by IP address
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByIP(int32_t zoneUIN, bool allZones, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return shared_ptr<Node>();

   shared_ptr<Node> node = FindNodeByIPInternal(zoneUIN, ipAddr);
   if (node != nullptr)
      return node;
   return allZones ? FindNodeByIP(ALL_ZONES, ipAddr) : shared_ptr<Node>();
}

/**
 * Find node by IP address using first match from IP address list
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByIP(int32_t zoneUIN, const InetAddressList *ipAddrList)
{
   for(int i = 0; i < ipAddrList->size(); i++)
   {
      shared_ptr<Node> node = FindNodeByIP(zoneUIN, ipAddrList->get(i));
      if (node != nullptr)
         return node;
   }
   return shared_ptr<Node>();
}

/**
 * Find interface by IP address
 */
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByIP(int32_t zoneUIN, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return shared_ptr<Interface>();

   shared_ptr<Interface> iface;
	if (IsZoningEnabled())
	{
	   shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
		if (zone != nullptr)
		{
			iface = zone->getInterfaceByAddr(ipAddr);
		}
	}
	else
	{
		iface = static_pointer_cast<Interface>(g_idxInterfaceByAddr.get(ipAddr));
	}
	return iface;
}

/**
 * Find node by MAC address
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByMAC(const BYTE *macAddr)
{
	shared_ptr<Interface> iface = FindInterfaceByMAC(macAddr);
	return (iface != nullptr) ? iface->getParentNode() : shared_ptr<Node>();
}

/**
 * Find interface by MAC address
 */
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByMAC(const BYTE *macAddr)
{
	shared_ptr<NetObj> object = MacDbFind(macAddr);
	if ((object == nullptr) || (object->getObjectClass() != OBJECT_INTERFACE))
	   return shared_ptr<Interface>();
   return static_pointer_cast<Interface>(object);
}

/**
 * Find interface by MAC address
 */
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByMAC(const MacAddress& macAddr)
{
   shared_ptr<NetObj> object = MacDbFind(macAddr);
   if ((object == nullptr) || (object->getObjectClass() != OBJECT_INTERFACE))
      return shared_ptr<Interface>();
   return static_pointer_cast<Interface>(object);
}

/**
 * Search data for node search by hostname
 */
struct NodeFindHostnameData
{
   int32_t zoneUIN;
   TCHAR hostname[MAX_DNS_NAME];
};

/**
 * Interface description comparator
 */
static bool HostnameComparator(NetObj *object, NodeFindHostnameData *data)
{
   if ((object->getObjectClass() != OBJECT_NODE) || object->isDeleted())
      return false;

   TCHAR primaryName[MAX_DNS_NAME];
   _tcslcpy(primaryName, static_cast<Node*>(object)->getPrimaryHostName(), MAX_DNS_NAME);
   _tcsupr(primaryName);
   return (_tcsstr(primaryName, data->hostname) != nullptr) && (!IsZoningEnabled() || (static_cast<Node*>(object)->getZoneUIN() == data->zoneUIN));
}

/**
 * Find a list of nodes that contain the hostname
 */
SharedObjectArray<NetObj> NXCORE_EXPORTABLE *FindNodesByHostname(int32_t zoneUIN, const TCHAR *hostname)
{
   NodeFindHostnameData data;
   data.zoneUIN = zoneUIN;
   _tcslcpy(data.hostname, hostname, MAX_DNS_NAME);
   _tcsupr(data.hostname);
   SharedObjectArray<NetObj> *nodes = g_idxNodeById.findAll(HostnameComparator, &data);
   return nodes;
}

/**
 * Interface description comparator
 */
static bool DescriptionComparator(NetObj *object, void *description)
{
	return ((object->getObjectClass() == OBJECT_INTERFACE) && !object->isDeleted() &&
	        !_tcscmp((const TCHAR *)description, ((Interface *)object)->getDescription()));
}

/**
 * Find interface by description
 */
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByDescription(const TCHAR *description, bool updateRefCount)
{
	return static_pointer_cast<Interface>(g_idxObjectById.find(DescriptionComparator, (void *)description));
}

/**
 * LLDP ID comparator
 */
static bool LldpIdComparator(NetObj *object, void *lldpId)
{
	const TCHAR *id = ((Node *)object)->getLLDPNodeId();
	return (id != nullptr) && !_tcscmp(id, (const TCHAR *)lldpId);
}

/**
 * Find node by LLDP ID
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByLLDPId(const TCHAR *lldpId)
{
	return static_pointer_cast<Node>(g_idxNodeById.find(LldpIdComparator, (void *)lldpId));
}

/**
 * SNMP sysName comparator
 */
static bool SysNameComparator(NetObj *object, const TCHAR *sysName)
{
   const TCHAR *n = static_cast<Node*>(object)->getSysName();
   return (n != nullptr) && !_tcscmp(n, sysName);
}

/**
 * Find node by SNMP sysName
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeBySysName(const TCHAR *sysName)
{
   if ((sysName == nullptr) || (sysName[0] == 0))
      return shared_ptr<Node>();

   // return nullptr if multiple nodes with same sysName found
   SharedObjectArray<NetObj> *objects = g_idxNodeById.findAll(SysNameComparator, sysName);
   shared_ptr<Node> node = (objects->size() == 1) ? static_pointer_cast<Node>(objects->getShared(0)) : shared_ptr<Node>();
   delete objects;
   return node;
}

/**
 * Bridge ID comparator
 */
static bool BridgeIdComparator(NetObj *object, void *bridgeId)
{
	return ((Node *)object)->isBridge() && !memcmp(((Node *)object)->getBridgeId(), bridgeId, MAC_ADDR_LENGTH);
}

/**
 * Find node by bridge ID (bridge base address)
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByBridgeId(const BYTE *bridgeId)
{
	return static_pointer_cast<Node>(g_idxNodeById.find(BridgeIdComparator, (void *)bridgeId));
}

/**
 * Find subnet by IP address
 */
shared_ptr<Subnet> NXCORE_EXPORTABLE FindSubnetByIP(int32_t zoneUIN, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return shared_ptr<Subnet>();

   shared_ptr<Subnet> subnet;
	if (IsZoningEnabled())
	{
		shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
		if (zone != nullptr)
		{
			subnet = zone->getSubnetByAddr(ipAddr);
		}
	}
	else
	{
		subnet = static_pointer_cast<Subnet>(g_idxSubnetByAddr.get(ipAddr));
	}
	return subnet;
}

/**
 * Subnet matching data
 */
struct SubnetMatchingData
{
   InetAddress ipAddr; // IP address to find subnet for
   int maskLen;        // Current match mask length
   shared_ptr<Subnet> subnet;     // search result

   SubnetMatchingData(const InetAddress& _ipAddr) : ipAddr(_ipAddr)
   {
      maskLen = -1;
   }
};

/**
 * Subnet matching callback
 */
static void SubnetMatchCallback(const InetAddress& addr, NetObj *object, void *context)
{
   SubnetMatchingData *data = static_cast<SubnetMatchingData*>(context);
   if (static_cast<Subnet*>(object)->getIpAddress().contain(data->ipAddr))
   {
      int maskLen = static_cast<Subnet*>(object)->getIpAddress().getMaskBits();
      if (maskLen > data->maskLen)
      {
         data->maskLen = maskLen;
         data->subnet = static_cast<Subnet*>(object)->self();
      }
   }
}

/**
 * Find subnet for given IP address
 */
shared_ptr<Subnet> NXCORE_EXPORTABLE FindSubnetForNode(int32_t zoneUIN, const InetAddress& nodeAddr)
{
   if (!nodeAddr.isValidUnicast())
      return shared_ptr<Subnet>();

   SubnetMatchingData matchData(nodeAddr);
   if (IsZoningEnabled())
   {
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         zone->forEachSubnet(SubnetMatchCallback, &matchData);
      }
   }
   else
   {
      g_idxSubnetByAddr.forEach(SubnetMatchCallback, &matchData);
   }
   return matchData.subnet;
}

/**
 * Adjust base address for new subnet to avoid overlapping with existing subnet objects.
 * Returns false if new subnet cannot be created.
 */
bool AdjustSubnetBaseAddress(InetAddress& baseAddr, int32_t zoneUIN)
{
   InetAddress addr = baseAddr.getSubnetAddress();
   while(FindSubnetByIP(zoneUIN, addr) != nullptr)
   {
      baseAddr.setMaskBits(baseAddr.getMaskBits() + 1);
      addr = baseAddr.getSubnetAddress();
   }

   // Do not create subnet if there are no address space for it
   return baseAddr.getHostBits() > 0;
}

/**
 * Find object by ID
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectById(UINT32 dwId, int objClass)
{
   ObjectIndex *index;
   switch(objClass)
   {
      case OBJECT_ACCESSPOINT:
         index = &g_idxAccessPointById;
         break;
      case OBJECT_CLUSTER:
         index = &g_idxClusterById;
         break;
      case OBJECT_MOBILEDEVICE:
         index = &g_idxMobileDeviceById;
         break;
      case OBJECT_NODE:
         index = &g_idxNodeById;
         break;
      case OBJECT_SENSOR:
         index = &g_idxSensorById;
         break;
      case OBJECT_SUBNET:
         index = &g_idxSubnetById;
         break;
      default:
         index = &g_idxObjectById;
         break;
   }

   shared_ptr<NetObj> object = index->get(dwId);
	if ((object == nullptr) || (objClass == -1))
		return object;
	return (objClass == object->getObjectClass()) ? object : shared_ptr<NetObj>();
}

/**
 * Filter for matching object name by regex and its class
 *
 * @param object
 * @param userData
 * @return
 */
static bool ObjectNameRegexAndClassFilter(NetObj *object, std::pair<int, PCRE*> *context)
{
   int ovector[30];
   return !object->isDeleted() &&
          (object->getObjectClass() == context->first) &&
          (_pcre_exec_t(context->second, nullptr, reinterpret_cast<const PCRE_TCHAR*>(object->getName()), static_cast<int>(_tcslen(object->getName())), 0, 0, ovector, 30) >= 0);
}

/**
 * Find objects whose name matches the regex
 * (refCounter is increased for each object)
 *
 * @param regex for matching object name
 * @param objClass
 * @return
 */
SharedObjectArray<NetObj> NXCORE_EXPORTABLE *FindObjectsByRegex(const TCHAR *regex, int objClass)
{
   const char *eptr;
   int eoffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(regex), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (preg == nullptr)
      return nullptr;

   ObjectIndex *index;
   switch(objClass)
   {
      case OBJECT_ACCESSPOINT:
         index = &g_idxAccessPointById;
         break;
      case OBJECT_CLUSTER:
         index = &g_idxClusterById;
         break;
      case OBJECT_MOBILEDEVICE:
         index = &g_idxMobileDeviceById;
         break;
      case OBJECT_NODE:
         index = &g_idxNodeById;
         break;
      case OBJECT_SENSOR:
         index = &g_idxSensorById;
         break;
      case OBJECT_SUBNET:
         index = &g_idxSubnetById;
         break;
      default:
         index = &g_idxObjectById;
         break;
   }

   std::pair<int, PCRE*> context(objClass, preg);
   SharedObjectArray<NetObj> *result = index->getObjects(ObjectNameRegexAndClassFilter, &context);
   _pcre_free_t(preg);
   return result;
}

/**
 * Get object name by ID
 */
const TCHAR NXCORE_EXPORTABLE *GetObjectName(DWORD id, const TCHAR *defaultName)
{
	shared_ptr<NetObj> object = g_idxObjectById.get(id);
   return (object != nullptr) ? object->getName() : defaultName;
}

/**
 * Generic object finding method
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObject(bool (* comparator)(NetObj *, void *), void *context, int objClass)
{
   ObjectIndex *index;
   switch(objClass)
   {
      case OBJECT_ACCESSPOINT:
         index = &g_idxAccessPointById;
         break;
      case OBJECT_CLUSTER:
         index = &g_idxClusterById;
         break;
      case OBJECT_MOBILEDEVICE:
         index = &g_idxMobileDeviceById;
         break;
      case OBJECT_NODE:
         index = &g_idxNodeById;
         break;
      case OBJECT_ZONE:
         index = &g_idxZoneByUIN;
         break;
      case OBJECT_SENSOR:
         index = &g_idxSensorById;
         break;
      case OBJECT_SUBNET:
         index = &g_idxSubnetById;
         break;
      default:
         index = &g_idxObjectById;
         break;
   }
   shared_ptr<NetObj> object = index->find(comparator, context);
   return ((object == nullptr) || (objClass == -1)) ? object : ((object->getObjectClass() == objClass) ? object : shared_ptr<NetObj>());
}

/**
 * Callback data for FindObjectByName
 */
struct __find_object_by_name_data
{
	int objClass;
	const TCHAR *name;
};

/**
 * Object name comparator for FindObjectByName
 */
static bool ObjectNameComparator(NetObj *object, void *data)
{
	struct __find_object_by_name_data *fd = (struct __find_object_by_name_data *)data;
	return ((fd->objClass == -1) || (fd->objClass == object->getObjectClass())) &&
	       !object->isDeleted() && !_tcsicmp(object->getName(), fd->name);
}

/**
 * Find object by name
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByName(const TCHAR *name, int objClass)
{
	struct __find_object_by_name_data data;
	data.objClass = objClass;
	data.name = name;
	return FindObject(ObjectNameComparator, &data, objClass);
}

/**
 * Find object by GUID
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByGUID(const uuid& guid, int objClass)
{
   shared_ptr<NetObj> object = g_idxObjectByGUID.get(guid);
   return ((object == nullptr) || (objClass == -1)) ? object : ((object->getObjectClass() == objClass) ? object : shared_ptr<NetObj>());
}

/**
 * Template name comparator for FindTemplateByName
 */
static bool TemplateNameComparator(NetObj *object, void *name)
{
	return (object->getObjectClass() == OBJECT_TEMPLATE) && !object->isDeleted() && !_tcsicmp(object->getName(), (const TCHAR *)name);
}

/**
 * Find template object by name
 */
shared_ptr<Template> NXCORE_EXPORTABLE FindTemplateByName(const TCHAR *name)
{
	return static_pointer_cast<Template>(g_idxObjectById.find(TemplateNameComparator, (void *)name));
}

/**
 * Data structure for IsClusterIP callback
 */
struct __cluster_ip_data
{
	InetAddress ipAddr;
	int32_t zoneUIN;
};

/**
 * Cluster IP comparator - returns true if given address in given zone is cluster IP address
 */
static bool ClusterIPComparator(NetObj *object, void *data)
{
	struct __cluster_ip_data *d = (struct __cluster_ip_data *)data;
	return (object->getObjectClass() == OBJECT_CLUSTER) && !object->isDeleted() &&
	       (((Cluster *)object)->getZoneUIN() == d->zoneUIN) &&
			 (((Cluster *)object)->isVirtualAddr(d->ipAddr) ||
			  ((Cluster *)object)->isSyncAddr(d->ipAddr));
}

/**
 * Check if given IP address is used by cluster (it's either
 * resource IP or located on one of sync subnets)
 */
bool NXCORE_EXPORTABLE IsClusterIP(int32_t zoneUIN, const InetAddress& ipAddr)
{
	struct __cluster_ip_data data;
	data.zoneUIN = zoneUIN;
	data.ipAddr = ipAddr;
	return g_idxObjectById.find(ClusterIPComparator, &data) != nullptr;
}

/**
 * Find zone object by UIN (unique identification number)
 */
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByUIN(int32_t uin)
{
	return static_pointer_cast<Zone>(g_idxZoneByUIN.get(uin));
}

/**
 * Comparator for FindZoneByProxyId
 */
static bool ZoneProxyIdComparator(NetObj *object, void *data)
{
   return static_cast<Zone*>(object)->isProxyNode(*static_cast<uint32_t*>(data));
}

/**
 * Find zone object by proxy node ID. Can be used to determine if given node is a proxy for any zone.
 */
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByProxyId(uint32_t proxyId)
{
   return static_pointer_cast<Zone>(g_idxZoneByUIN.find(ZoneProxyIdComparator, &proxyId));
}

/**
 * Zone ID selector data
 */
static Mutex s_zoneUinSelectorLock;
static IntegerArray<int32_t> s_zoneUinSelectorHistory;

/**
 * Find unused zone UIN
 */
int32_t FindUnusedZoneUIN()
{
   int32_t uin = 0;
   s_zoneUinSelectorLock.lock();
   for(UINT32 i = 1; i < 0x7FFFFFFF; i++)
   {
      if (g_idxZoneByUIN.get(i) != nullptr)
         continue;
      if (s_zoneUinSelectorHistory.contains(i))
         continue;
      s_zoneUinSelectorHistory.add(i);
      uin = i;
      break;
   }
   s_zoneUinSelectorLock.unlock();
   return uin;
}

/**
 * Object comparator for FindLocalMgmtNode()
 */
static bool LocalMgmtNodeComparator(NetObj *object, void *data)
{
	return (((Node *)object)->getCapabilities() & NC_IS_LOCAL_MGMT) ? true : false;
}

/**
 * Find local management node ID
 */
UINT32 FindLocalMgmtNode()
{
	shared_ptr<NetObj> object = g_idxNodeById.find(LocalMgmtNodeComparator, nullptr);
	return (object != nullptr) ? object->getId() : 0;
}

/**
 * ObjectIndex::forEach callback which recalculates object's status
 */
static void RecalcStatusCallback(NetObj *object, void *data)
{
	object->calculateCompoundStatus();
}

/**
 * ObjectIndex::forEach callback which links objects after loading
 */
static void LinkObjects(NetObj *object, void *data)
{
   object->linkObjects();
}

/**
 * Load objects from database at stratup
 */
BOOL LoadObjects()
{
   // Prevent objects to change it's modification flag
   g_bModificationsLocked = TRUE;

   DB_HANDLE mainDB = DBConnectionPoolAcquireConnection();
   DB_HANDLE hdb = mainDB;
   DB_HANDLE cachedb = (g_flags & AF_CACHE_DB_ON_STARTUP) ? DBOpenInMemoryDatabase() : nullptr;
   if (cachedb != nullptr)
   {
      static const TCHAR *intColumns[] = { _T("condition_id"), _T("sequence_number"), _T("dci_id"), _T("node_id"), _T("dci_func"), _T("num_pols"),
                                           _T("dashboard_id"), _T("element_id"), _T("element_type"), _T("threshold_id"), _T("item_id"),
                                           _T("check_function"), _T("check_operation"), _T("sample_count"), _T("event_code"), _T("rearm_event_code"),
                                           _T("repeat_interval"), _T("current_state"), _T("current_severity"), _T("match_count"),
                                           _T("last_event_timestamp"), _T("table_id"), _T("flags"), _T("id"), _T("activation_event"),
                                           _T("deactivation_event"), _T("group_id"), _T("iface_id"), _T("vlan_id"), _T("object_id"), nullptr };

      nxlog_debug(1, _T("Caching object configuration tables"));
      bool success =
               DBCacheTable(cachedb, mainDB, _T("object_properties"), _T("object_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("object_custom_attributes"), _T("object_id,attr_name"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("object_urls"), _T("object_id,url_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("responsible_users"), _T("object_id,user_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("nodes"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("zones"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("zone_proxies"), _T("object_id,proxy_node"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("conditions"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("cond_dci_map"), _T("condition_id,sequence_number"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("subnets"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("nsmap"), _T("subnet_id,node_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("racks"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("rack_passive_elements"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("physical_links"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("chassis"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("mobile_devices"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("sensors"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("access_points"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("interfaces"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("interface_address_list"), _T("iface_id,ip_addr"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("interface_vlan_list"), _T("iface_id,vlan_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("network_services"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("vpn_connectors"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("vpn_connector_networks"), _T("vpn_id,ip_addr"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("clusters"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("cluster_members"), _T("cluster_id,node_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("cluster_sync_subnets"), _T("cluster_id,subnet_addr"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("cluster_resources"), _T("cluster_id,resource_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("templates"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("items"), _T("item_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("thresholds"), _T("threshold_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("raw_dci_values"), _T("item_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dc_tables"), _T("item_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dc_table_columns"), _T("table_id,column_name"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dct_column_names"), _T("column_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dct_thresholds"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dct_threshold_conditions"), _T("threshold_id,group_id,sequence_number"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dct_threshold_instances"), _T("threshold_id,instance_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dct_node_map"), _T("template_id,node_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dci_schedules"), _T("item_id,schedule_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dci_access"), _T("dci_id,user_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("ap_common"), _T("guid"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_maps"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_map_elements"), _T("map_id,element_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_map_links"), nullptr, _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_map_seed_nodes"), _T("map_id,seed_node_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("node_components"), _T("node_id,component_index"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("object_containers"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("container_members"), _T("container_id,object_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dashboards"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dashboard_elements"), _T("dashboard_id,element_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dashboard_associations"), _T("object_id,dashboard_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("slm_checks"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("business_services"), _T("service_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("node_links"), _T("nodelink_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("acl"), _T("object_id,user_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("trusted_nodes"), _T("source_object_id,target_node_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("auto_bind_target"), _T("object_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("icmp_statistics"), _T("object_id,poll_target"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("icmp_target_address_list"), _T("node_id,ip_addr"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("software_inventory"), _T("node_id,name,version"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("hardware_inventory"), _T("node_id,category,component_index"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("versionable_object"), _T("object_id"), _T("*"));

      if (success)
      {
         hdb = cachedb;

         // create additional indexes
         DBQuery(cachedb, _T("CREATE INDEX idx_items_node_id ON items(node_id)"));
         DBQuery(cachedb, _T("CREATE INDEX idx_thresholds_item_id ON thresholds(item_id)"));
         DBQuery(cachedb, _T("CREATE INDEX idx_dc_tables_node_id ON dc_tables(node_id)"));
         DBQuery(cachedb, _T("CREATE INDEX idx_dct_thresholds_table_id ON dct_thresholds(table_id)"));
      }
   }

   // Load built-in object properties
   DbgPrintf(2, _T("Loading built-in object properties..."));
   g_entireNetwork->loadFromDatabase(hdb);
   g_infrastructureServiceRoot->loadFromDatabase(hdb);
   g_templateRoot->loadFromDatabase(hdb);
	g_mapRoot->loadFromDatabase(hdb);
	g_dashboardRoot->loadFromDatabase(hdb);
	g_businessServiceRoot->loadFromDatabase(hdb);

	// Switch indexes to startup mode
	g_idxObjectById.setStartupMode(true);
   g_idxSubnetById.setStartupMode(true);
	g_idxZoneByUIN.setStartupMode(true);
	g_idxNodeById.setStartupMode(true);
	g_idxClusterById.setStartupMode(true);
	g_idxMobileDeviceById.setStartupMode(true);
	g_idxAccessPointById.setStartupMode(true);
	g_idxConditionById.setStartupMode(true);
	g_idxServiceCheckById.setStartupMode(true);
	g_idxNetMapById.setStartupMode(true);
	g_idxChassisById.setStartupMode(true);
	g_idxSensorById.setStartupMode(true);

   // Load zones
   if (g_flags & AF_ENABLE_ZONING)
   {
      DbgPrintf(2, _T("Loading zones..."));

      // Load (or create) default zone
      auto zone = MakeSharedNObject<Zone>();
      zone->generateGuid();
      zone->loadFromDatabase(hdb, BUILTIN_OID_ZONE0);
      NetObjInsert(zone, false, false);
      g_entireNetwork->addZone(zone);

      DB_RESULT hResult = DBSelect(hdb, _T("SELECT id FROM zones WHERE id<>4"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            UINT32 id = DBGetFieldULong(hResult, i, 0);
            zone = MakeSharedNObject<Zone>();
            if (zone->loadFromDatabase(hdb, id))
            {
               if (!zone->isDeleted())
                  g_entireNetwork->addZone(zone);
               NetObjInsert(zone, false, false);  // Insert into indexes
            }
            else     // Object load failed
            {
               zone->destroy();
               nxlog_write(NXLOG_ERROR, _T("Failed to load zone object with ID %u from database"), id);
            }
         }
         DBFreeResult(hResult);
      }
   }
   g_idxZoneByUIN.setStartupMode(false);

   // Load conditions
   // We should load conditions before nodes because
   // DCI cache size calculation uses information from condition objects
   DbgPrintf(2, _T("Loading conditions..."));
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id FROM conditions"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto condition = MakeSharedNObject<ConditionObject>();
         if (condition->loadFromDatabase(hdb, id))
         {
            NetObjInsert(condition, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            condition->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load condition object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxConditionById.setStartupMode(false);

   // Load subnets
   DbgPrintf(2, _T("Loading subnets..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM subnets"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto subnet = MakeSharedNObject<Subnet>();
         if (subnet->loadFromDatabase(hdb, id))
         {
            if (!subnet->isDeleted())
            {
               if (g_flags & AF_ENABLE_ZONING)
               {
                  shared_ptr<Zone> zone = FindZoneByUIN(subnet->getZoneUIN());
                  if (zone != nullptr)
                     zone->addSubnet(subnet);
               }
               else
               {
                  g_entireNetwork->addSubnet(subnet);
               }
            }
            NetObjInsert(subnet, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            subnet->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load subnet object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxSubnetById.setStartupMode(false);

   // Load racks
   DbgPrintf(2, _T("Loading racks..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM racks"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto rack = MakeSharedNObject<Rack>();
         if (rack->loadFromDatabase(hdb, id))
         {
            NetObjInsert(rack, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(NXLOG_ERROR, _T("Failed to load rack object with ID %u from database"), id);
            rack->destroy();
         }
      }
      DBFreeResult(hResult);
   }

   // Load chassis
   DbgPrintf(2, _T("Loading chassis..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM chassis"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto chassis = MakeSharedNObject<Chassis>();
         if (chassis->loadFromDatabase(hdb, id))
         {
            NetObjInsert(chassis, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(NXLOG_ERROR, _T("Failed to load chassis object with ID %u from database"), id);
            chassis->destroy();
         }
      }
      DBFreeResult(hResult);
   }
   g_idxChassisById.setStartupMode(false);

   // Load mobile devices
   DbgPrintf(2, _T("Loading mobile devices..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM mobile_devices"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto md = MakeSharedNObject<MobileDevice>();
         if (md->loadFromDatabase(hdb, id))
         {
            NetObjInsert(md, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            md->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load mobile device object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxMobileDeviceById.setStartupMode(false);

   // Load sensors
   DbgPrintf(2, _T("Loading sensors..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM sensors"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto sensor = MakeSharedNObject<Sensor>();
         if (sensor->loadFromDatabase(hdb, id))
         {
            NetObjInsert(sensor, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            sensor->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load sensor object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxSensorById.setStartupMode(false);

   // Load nodes
   DbgPrintf(2, _T("Loading nodes..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM nodes"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto node = MakeSharedNObject<Node>();
         if (node->loadFromDatabase(hdb, id))
         {
            NetObjInsert(node, false, false);  // Insert into indexes
            if (IsZoningEnabled())
            {
               shared_ptr<Zone> zone = FindZoneByProxyId(id);
               if (zone != nullptr)
               {
                  zone->updateProxyStatus(node, false);
               }
            }
         }
         else     // Object load failed
         {
            node->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load node object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxNodeById.setStartupMode(false);

   // Load access points
   DbgPrintf(2, _T("Loading access points..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM access_points"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto ap = MakeSharedNObject<AccessPoint>();
         if (ap->loadFromDatabase(hdb, id))
         {
            NetObjInsert(ap, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(NXLOG_ERROR, _T("Failed to load access point object with ID %u from database"), id);
            ap->destroy();
         }
      }
      DBFreeResult(hResult);
   }
   g_idxAccessPointById.setStartupMode(false);

   // Load interfaces
   DbgPrintf(2, _T("Loading interfaces..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM interfaces"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto iface = MakeSharedNObject<Interface>();
         if (iface->loadFromDatabase(hdb, id))
         {
            NetObjInsert(iface, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(NXLOG_ERROR, _T("Failed to load interface object with ID %u from database"), id);
            iface->destroy();
         }
      }
      DBFreeResult(hResult);
   }

   // Load network services
   DbgPrintf(2, _T("Loading network services..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM network_services"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto service = MakeSharedNObject<NetworkService>();
         if (service->loadFromDatabase(hdb, id))
         {
            NetObjInsert(service, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            service->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load network service object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load VPN connectors
   DbgPrintf(2, _T("Loading VPN connectors..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM vpn_connectors"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto connector = MakeSharedNObject<VPNConnector>();
         if (connector->loadFromDatabase(hdb, id))
         {
            NetObjInsert(connector, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            connector->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load VPN connector object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load clusters
   DbgPrintf(2, _T("Loading clusters..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM clusters"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto cluster = MakeSharedNObject<Cluster>();
         if (cluster->loadFromDatabase(hdb, id))
         {
            NetObjInsert(cluster, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            cluster->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load cluster object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxClusterById.setStartupMode(false);

   // Start cache loading thread.
   // All data collection targets must be loaded at this point.
   ThreadCreate(CacheLoadingThread, 0, nullptr);

   // Load templates
   DbgPrintf(2, _T("Loading templates..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM templates"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto tmpl = MakeSharedNObject<Template>();
         if (tmpl->loadFromDatabase(hdb, id))
         {
            NetObjInsert(tmpl, false, false);  // Insert into indexes
				tmpl->calculateCompoundStatus();	// Force status change to NORMAL
         }
         else     // Object load failed
         {
            tmpl->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load template object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load network maps
   DbgPrintf(2, _T("Loading network maps..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM network_maps"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto map = MakeSharedNObject<NetworkMap>();
         if (map->loadFromDatabase(hdb, id))
         {
            NetObjInsert(map, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            map->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load network map object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxNetMapById.setStartupMode(false);

   // Load container objects
   DbgPrintf(2, _T("Loading containers..."));
   TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_CONTAINER);
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto container = MakeSharedNObject<Container>();
         if (container->loadFromDatabase(hdb, id))
         {
            NetObjInsert(container, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            container->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load container object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load template group objects
   DbgPrintf(2, _T("Loading template groups..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_TEMPLATEGROUP);
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto group = MakeSharedNObject<TemplateGroup>();
         if (group->loadFromDatabase(hdb, id))
         {
            NetObjInsert(group, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            group->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load template group object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load map group objects
   DbgPrintf(2, _T("Loading map groups..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_NETWORKMAPGROUP);
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto group = MakeSharedNObject<NetworkMapGroup>();
         if (group->loadFromDatabase(hdb, id))
         {
            NetObjInsert(group, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            group->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load network map group object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load dashboard objects
   DbgPrintf(2, _T("Loading dashboards..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM dashboards"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto dashboard = MakeSharedNObject<Dashboard>();
         if (dashboard->loadFromDatabase(hdb, id))
         {
            NetObjInsert(dashboard, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            dashboard->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load dashboard object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load dashboard group objects
   DbgPrintf(2, _T("Loading dashboard groups..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_DASHBOARDGROUP);
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto group = MakeSharedNObject<DashboardGroup>();
         if (group->loadFromDatabase(hdb, id))
         {
            NetObjInsert(group, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            group->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load dashboard group object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   // Loading business service objects
   DbgPrintf(2, _T("Loading business services..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_BUSINESSSERVICE);
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
	   int count = DBGetNumRows(hResult);
	   for(int i = 0; i < count; i++)
	   {
		   UINT32 id = DBGetFieldULong(hResult, i, 0);
		   auto service = MakeSharedNObject<BusinessService>();
		   if (service->loadFromDatabase(hdb, id))
		   {
			   NetObjInsert(service, false, false);  // Insert into indexes
		   }
		   else     // Object load failed
		   {
			   service->destroy();
			   nxlog_write(NXLOG_ERROR, _T("Failed to load business service object with ID %u from database"), id);
		   }
	   }
	   DBFreeResult(hResult);
   }

   // Loading business service objects
   DbgPrintf(2, _T("Loading node links..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_NODELINK);
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
	   int count = DBGetNumRows(hResult);
	   for(int i = 0; i < count; i++)
	   {
		   UINT32 id = DBGetFieldULong(hResult, i, 0);
		   auto nl = MakeSharedNObject<NodeLink>();
		   if (nl->loadFromDatabase(hdb, id))
		   {
			   NetObjInsert(nl, false, false);  // Insert into indexes
		   }
		   else     // Object load failed
		   {
			   nl->destroy();
			   nxlog_write(NXLOG_ERROR, _T("Failed to load node link object with ID %u from database"), id);
		   }
	   }
	   DBFreeResult(hResult);
   }

   // Load service check objects
   DbgPrintf(2, _T("Loading service checks..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM slm_checks"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto check = MakeSharedNObject<SlmCheck>();
         if (check->loadFromDatabase(hdb, id))
         {
            NetObjInsert(check, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            check->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load service check object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(mainDB);

   g_idxObjectById.setStartupMode(false);
   g_idxServiceCheckById.setStartupMode(false);

	// Load custom object classes provided by modules
   CALL_ALL_MODULES(pfLoadObjects, ());

   // Link children to container and template group objects
   DbgPrintf(2, _T("Linking objects..."));
	g_idxObjectById.forEach(LinkObjects, nullptr);

	// Link custom object classes provided by modules
   CALL_ALL_MODULES(pfLinkObjects, ());

   // Allow objects to change it's modification flag
   g_bModificationsLocked = FALSE;

   // Recalculate status for built-in objects
   g_entireNetwork->calculateCompoundStatus();
   g_infrastructureServiceRoot->calculateCompoundStatus();
   g_templateRoot->calculateCompoundStatus();
   g_mapRoot->calculateCompoundStatus();
   g_businessServiceRoot->calculateCompoundStatus();

   // Recalculate status for zone objects
   if (g_flags & AF_ENABLE_ZONING)
   {
		g_idxZoneByUIN.forEach(RecalcStatusCallback, nullptr);
   }

   // Start map update thread
   s_mapUpdateThread = ThreadCreateEx(MapUpdateThread, 0, nullptr);

   // Start template update applying thread
   s_applyTemplateThread = ThreadCreateEx(ApplyTemplateThread, 0, nullptr);

   if (cachedb != nullptr)
      DBCloseInMemoryDatabase(cachedb);

   return TRUE;
}

/**
 * Stop object maintenance threads
 */
void StopObjectMaintenanceThreads()
{
   g_templateUpdateQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_applyTemplateThread);
   ThreadJoin(s_mapUpdateThread);
}

/**
 * Callback for DeleteUserFromAllObjects
 */
static void DropUserAccess(NetObj *object, void *userId)
{
	object->dropUserAccess(CAST_FROM_POINTER(userId, UINT32));
}

/**
 * Delete user or group from all objects' ACLs
 */
void DeleteUserFromAllObjects(UINT32 dwUserId)
{
	g_idxObjectById.forEach(DropUserAccess, CAST_TO_POINTER(dwUserId, void *));
}

/**
 * User data for DumpObjectCallback
 */
struct __dump_objects_data
{
	CONSOLE_CTX console;
   const TCHAR *filter;
};

/**
 * Print object information
 */
static void PrintObjectInfo(ServerConsole *console, UINT32 objectId, const TCHAR *prefix)
{
   if (objectId == 0)
      return;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   ConsolePrintf(console, _T("%s %s [%u]\n"), prefix, (object != nullptr) ? object->getName() : _T("<unknown>"), objectId);
}

/**
 * Print ICMP statistic for node's child object
 */
template <class O> static void PrintObjectIcmpStatistic(ServerConsole *console, const O& object)
{
   auto parentNode = object.getParentNode();
   if (parentNode == nullptr)
      return;

   TCHAR target[MAX_OBJECT_NAME + 2];
   _sntprintf(target, MAX_OBJECT_NAME + 2, _T("N:%s"), object.getName());
   UINT32 last, min, max, avg, loss;
   if (parentNode->getIcmpStatistics(target, &last, &min, &max, &avg, &loss))
   {
      ConsolePrintf(console, _T("   ICMP statistics:\n"));
      ConsolePrintf(console, _T("      RTT last.........: %u ms\n"), last);
      ConsolePrintf(console, _T("      RTT min..........: %u ms\n"), min);
      ConsolePrintf(console, _T("      RTT max..........: %u ms\n"), max);
      ConsolePrintf(console, _T("      RTT average......: %u ms\n"), avg);
      ConsolePrintf(console, _T("      Packet loss......: %u\n"), loss);
   }
}

/**
 * Dump object information to console
 */
static void DumpObject(ServerConsole *console, const NetObj& object)
{
   ConsolePrintf(console, _T("\x1b[1mObject ID %d \"%s\"\x1b[0m\n")
                       _T("   Class=%s  Status=%s  IsModified=%d  IsDeleted=%d\n"),
                 object.getId(), object.getName(), object.getObjectClassName(),
                 GetStatusAsText(object.getStatus(), true),
                 object.isModified(), object.isDeleted());
   ConsolePrintf(console, _T("   Parents: <%s>\n   Children: <%s>\n"),
                 object.dbgGetParentList().cstr(), object.dbgGetChildList().cstr());

   TCHAR buffer[256];
   ConsolePrintf(console, _T("   Last change.........: %s\n"), FormatTimestamp(object.getTimeStamp(), buffer));

   if (object.isDataCollectionTarget())
   {
      ConsolePrintf(console, _T("   State flags.........: 0x%08x\n"), static_cast<const DataCollectionTarget&>(object).getState());
   }
   switch(object.getObjectClass())
   {
      case OBJECT_NODE:
         ConsolePrintf(console, _T("   Primary IP..........: %s\n   Primary hostname....: %s\n   Capabilities........: isSNMP=%d isAgent=%d isLocalMgmt=%d\n   SNMP ObjectId.......: %s\n"),
                       static_cast<const Node&>(object).getIpAddress().toString(buffer),
                       static_cast<const Node&>(object).getPrimaryHostName().cstr(),
                       static_cast<const Node&>(object).isSNMPSupported(),
                       static_cast<const Node&>(object).isNativeAgent(),
                       static_cast<const Node&>(object).isLocalManagement(),
                       static_cast<const Node&>(object).getSNMPObjectId().cstr());
         PrintObjectInfo(console, object.getAssignedZoneProxyId(false), _T("   Primary zone proxy..:"));
         PrintObjectInfo(console, object.getAssignedZoneProxyId(true), _T("   Backup zone proxy...:"));
         ConsolePrintf(console, _T("   ICMP polling........: %s\n"),
                  static_cast<const Node&>(object).isIcmpStatCollectionEnabled() ? _T("ON") : _T("OFF"));
         if (static_cast<const Node&>(object).isIcmpStatCollectionEnabled())
         {
            StringList *collectors = static_cast<const Node&>(object).getIcmpStatCollectors();
            for(int i = 0; i < collectors->size(); i++)
            {
               UINT32 last, min, max, avg, loss;
               if (static_cast<const Node&>(object).getIcmpStatistics(collectors->get(i), &last, &min, &max, &avg, &loss))
               {
                  ConsolePrintf(console, _T("   ICMP statistics (%s):\n"), collectors->get(i));
                  ConsolePrintf(console, _T("      RTT last.........: %u ms\n"), last);
                  ConsolePrintf(console, _T("      RTT min..........: %u ms\n"), min);
                  ConsolePrintf(console, _T("      RTT max..........: %u ms\n"), max);
                  ConsolePrintf(console, _T("      RTT average......: %u ms\n"), avg);
                  ConsolePrintf(console, _T("      Packet loss......: %u\n"), loss);
               }
            }
            delete collectors;
         }
         break;
      case OBJECT_SUBNET:
         ConsolePrintf(console, _T("   IP address..........: %s/%d\n"),
                  static_cast<const Subnet&>(object).getIpAddress().toString(buffer),
                  static_cast<const Subnet&>(object).getIpAddress().getMaskBits());
         break;
      case OBJECT_ACCESSPOINT:
         ConsolePrintf(console, _T("   MAC address.........: %s\n"), static_cast<const AccessPoint&>(object).getMacAddr().toString(buffer));
         ConsolePrintf(console, _T("   IP address..........: %s\n"), static_cast<const AccessPoint&>(object).getIpAddress().toString(buffer));
         PrintObjectIcmpStatistic(console, static_cast<const AccessPoint&>(object));
         break;
      case OBJECT_INTERFACE:
         ConsolePrintf(console, _T("   MAC address.........: %s\n"), static_cast<const Interface&>(object).getMacAddr().toString(buffer));
         for(int n = 0; n < static_cast<const Interface&>(object).getIpAddressList()->size(); n++)
         {
            const InetAddress& a = static_cast<const Interface&>(object).getIpAddressList()->get(n);
            ConsolePrintf(console, _T("   IP address..........: %s/%d\n"), a.toString(buffer), a.getMaskBits());
         }
         ConsolePrintf(console, _T("   Interface index.....: %u\n"), static_cast<const Interface&>(object).getIfIndex());
         ConsolePrintf(console, _T("   Physical port.......: %s\n"),
                  static_cast<const Interface&>(object).isPhysicalPort() ? _T("yes") : _T("no"));
         if (static_cast<const Interface&>(object).isPhysicalPort())
         {
            ConsolePrintf(console, _T("   Physical location...: %s\n"),
                     static_cast<const Interface&>(object).getPhysicalLocation().toString(buffer, 256));
         }
         PrintObjectIcmpStatistic(console, static_cast<const Interface&>(object));
         break;
      case OBJECT_TEMPLATE:
         ConsolePrintf(console, _T("   Version.............: %d\n"),
                  static_cast<const Template&>(object).getVersion());
         break;
      case OBJECT_ZONE:
         ConsolePrintf(console, _T("   UIN.................: %d\n"),
                  static_cast<const Zone&>(object).getUIN());
         static_cast<const Zone&>(object).dumpState(console);
         break;
   }
}

/**
 * Enumeration callback for DumpObjects
 */
static void DumpObjectCallback(NetObj *object, void *data)
{
	struct __dump_objects_data *dd = static_cast<__dump_objects_data*>(data);
   if ((dd->filter == nullptr) || MatchString(dd->filter, object->getName(), false))
   {
      DumpObject(dd->console, *object);
   }
}

/**
 * Dump objects to debug console
 */
void DumpObjects(CONSOLE_CTX pCtx, const TCHAR *filter)
{
   bool findById = false;
   if (filter != nullptr)
   {
      uuid guid = uuid::parse(filter);
      if (!guid.isNull())
      {
         findById = true;
         shared_ptr<NetObj> object = FindObjectByGUID(guid);
         if (object != nullptr)
         {
            DumpObject(pCtx, *object);
         }
         else
         {
            pCtx->printf(_T("Object with GUID %s not found\n"), filter);
         }
      }
   }

   if (!findById)
   {
      __dump_objects_data data;
      data.console = pCtx;
      data.filter = filter;
      g_idxObjectById.forEach(DumpObjectCallback, &data);
   }
}

/**
 * Check is given object class is a valid parent class for other object
 * This function is used to check manually created bindings, so it won't
 * return TRUE for node -- subnet for example
 */
bool IsValidParentClass(int childClass, int parentClass)
{
   switch(parentClass)
   {
		case OBJECT_NETWORK:
			if ((childClass == OBJECT_ZONE) && (g_flags & AF_ENABLE_ZONING))
				return true;
			break;
      case OBJECT_SERVICEROOT:
      case OBJECT_CONTAINER:
         if ((childClass == OBJECT_CHASSIS) ||
             (childClass == OBJECT_CLUSTER) ||
             (childClass == OBJECT_CONDITION) ||
             (childClass == OBJECT_CONTAINER) ||
             (childClass == OBJECT_MOBILEDEVICE) ||
             (childClass == OBJECT_NODE) ||
             (childClass == OBJECT_RACK) ||
             (childClass == OBJECT_SUBNET) ||
             (childClass == OBJECT_SENSOR))
            return true;
         break;
      case OBJECT_CHASSIS:
      case OBJECT_RACK:
         if (childClass == OBJECT_NODE)
            return true;
         break;
      case OBJECT_TEMPLATEROOT:
      case OBJECT_TEMPLATEGROUP:
         if ((childClass == OBJECT_TEMPLATEGROUP) ||
             (childClass == OBJECT_TEMPLATE))
            return true;
         break;
      case OBJECT_TEMPLATE:
         if ((childClass == OBJECT_NODE) ||
             (childClass == OBJECT_CLUSTER) ||
             (childClass == OBJECT_MOBILEDEVICE) ||
             (childClass == OBJECT_SENSOR))
            return true;
         break;
      case OBJECT_NETWORKMAPROOT:
      case OBJECT_NETWORKMAPGROUP:
         if ((childClass == OBJECT_NETWORKMAPGROUP) ||
             (childClass == OBJECT_NETWORKMAP))
            return true;
         break;
      case OBJECT_DASHBOARDROOT:
      case OBJECT_DASHBOARDGROUP:
      case OBJECT_DASHBOARD:
         if ((childClass == OBJECT_DASHBOARDGROUP) ||
             (childClass == OBJECT_DASHBOARD))
            return true;
         break;
      case OBJECT_NODE:
         if ((childClass == OBJECT_NETWORKSERVICE) ||
             (childClass == OBJECT_VPNCONNECTOR) ||
				 (childClass == OBJECT_INTERFACE))
            return true;
         break;
      case OBJECT_CLUSTER:
         if (childClass == OBJECT_NODE)
            return true;
         break;
		case OBJECT_BUSINESSSERVICEROOT:
			if ((childClass == OBJECT_BUSINESSSERVICE) ||
			    (childClass == OBJECT_NODELINK))
            return true;
         break;
		case OBJECT_BUSINESSSERVICE:
			if ((childClass == OBJECT_BUSINESSSERVICE) ||
			    (childClass == OBJECT_NODELINK) ||
			    (childClass == OBJECT_SLMCHECK))
            return true;
         break;
		case OBJECT_NODELINK:
			if (childClass == OBJECT_SLMCHECK)
            return true;
         break;
      case -1:    // Creating object without parent
         if (childClass == OBJECT_NODE)
            return true;   // OK only for nodes, because parent subnet will be created automatically
         break;
   }

   // Additional check by loaded modules
   ENUMERATE_MODULES(pfIsValidParentClass)
	{
      if (CURRENT_MODULE.pfIsValidParentClass(childClass, parentClass))
         return true;	// accepted by module
	}

   return false;
}

/**
 * Update interface index when IP address changes
 */
void UpdateInterfaceIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, const shared_ptr<Interface>& iface)
{
	if (IsZoningEnabled())
	{
		shared_ptr<Zone> zone = FindZoneByUIN(iface->getZoneUIN());
		if (zone != nullptr)
		{
			zone->updateInterfaceIndex(oldIpAddr, newIpAddr, iface);
		}
		else
		{
			nxlog_debug(1, _T("UpdateInterfaceIndex: Cannot find zone object for interface %s [%u] (zone UIN %u)"),
			         iface->getName(), iface->getId(), iface->getZoneUIN());
		}
	}
	else
	{
      g_idxInterfaceByAddr.remove(oldIpAddr);
      g_idxInterfaceByAddr.put(newIpAddr, iface);
	}
}

/**
 * Update node index when IP address changes
 */
void UpdateNodeIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, const shared_ptr<Node>& node)
{
   if (IsZoningEnabled())
   {
      shared_ptr<Zone> zone = FindZoneByUIN(node->getZoneUIN());
      if (zone != nullptr)
      {
         zone->updateNodeIndex(oldIpAddr, newIpAddr, node);
      }
      else
      {
         nxlog_debug(1, _T("UpdateNodeIndex: Cannot find zone object for node %s [%u] (zone UIN %u)"),
                  node->getName(), node->getId(), node->getZoneUIN());
      }
   }
   else
   {
      g_idxNodeByAddr.remove(oldIpAddr);
      g_idxNodeByAddr.put(newIpAddr, node);
   }
}

/**
 * Calculate propagated status for object using default algorithm
 */
int DefaultPropagatedStatus(int iObjectStatus)
{
   int iStatus;

   switch(m_iStatusPropAlg)
   {
      case SA_PROPAGATE_UNCHANGED:
         iStatus = iObjectStatus;
         break;
      case SA_PROPAGATE_FIXED:
         iStatus = ((iObjectStatus > STATUS_NORMAL) && (iObjectStatus < STATUS_UNKNOWN)) ? m_iFixedStatus : iObjectStatus;
         break;
      case SA_PROPAGATE_RELATIVE:
         if ((iObjectStatus > STATUS_NORMAL) && (iObjectStatus < STATUS_UNKNOWN))
         {
            iStatus = iObjectStatus + m_iStatusShift;
            if (iStatus < 0)
               iStatus = 0;
            if (iStatus > STATUS_CRITICAL)
               iStatus = STATUS_CRITICAL;
         }
         else
         {
            iStatus = iObjectStatus;
         }
         break;
      case SA_PROPAGATE_TRANSLATED:
         if ((iObjectStatus > STATUS_NORMAL) && (iObjectStatus < STATUS_UNKNOWN))
         {
            iStatus = m_iStatusTranslation[iObjectStatus - 1];
         }
         else
         {
            iStatus = iObjectStatus;
         }
         break;
      default:
         iStatus = STATUS_UNKNOWN;
         break;
   }
   return iStatus;
}

/**
 * Get default data for status calculation
 */
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds)
{
   *pnSingleThreshold = m_iStatusSingleThreshold;
   *ppnThresholds = m_iStatusThresholds;
   return m_iStatusCalcAlg;
}

/**
 * Returns true if object of given class can be event source
 */
bool IsEventSource(int objectClass)
{
	return (objectClass == OBJECT_NODE) ||
	       (objectClass == OBJECT_CONTAINER) ||
	       (objectClass == OBJECT_CLUSTER) ||
			 (objectClass == OBJECT_MOBILEDEVICE ||
          (objectClass == OBJECT_SENSOR));
}

/**
 * Check of object1 is parent of object2 (also indirect parent)
 */
bool NXCORE_EXPORTABLE IsParentObject(uint32_t object1, uint32_t object2)
{
   shared_ptr<NetObj> p = FindObjectById(object1);
   return (p != nullptr) ? p->isChild(object2) : false;
}

/**
 * Callback data for CreateObjectAccessSnapshot
 */
struct CreateObjectAccessSnapshot_CallbackData
{
   UINT32 userId;
   StructArray<ACL_ELEMENT> *accessList;
};

/**
 * Callback for CreateObjectAccessSnapshot
 */
static void CreateObjectAccessSnapshot_Callback(NetObj *object, void *arg)
{
   CreateObjectAccessSnapshot_CallbackData *data = (CreateObjectAccessSnapshot_CallbackData *)arg;
   UINT32 accessRights = object->getUserRights(data->userId);
   if (accessRights != 0)
   {
      ACL_ELEMENT e;
      e.dwUserId = object->getId();
      e.dwAccessRights = accessRights;
      data->accessList->add(&e);
   }
}

/**
 * Create access snapshot for given user and object class
 */
bool NXCORE_EXPORTABLE CreateObjectAccessSnapshot(UINT32 userId, int objClass)
{
   ObjectIndex *index;
   switch(objClass)
   {
      case OBJECT_ACCESSPOINT:
         index = &g_idxAccessPointById;
         break;
      case OBJECT_CLUSTER:
         index = &g_idxClusterById;
         break;
      case OBJECT_MOBILEDEVICE:
         index = &g_idxMobileDeviceById;
         break;
      case OBJECT_NODE:
         index = &g_idxNodeById;
         break;
      case OBJECT_ZONE:
         index = &g_idxZoneByUIN;
         break;
      case OBJECT_SENSOR:
         index = &g_idxSensorById;
         break;
      default:
         index = &g_idxObjectById;
         break;
   }

   StructArray<ACL_ELEMENT> accessList;
   CreateObjectAccessSnapshot_CallbackData data;
   data.userId = userId;
   data.accessList = &accessList;
   index->forEach(CreateObjectAccessSnapshot_Callback, &data);

   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (DBBegin(hdb))
   {
      success = ExecuteQueryOnObject(hdb, userId, _T("DELETE FROM object_access_snapshot WHERE user_id=?"));
      if (success && (accessList.size() > 0))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO object_access_snapshot (user_id,object_id,access_rights) VALUES (?,?,?)"), accessList.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, userId);
            for(int i = 0; (i < accessList.size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, accessList.get(i)->dwUserId);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, accessList.get(i)->dwAccessRights);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      if (success)
         DBCommit(hdb);
      else
         DBRollback(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Filter object
 */
static int FilterObject(NXSL_VM *vm, shared_ptr<NetObj> object, NXSL_VariableSystem **globalVariables)
{
   SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   vm->setContextObject(object->createNXSLObject(vm));
   NXSL_VariableSystem *expressionVariables = nullptr;
   ObjectRefArray<NXSL_Value> args(0);
   if (!vm->run(args, globalVariables, &expressionVariables))
   {
      delete expressionVariables;
      return -1;
   }
   if ((globalVariables != nullptr) && (expressionVariables != nullptr))
   {
      (*globalVariables)->merge(expressionVariables);
      delete expressionVariables;
   }
   return vm->getResult()->getValueAsBoolean() ? 1 : 0;
}

/**
 * Filter objects accessible by given user
 */
static bool FilterAccessibleObjects(NetObj *object, void *data)
{
   return object->checkAccessRights(CAST_FROM_POINTER(data, UINT32), OBJECT_ACCESS_READ);
}

/**
 * Query result comparator data
 */
struct ObjectQueryComparatorData
{
   const StringList *orderBy;
   const StringList *fields;
};

/**
 * Query result comparator
 */
static int ObjectQueryComparator(ObjectQueryComparatorData *data, const ObjectQueryResult **object1, const ObjectQueryResult **object2)
{
   for(int i = 0; i < data->orderBy->size(); i++)
   {
      bool descending = false;
      const TCHAR *attr = data->orderBy->get(i);
      if (*attr == _T('-'))
      {
         descending = true;
         attr++;
      }
      else if (*attr == _T('+'))
      {
         attr++;
      }

      int attrIndex = data->fields->indexOf(attr);
      if (attrIndex < 0)
      {
         nxlog_debug(7, _T("ObjectQueryComparator: invalid attribute \"%s\""), attr);
         continue;
      }

      const TCHAR *v1 = (*object1)->values->get(attrIndex);
      const TCHAR *v2 = (*object2)->values->get(attrIndex);

      // Try to compare as numbers
      if ((v1 != nullptr) && (v2 != nullptr))
      {
         TCHAR *eptr;
         double d1 = _tcstod(v1, &eptr);
         if (*eptr == 0)
         {
            double d2 = _tcstod(v2, &eptr);
            if (*eptr == 0)
            {
               if (d1 < d2)
                  return descending ? 1 : -1;
               if (d1 > d2)
                  return descending ? -1 : 1;
               continue;
            }
         }
      }

      // Compare as text if at least one value is not a number
      int rc = _tcsicmp(CHECK_NULL_EX(v1), CHECK_NULL_EX(v2));
      if (rc != 0)
         return descending ? -rc : rc;
   }
   return 0;
}

/**
 * Query objects
 */
ObjectArray<ObjectQueryResult> *QueryObjects(const TCHAR *query, uint32_t userId, TCHAR *errorMessage,
         size_t errorMessageLen, const StringList *fields, const StringList *orderBy, uint32_t limit)
{
   NXSL_VM *vm = NXSLCompileAndCreateVM(query, errorMessage, errorMessageLen, new NXSL_ServerEnv());
   if (vm == nullptr)
      return nullptr;

   bool readFields = (fields != nullptr);

   // Set class constants
   vm->addConstant("ACCESSPOINT", vm->createValue(OBJECT_ACCESSPOINT));
   vm->addConstant("BUSINESSSERVICE", vm->createValue(OBJECT_BUSINESSSERVICE));
   vm->addConstant("BUSINESSSERVICEROOT", vm->createValue(OBJECT_BUSINESSSERVICEROOT));
   vm->addConstant("CHASSIS", vm->createValue(OBJECT_CHASSIS));
   vm->addConstant("CLUSTER", vm->createValue(OBJECT_CLUSTER));
   vm->addConstant("CONDITION", vm->createValue(OBJECT_CONDITION));
   vm->addConstant("CONTAINER", vm->createValue(OBJECT_CONTAINER));
   vm->addConstant("DASHBOARD", vm->createValue(OBJECT_DASHBOARD));
   vm->addConstant("DASHBOARDGROUP", vm->createValue(OBJECT_DASHBOARDGROUP));
   vm->addConstant("DASHBOARDROOT", vm->createValue(OBJECT_DASHBOARDROOT));
   vm->addConstant("INTERFACE", vm->createValue(OBJECT_INTERFACE));
   vm->addConstant("MOBILEDEVICE", vm->createValue(OBJECT_MOBILEDEVICE));
   vm->addConstant("NETWORK", vm->createValue(OBJECT_NETWORK));
   vm->addConstant("NETWORKMAP", vm->createValue(OBJECT_NETWORKMAP));
   vm->addConstant("NETWORKMAPGROUP", vm->createValue(OBJECT_NETWORKMAPGROUP));
   vm->addConstant("NETWORKMAPROOT", vm->createValue(OBJECT_NETWORKMAPROOT));
   vm->addConstant("NETWORKSERVICE", vm->createValue(OBJECT_NETWORKSERVICE));
   vm->addConstant("NODE", vm->createValue(OBJECT_NODE));
   vm->addConstant("NODELINK", vm->createValue(OBJECT_NODELINK));
   vm->addConstant("RACK", vm->createValue(OBJECT_RACK));
   vm->addConstant("SENSOR", vm->createValue(OBJECT_SENSOR));
   vm->addConstant("SERVICEROOT", vm->createValue(OBJECT_SERVICEROOT));
   vm->addConstant("SLMCHECK", vm->createValue(OBJECT_SLMCHECK));
   vm->addConstant("SUBNET", vm->createValue(OBJECT_SUBNET));
   vm->addConstant("TEMPLATE", vm->createValue(OBJECT_TEMPLATE));
   vm->addConstant("TEMPLATEGROUP", vm->createValue(OBJECT_TEMPLATEGROUP));
   vm->addConstant("TEMPLATEROOT", vm->createValue(OBJECT_TEMPLATEROOT));
   vm->addConstant("VPNCONNECTOR", vm->createValue(OBJECT_VPNCONNECTOR));
   vm->addConstant("ZONE", vm->createValue(OBJECT_ZONE));

   SharedObjectArray<NetObj> *objects = g_idxObjectById.getObjects(FilterAccessibleObjects);
   ObjectArray<ObjectQueryResult> *resultSet = new ObjectArray<ObjectQueryResult>(64, 64, Ownership::True);
   for(int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> curr = objects->getShared(i);

      NXSL_VariableSystem *globals = nullptr;
      int rc = FilterObject(vm, curr, readFields ? &globals : nullptr);
      if (rc < 0)
      {
         _tcslcpy(errorMessage, vm->getErrorText(), errorMessageLen);
         delete_and_null(resultSet);
         delete globals;
         break;
      }

      if (rc > 0)
      {
         StringList *objectData;
         if (readFields)
         {
            objectData = new StringList();
            NXSL_Value *objectValue = curr->createNXSLObject(vm);
            NXSL_Object *object = objectValue->getValueAsObject();
            for(int j = 0; j < fields->size(); j++)
            {
               NXSL_Variable *v = globals->find(fields->get(j));
               if (v != nullptr)
               {
                  objectData->add(v->getValue()->getValueAsCString());
               }
               else
               {
#ifdef UNICODE
                  char attr[MAX_IDENTIFIER_LENGTH];
                  WideCharToMultiByte(CP_UTF8, 0, fields->get(j), -1, attr, MAX_IDENTIFIER_LENGTH - 1, nullptr, nullptr);
                  attr[MAX_IDENTIFIER_LENGTH - 1] = 0;
                  NXSL_Value *av = object->getClass()->getAttr(object, attr);
#else
                  NXSL_Value *av = object->getClass()->getAttr(object, fields->get(j));
#endif
                  if (av != nullptr)
                  {
                     objectData->add(av->getValueAsCString());
                     vm->destroyValue(av);
                  }
                  else
                  {
                     objectData->add(_T(""));
                  }
               }
            }
            vm->destroyValue(objectValue);
         }
         else
         {
            objectData = nullptr;
         }
         resultSet->add(new ObjectQueryResult(curr, objectData));
      }
      delete globals;
   }

   delete vm;
   delete objects;

   // Sort result set and apply limit
   if (resultSet != nullptr)
   {
      if ((orderBy != nullptr) && !orderBy->isEmpty())
      {
         ObjectQueryComparatorData cd;
         cd.fields = fields;
         cd.orderBy = orderBy;
         resultSet->sort(ObjectQueryComparator, &cd);
      }
      if (limit > 0)
      {
         resultSet->shrinkTo((int)limit);
      }
   }

   return resultSet;
}

/**
 * Node dependency check data
 */
struct NodeDependencyCheckData
{
   uint32_t nodeId;
   StructArray<DependentNode> *dependencies;
};

/**
 * Node dependency check callback
 */
static void NodeDependencyCheckCallback(NetObj *object, void *context)
{
   NodeDependencyCheckData *d = static_cast<NodeDependencyCheckData*>(context);
   Node *node = static_cast<Node*>(object);

   uint32_t type = 0;
   if (node->getEffectiveAgentProxy() == d->nodeId)
      type |= NODE_DEP_AGENT_PROXY;
   if (node->getEffectiveSnmpProxy() == d->nodeId)
      type |= NODE_DEP_SNMP_PROXY;
   if (node->getEffectiveIcmpProxy() == d->nodeId)
      type |= NODE_DEP_ICMP_PROXY;
   if (node->isDataCollectionSource(d->nodeId))
      type |= NODE_DEP_DC_SOURCE;

   if (type != 0)
   {
      DependentNode dn;
      dn.nodeId = node->getId();
      dn.dependencyType = type;
      d->dependencies->add(&dn);
   }
}

/**
 * Get dependent nodes
 */
StructArray<DependentNode> *GetNodeDependencies(uint32_t nodeId)
{
   NodeDependencyCheckData data;
   data.nodeId = nodeId;
   data.dependencies = new StructArray<DependentNode>();
   g_idxNodeById.forEach(NodeDependencyCheckCallback, &data);
   return data.dependencies;
}

/**
 * Callback for cleaning expired DCI data on node
 */
static void ResetPollTimers(NetObj *object, void *data)
{
   static_cast<DataCollectionTarget*>(object)->resetPollTimers();
}

/**
 * Reset poll timers for all nodes
 */
void ResetObjectPollTimers(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   nxlog_debug_tag(_T("poll.system"), 2, _T("Resetting object poll timers"));
   g_idxNodeById.forEach(ResetPollTimers, nullptr);
   g_idxClusterById.forEach(ResetPollTimers, nullptr);
   g_idxMobileDeviceById.forEach(ResetPollTimers, nullptr);
   g_idxSensorById.forEach(ResetPollTimers, nullptr);
   g_idxAccessPointById.forEach(ResetPollTimers, nullptr);
   g_idxChassisById.forEach(ResetPollTimers, nullptr);
}
