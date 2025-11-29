/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
#include <agent_tunnel.h>

#if WITH_PRIVATE_EXTENSIONS
#include <nxlicensing.h>
#elif defined(_WIN32) && !defined(WIN32_UNRESTRICTED_BUILD)
#include <licensing.cpp>
#endif

/**
 * Global data
 */
bool g_modificationsLocked = false;

shared_ptr<Network> NXCORE_EXPORTABLE g_entireNetwork;
shared_ptr<ServiceRoot> NXCORE_EXPORTABLE g_infrastructureServiceRoot;
shared_ptr<TemplateRoot> NXCORE_EXPORTABLE g_templateRoot;
shared_ptr<NetworkMapRoot> NXCORE_EXPORTABLE g_mapRoot;
shared_ptr<DashboardRoot> NXCORE_EXPORTABLE g_dashboardRoot;
shared_ptr<AssetRoot> NXCORE_EXPORTABLE g_assetRoot;
shared_ptr<BusinessServiceRoot> NXCORE_EXPORTABLE g_businessServiceRoot;

uint32_t NXCORE_EXPORTABLE g_dwMgmtNode = 0;
wchar_t NXCORE_EXPORTABLE g_mgmtAgentAddress[128] = L"";

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
ObjectIndex g_idxBusinessServicesById;
ObjectIndex g_idxNetMapById;
ObjectIndex g_idxChassisById;
ObjectIndex g_idxSensorById;
ObjectIndex g_idxAssetById;
ObjectIndex g_idxCollectorById;
ObjectIndex g_idxCircuitById;

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
static THREAD s_applyTemplateThread = INVALID_THREAD_HANDLE;

/**
 * Zone ID selector data
 */
static Mutex s_zoneUinSelectorLock;
static HashSet<int32_t> s_zoneUinSelectorHistory;

/**
 * Save zone selector history (all zone UINs ever used for selection)
 */
static void SaveZoneSelectorHistory()
{
   StringBuffer sb;
   s_zoneUinSelectorLock.lock();
   for(const int32_t *uin : s_zoneUinSelectorHistory)
   {
      sb.append(*uin);
      sb.append(_T(','));
   }
   s_zoneUinSelectorLock.unlock();
   sb.shrink(1);
   ConfigWriteCLOB(_T("Objects.Zones.UINHistory"), sb, true);
}

/**
 * Find unused zone UIN
 */
int32_t FindUnusedZoneUIN()
{
   int32_t uin = 0;
   s_zoneUinSelectorLock.lock();
   for(int32_t i = 1; i < 0x7FFFFFFF; i++)
   {
      if (g_idxZoneByUIN.get(i) != nullptr)
         continue;
      if (s_zoneUinSelectorHistory.contains(i))
         continue;
      s_zoneUinSelectorHistory.put(i);
      uin = i;
      break;
   }
   s_zoneUinSelectorLock.unlock();
   ThreadPoolExecute(g_mainThreadPool, SaveZoneSelectorHistory);
   return uin;
}

/**
 * Thread which apply template updates
 */
static void ApplyTemplateThread()
{
   ThreadSetName("ApplyTemplates");
   nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 1, _T("Background template apply thread started"));
   while(!IsShutdownInProgress())
   {
      TemplateUpdateTask *task = g_templateUpdateQueue.getOrBlock();
      if (task == INVALID_POINTER_VALUE)
         break;

      nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 5, _T("Processing template update task for template \"%s\" [%u] (type=%d target=%u removeDci=%s)"),
            task->source->getName(), task->source->getId(), task->updateType, task->targetId, BooleanToString(task->removeDCI));
      shared_ptr<NetObj> dcTarget = FindObjectById(task->targetId);
      if ((dcTarget != nullptr) && dcTarget->isDataCollectionTarget())
      {
         switch(task->updateType)
         {
            case APPLY_TEMPLATE:
               task->source->applyToTarget(static_pointer_cast<DataCollectionTarget>(dcTarget));
               static_cast<DataCollectionTarget*>(dcTarget.get())->applyDCIChanges(false);
               break;
            case REMOVE_TEMPLATE:
               static_cast<DataCollectionTarget*>(dcTarget.get())->onTemplateRemove(task->source, task->removeDCI);
               static_cast<DataCollectionTarget*>(dcTarget.get())->applyDCIChanges(false);
               break;
            default:
               nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 6, _T("Invalid template update type %d"), task->updateType);
               break;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 5, _T("Invalid template update target %s [%u]"), (dcTarget != nullptr) ? dcTarget->getName() : _T("(null)"), task->targetId);
      }
      delete task;
   }

   nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 1, _T("Background template apply thread stopped"));
}

/**
 * Update DCI cache for all data collection targets referenced by given index
 */
static void UpdateDataCollectionCache(ObjectIndex *idx)
{
	unique_ptr<SharedObjectArray<NetObj>> objects = idx->getObjects();
   for(int i = 0; i < objects->size(); i++)
   {
      DataCollectionTarget *t = static_cast<DataCollectionTarget*>(objects->get(i));
      t->updateDciCache();
   }
}

/**
 * DCI cache loading thread
 */
static void CacheLoadingThread()
{
   ThreadSetName("CacheLoader");
   nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 1, _T("Started caching of DCI values"));

	UpdateDataCollectionCache(&g_idxNodeById);
	UpdateDataCollectionCache(&g_idxClusterById);
   UpdateDataCollectionCache(&g_idxCollectorById);
   UpdateDataCollectionCache(&g_idxCircuitById);
	UpdateDataCollectionCache(&g_idxMobileDeviceById);
	UpdateDataCollectionCache(&g_idxAccessPointById);
   UpdateDataCollectionCache(&g_idxChassisById);
   UpdateDataCollectionCache(&g_idxSensorById);

   nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 1, _T("Finished caching of DCI values"));
}

/**
 * Initialize objects infrastructure
 */
void ObjectsInit()
{
   // Load default status calculation info
   m_iStatusCalcAlg = ConfigReadInt(_T("Objects.StatusCalculation.CalculationAlgorithm"), SA_CALCULATE_MOST_CRITICAL);
   m_iStatusPropAlg = ConfigReadInt(_T("Objects.StatusCalculation.PropagationAlgorithm"), SA_PROPAGATE_UNCHANGED);
   m_iFixedStatus = ConfigReadInt(_T("Objects.StatusCalculation.FixedStatusValue"), STATUS_NORMAL);
   m_iStatusShift = ConfigReadInt(_T("Objects.StatusCalculation.Shift"), 0);
   ConfigReadByteArray(_T("Objects.StatusCalculation.Translation"), m_iStatusTranslation, 4, STATUS_WARNING);
   m_iStatusSingleThreshold = ConfigReadInt(_T("Objects.StatusCalculation.SingleThreshold"), 75);
   ConfigReadByteArray(_T("Objects.StatusCalculation.Thresholds"), m_iStatusThresholds, 4, 50);

   // Create "Entire Network" object
   g_entireNetwork = make_shared<Network>();
   NetObjInsert(g_entireNetwork, false, false);

   // Create "Service Root" object
   g_infrastructureServiceRoot = make_shared<ServiceRoot>();
   NetObjInsert(g_infrastructureServiceRoot, false, false);

   // Create "Template Root" object
   g_templateRoot = make_shared<TemplateRoot>();
   NetObjInsert(g_templateRoot, false, false);

	// Create "Network Maps Root" object
   g_mapRoot = make_shared<NetworkMapRoot>();
   NetObjInsert(g_mapRoot, false, false);

	// Create "Dashboard Root" object
   g_dashboardRoot = make_shared<DashboardRoot>();
   NetObjInsert(g_dashboardRoot, false, false);

   // Create "Asset Root" object
   g_assetRoot = make_shared<AssetRoot>();
   NetObjInsert(g_assetRoot, false, false);

   // Create "Business Service Root" object
   g_businessServiceRoot = make_shared<BusinessServiceRoot>();
   NetObjInsert(g_businessServiceRoot, false, false);

	nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 1, _T("Built-in objects created"));
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
         TCHAR query[256], queryTemplate[256];

         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         MetaDataReadStr(_T("IDataTableCreationCommand"), queryTemplate, 255, _T(""));
         _sntprintf(query, sizeof(query) / sizeof(TCHAR), queryTemplate, object->getId());
         DBQuery(hdb, query);

         for(int i = 0; i < 10; i++)
         {
            _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("IDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(query, queryTemplate, 255, _T(""));
            if (queryTemplate[0] != 0)
            {
               _sntprintf(query, sizeof(query) / sizeof(TCHAR), queryTemplate, object->getId(), object->getId());
               DBQuery(hdb, query);
            }
         }

         for(int i = 0; i < 10; i++)
         {
            _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("TDataTableCreationCommand_%d"), i);
            MetaDataReadStr(query, queryTemplate, 255, _T(""));
            if (queryTemplate[0] != 0)
            {
               _sntprintf(query, sizeof(query) / sizeof(TCHAR), queryTemplate, object->getId(), object->getId());
               DBQuery(hdb, query);
            }
         }

         for(int i = 0; i < 10; i++)
         {
            _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("TDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(query, queryTemplate, 255, _T(""));
            if (queryTemplate[0] != 0)
            {
               _sntprintf(query, sizeof(query) / sizeof(TCHAR), queryTemplate, object->getId(), object->getId());
               DBQuery(hdb, query);
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
         case OBJECT_ASSETGROUP:
         case OBJECT_ASSETROOT:
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
         case OBJECT_DASHBOARDTEMPLATE:
			case OBJECT_DASHBOARDROOT:
         case OBJECT_DASHBOARDGROUP:
			case OBJECT_DASHBOARD:
			case OBJECT_BUSINESSSERVICEROOT:
			case OBJECT_RACK:
         case OBJECT_WIRELESSDOMAIN:
            break;
         case OBJECT_NODE:
				g_idxNodeById.put(object->getId(), object);
            if (!(static_cast<Node&>(*object).getFlags() & NF_EXTERNAL_GATEWAY))
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
            if (static_cast<Node&>(*object).getAgentCertificateMappingData() != nullptr)
               UpdateAgentCertificateMappingIndex(static_pointer_cast<Node>(object), nullptr, static_cast<Node&>(*object).getAgentCertificateMappingData());
            break;
         case OBJECT_BUSINESSSERVICE:
         case OBJECT_BUSINESSSERVICEPROTO:
            g_idxBusinessServicesById.put(object->getId(), object);
            break;
         case OBJECT_ASSET:
            g_idxAssetById.put(object->getId(), object);
            break;
         case OBJECT_CIRCUIT:
            g_idxCircuitById.put(object->getId(), object);
            break;
			case OBJECT_CLUSTER:
            g_idxClusterById.put(object->getId(), object);
            break;
         case OBJECT_COLLECTOR:
            g_idxCollectorById.put(object->getId(), object);
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
                  EventBuilder(EVENT_SUBNET_ADDED, g_dwMgmtNode)
                     .param(_T("subnetObjectId"), object->getId(), EventBuilder::OBJECT_ID_FORMAT)
                     .param(_T("subnetName"), object->getName())
                     .param(_T("ipAddress"), addr)
                     .param(_T("networkMask"), addr.getMaskBits())
                     .post();
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
      case OBJECT_ASSETGROUP:
      case OBJECT_ASSETROOT:
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
      case OBJECT_DASHBOARDTEMPLATE:
		case OBJECT_DASHBOARDROOT:
      case OBJECT_DASHBOARDGROUP:
		case OBJECT_DASHBOARD:
		case OBJECT_BUSINESSSERVICEROOT:
		case OBJECT_RACK:
		case OBJECT_WIRELESSDOMAIN:
			break;
      case OBJECT_NODE:
			g_idxNodeById.remove(object.getId());
         if (!(static_cast<const Node&>(object).getFlags() & NF_EXTERNAL_GATEWAY))
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
         if (static_cast<const Node&>(object).getAgentCertificateMappingData() != nullptr)
            UpdateAgentCertificateMappingIndex(const_cast<Node&>(static_cast<const Node&>(object)).self(), static_cast<const Node&>(object).getAgentCertificateMappingData(), nullptr);
         break;
		case OBJECT_BUSINESSSERVICE:
      case OBJECT_BUSINESSSERVICEPROTO:
			g_idxBusinessServicesById.remove(object.getId());
         break;
		case OBJECT_ASSET:
			g_idxAssetById.remove(object.getId());
         break;
      case OBJECT_CIRCUIT:
         g_idxCircuitById.remove(object.getId());
         break;
      case OBJECT_CLUSTER:
         g_idxClusterById.remove(object.getId());
         break;
      case OBJECT_COLLECTOR:
         g_idxCollectorById.remove(object.getId());
         break;
      case OBJECT_MOBILEDEVICE:
			g_idxMobileDeviceById.remove(object.getId());
         break;
		case OBJECT_ACCESSPOINT:
			g_idxAccessPointById.remove(object.getId());
         MacDbRemoveAccessPoint(static_cast<const AccessPoint&>(object));
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
            const ObjectArray<InetAddress>& list = static_cast<const Interface&>(object).getIpAddressList()->getList();
            for(int i = 0; i < list.size(); i++)
            {
               InetAddress *addr = list.get(i);
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
         MacDbRemoveInterface(static_cast<const Interface&>(object));
         break;
      case OBJECT_ZONE:
         s_zoneUinSelectorLock.lock();
         s_zoneUinSelectorHistory.put(static_cast<const Zone&>(object).getUIN());
         s_zoneUinSelectorLock.unlock();
         ThreadPoolExecute(g_mainThreadPool, SaveZoneSelectorHistory);
			g_idxZoneByUIN.remove(static_cast<const Zone&>(object).getUIN());
         break;
      case OBJECT_CONDITION:
			g_idxConditionById.remove(object.getId());
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
 * Find node by MAC address
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByMAC(const MacAddress& macAddr)
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
unique_ptr<SharedObjectArray<NetObj>> NXCORE_EXPORTABLE FindNodesByHostname(int32_t zoneUIN, const TCHAR *hostname)
{
   NodeFindHostnameData data;
   data.zoneUIN = zoneUIN;
   _tcslcpy(data.hostname, hostname, MAX_DNS_NAME);
   _tcsupr(data.hostname);
   return g_idxNodeById.findAll(HostnameComparator, &data);
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
static bool LldpIdComparator(NetObj *object, const TCHAR *lldpId)
{
	const TCHAR *id = static_cast<Node*>(object)->getLLDPNodeId();
	return (id != nullptr) && !_tcscmp(id, lldpId);
}

/**
 * Find node by LLDP ID
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByLLDPId(const TCHAR *lldpId)
{
	return static_pointer_cast<Node>(g_idxNodeById.find(LldpIdComparator, lldpId));
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
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxNodeById.findAll(SysNameComparator, sysName);
   shared_ptr<Node> node = (objects->size() == 1) ? static_pointer_cast<Node>(objects->getShared(0)) : shared_ptr<Node>();
   return node;
}

/**
 * Bridge ID comparator
 */
static bool BridgeIdComparator(NetObj *object, const BYTE *bridgeId)
{
	return static_cast<Node*>(object)->isBridge() && !memcmp(static_cast<Node*>(object)->getBridgeId(), bridgeId, MAC_ADDR_LENGTH);
}

/**
 * Find node by bridge ID (bridge base address)
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByBridgeId(const BYTE *bridgeId)
{
	return static_pointer_cast<Node>(g_idxNodeById.find(BridgeIdComparator, bridgeId));
}

/**
 * Agent ID comparator
 */
static bool AgentIdComparator(NetObj *object, const uuid *agentId)
{
   return static_cast<Node*>(object)->getAgentId().equals(*agentId);
}

/**
 * Find node by agent ID
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByAgentId(const uuid& agentId)
{
   if (agentId.isNull())
      return shared_ptr<Node>();
   return static_pointer_cast<Node>(g_idxNodeById.find(AgentIdComparator, &agentId));
}

/**
 * Hardware ID comparator
 */
static bool HardwareIdComparator(NetObj *object, const NodeHardwareId *hardwareId)
{
   return static_cast<Node*>(object)->getHardwareId().equals(*hardwareId);
}

/**
 * Find node by hardware ID
 */
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByHardwareId(const NodeHardwareId& hardwareId)
{
   if (hardwareId.isNull())
      return shared_ptr<Node>();
   return static_pointer_cast<Node>(g_idxNodeById.find(HardwareIdComparator, &hardwareId));
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
   if (static_cast<Subnet*>(object)->getIpAddress().contains(data->ipAddr))
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
 * Select appropriate object index based on object class
 *
 * @param objectClassHint Object class to get index for
 * @return Pointer to the appropriate index
 */
static ObjectIndex* GetObjectIndexByClass(int objectClassHint)
{
   switch(objectClassHint)
   {
      case OBJECT_ACCESSPOINT:
         return &g_idxAccessPointById;
      case OBJECT_ASSET:
         return &g_idxAssetById;
      case OBJECT_BUSINESSSERVICE:
      case OBJECT_BUSINESSSERVICEPROTO:
         return &g_idxBusinessServicesById;
      case OBJECT_CIRCUIT:
         return &g_idxCircuitById;
      case OBJECT_CLUSTER:
         return &g_idxClusterById;
      case OBJECT_COLLECTOR:
         return &g_idxCollectorById;
      case OBJECT_MOBILEDEVICE:
         return &g_idxMobileDeviceById;
      case OBJECT_NODE:
         return &g_idxNodeById;
      case OBJECT_SENSOR:
         return &g_idxSensorById;
      case OBJECT_SUBNET:
         return &g_idxSubnetById;
      case OBJECT_ZONE:
         return &g_idxZoneByUIN;
      default:
         return &g_idxObjectById;
   }
}

/**
 * Find object by ID
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectById(uint32_t id, int objectClassHint)
{
   ObjectIndex *index = GetObjectIndexByClass(objectClassHint);
   shared_ptr<NetObj> object = index->get(id);
	if ((object == nullptr) || (objectClassHint == -1))
		return object;
	return (objectClassHint == object->getObjectClass()) ? object : shared_ptr<NetObj>();
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
unique_ptr<SharedObjectArray<NetObj>> NXCORE_EXPORTABLE FindObjectsByRegex(const TCHAR *regex, int objectClassHint)
{
   const char *eptr;
   int eoffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(regex), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (preg == nullptr)
      return unique_ptr<SharedObjectArray<NetObj>>();

   ObjectIndex *index = GetObjectIndexByClass(objectClassHint);
   std::pair<int, PCRE*> context(objectClassHint, preg);
   unique_ptr<SharedObjectArray<NetObj>> result = index->getObjects(ObjectNameRegexAndClassFilter, &context);
   _pcre_free_t(preg);
   return result;
}

/**
 * Get object name by ID
 */
const wchar_t NXCORE_EXPORTABLE *GetObjectName(uint32_t id, const wchar_t *defaultName)
{
	shared_ptr<NetObj> object = g_idxObjectById.get(id);
   return (object != nullptr) ? object->getName() : defaultName;
}

/**
 * Generic object search function
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObject(bool (*comparator)(NetObj*, void*), void *context, int objectClassHint)
{
   ObjectIndex *index = GetObjectIndexByClass(objectClassHint);
   shared_ptr<NetObj> object = index->find(comparator, context);
   return ((object == nullptr) || (objectClassHint == -1)) ? object : ((object->getObjectClass() == objectClassHint) ? object : shared_ptr<NetObj>());
}

/**
 * Generic object search function
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObject(std::function<bool (NetObj*)> comparator, int objectClassHint)
{
   ObjectIndex *index = GetObjectIndexByClass(objectClassHint);
   shared_ptr<NetObj> object = index->find(comparator);
   return ((object == nullptr) || (objectClassHint == -1)) ? object : ((object->getObjectClass() == objectClassHint) ? object : shared_ptr<NetObj>());
}

/**
 * Find object by name
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByName(const wchar_t *name, int objectClassHint)
{
	return FindObject(
	   [name, objectClassHint] (NetObj *object) -> bool
	   {
         return ((objectClassHint == -1) || (objectClassHint == object->getObjectClass())) && !object->isDeleted() && !wcsicmp(object->getName(), name);
	   }, objectClassHint);
}

/**
 * Find object by name using fuzzy search (case insensitive)
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByFuzzyName(const wchar_t *name, int objectClassHint)
{
   double similarity = 0.79999;  // minimum similarity threshold
   shared_ptr<NetObj> bestMatch = nullptr;
   ObjectIndex *index = GetObjectIndexByClass(objectClassHint);
   index->forEach(
      [name, objectClassHint, &similarity, &bestMatch] (NetObj *object) -> EnumerationCallbackResult
      {
         if (object->isDeleted())
            return _CONTINUE;
         if (objectClassHint != -1 && objectClassHint != object->getObjectClass())
            return _CONTINUE;
         double s = CalculateStringSimilarity(name, object->getName(), true);
         if (s > similarity)
         {
            similarity = s;
            bestMatch = object->self();
         }
         return s < 1.0 ? _CONTINUE : _STOP;
      });
   return bestMatch;
}

/**
 * Find object by GUID
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByGUID(const uuid& guid, int objectClassHint)
{
   shared_ptr<NetObj> object = g_idxObjectByGUID.get(guid);
   return ((object == nullptr) || (objectClassHint == -1)) ? object : ((object->getObjectClass() == objectClassHint) ? object : shared_ptr<NetObj>());
}

/**
 * Find template object by name
 */
shared_ptr<Template> NXCORE_EXPORTABLE FindTemplateByName(const TCHAR *name)
{
	return static_pointer_cast<Template>(g_idxObjectById.find(
	   [name] (NetObj *object) -> bool
	   {
	      return (object->getObjectClass() == OBJECT_TEMPLATE) && !object->isDeleted() && !_tcsicmp(object->getName(), (const TCHAR *)name);
	   }));
}

/**
 * Check if given IP address is used by cluster (it's either
 * resource IP or located on one of sync subnets).
 * Optionally can return ID of cluster object and flag indicating
 * if given IP address is resource IP.
 */
bool NXCORE_EXPORTABLE IsClusterIP(int32_t zoneUIN, const InetAddress& ipAddr, uint32_t *clusterId, bool *isResource)
{
	return g_idxObjectById.find(
	   [zoneUIN, ipAddr, clusterId, isResource] (NetObj *object) -> bool
	   {
         if ((object->getObjectClass() != OBJECT_CLUSTER) || object->isDeleted() || (static_cast<Cluster*>(object)->getZoneUIN() != zoneUIN))
	         return false;
         if (static_cast<Cluster*>(object)->isVirtualAddr(ipAddr))
         {
            if (clusterId != nullptr)
               *clusterId = object->getId();
            if (isResource != nullptr)
               *isResource = true;
            return true;
         }
         if (static_cast<Cluster*>(object)->isSyncAddr(ipAddr))
         {
            if (clusterId != nullptr)
               *clusterId = object->getId();
            if (isResource != nullptr)
               *isResource = false;
            return true;
         }
         return false;
	   }) != nullptr;
}

/**
 * Find zone object by UIN (unique identification number)
 */
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByUIN(int32_t uin)
{
	return static_pointer_cast<Zone>(g_idxZoneByUIN.get(uin));
}

/**
 * Find zone object by proxy node ID. Can be used to determine if given node is a proxy for any zone.
 */
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByProxyId(uint32_t proxyId)
{
   return static_pointer_cast<Zone>(g_idxZoneByUIN.find(
      [proxyId] (NetObj *object) -> bool
      {
         return static_cast<Zone*>(object)->isProxyNode(proxyId);
      }));
}

/**
 * Find local management node ID
 */
uint32_t FindLocalMgmtNode()
{
	shared_ptr<NetObj> object = g_idxNodeById.find([](NetObj *object, void *context) -> bool { return static_cast<Node*>(object)->isLocalManagement(); }, nullptr);
	return (object != nullptr) ? object->getId() : 0;
}

/**
 * Template function for loading objects from database
 * 
 * @param className    object class name
 * @param hdb          database handle
 * @param query        sets table and WHERE condition, if needed
 * @param index        clearing startup mode for specific object index
 * @param beforeInsert function called before object insertion in indexes
 * @param afterInsert  function called after object insertion in indexes
 */
template<typename T> static void LoadObjectsFromTable(const TCHAR *className, DB_HANDLE hdb, DB_STATEMENT *preparedStatements,
   const TCHAR *query, void (*beforeInsert)(const shared_ptr<T>& obj) = nullptr, void (*afterInsert)(const shared_ptr<T>& obj) = nullptr)
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Loading %s%s..."), className, _tcscmp(className, _T("chassis")) ? _T("s") : _T(""));
   DB_RESULT hResult = DBSelectFormatted(hdb, _T("SELECT id FROM %s"), query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         auto object = make_shared<T>();
         if (object->loadFromDatabase(hdb, id, preparedStatements))
         {
            // In case we need some logic before inserting object to indexes
            if (beforeInsert != nullptr)
            {
               beforeInsert(object);
            }

            // Insert into indexes
            NetObjInsert(object, false, false);

            // In case we need some logic after inserting object to indexes
            if (afterInsert != nullptr)
            {
               afterInsert(object);
            }
         }
         else     // Object load failed
         {
            object->destroy();
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_OBJECT_INIT, _T("Failed to load %s object with ID %u from database"), className, id);
         }
      }
      DBFreeResult(hResult);
   }
}

/**
 * Scheduled task for comments macros expansion
 */
void ExpandCommentMacrosTask(const shared_ptr<ScheduledTaskParameters> &parameters)
{
   nxlog_debug_tag(_T("obj.comments"), 2, _T("Updating all objects comments macros"));
   g_idxObjectById.forEach(
      [] (NetObj *object) -> EnumerationCallbackResult
      {
         object->expandCommentMacros();
         return _CONTINUE;
      });
   nxlog_debug_tag(_T("obj.comments"), 5, _T("Objects comments macros update complete"));
}

/**
 * Load objects from database at stratup
 */
bool LoadObjects()
{
   // Prevent objects to change it's modification flag
   g_modificationsLocked = true;

   // Load zone selector history
   wchar_t *uinHistory = ConfigReadCLOB(L"Objects.Zones.UINHistory", L"");
   StringList uinList = String::split(uinHistory, wcslen(uinHistory), L",");
   for(int i = 0; i < uinList.size(); i++)
   {
      wchar_t *eptr;
      int32_t uin = wcstol(uinList.get(i), &eptr, 10);
      if ((uin != 0) && (*eptr == 0))
         s_zoneUinSelectorHistory.put(uin);
   }
   MemFree(uinHistory);

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
                                           _T("deactivation_event"), _T("group_id"), _T("iface_id"), _T("vlan_id"), _T("object_id"),
                                           _T("asset_id"), _T("owner_id"), _T("radio_index"), nullptr };

      nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 1, _T("Caching object configuration tables"));
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
               DBCacheTable(cachedb, mainDB, _T("radios"), _T("owner_id,radio_index,bssid"), _T("*"), intColumns) &&
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
               DBCacheTable(cachedb, mainDB, _T("dc_targets"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dct_thresholds"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dct_threshold_conditions"), _T("threshold_id,group_id,sequence_number"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dct_threshold_instances"), _T("threshold_id,instance_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dct_node_map"), _T("template_id,node_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dci_delete_list"), _T("node_id,dci_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dci_schedules"), _T("item_id,schedule_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dci_access"), _T("dci_id,user_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("ap_common"), _T("guid"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_maps"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_map_deleted_nodes"), _T("map_id,object_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_map_elements"), _T("map_id,element_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_map_links"), _T("map_id,link_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("network_map_seed_nodes"), _T("map_id,seed_node_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("node_components"), _T("node_id,component_index"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("object_containers"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("ospf_areas"), _T("node_id,area_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("ospf_neighbors"), _T("node_id,router_id,if_index,ip_address"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("container_members"), _T("container_id,object_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dashboards"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dashboard_elements"), _T("dashboard_id,element_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("dashboard_templates"), _T("id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dashboard_template_instances"), _T("dashboard_template_id,instance_object_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("dashboard_associations"), _T("object_id,dashboard_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("business_service_checks"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("business_services"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("business_service_prototypes"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("acl"), _T("object_id,user_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("trusted_objects"), _T("object_id,trusted_object_id"), _T("*")) &&
               DBCacheTable(cachedb, mainDB, _T("auto_bind_target"), _T("object_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("icmp_statistics"), _T("object_id,poll_target"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("icmp_target_address_list"), _T("node_id,ip_addr"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("software_inventory"), _T("node_id,name,version"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("hardware_inventory"), _T("node_id,category,component_index"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("versionable_object"), _T("object_id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("pollable_objects"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("assets"), _T("id"), _T("*"), intColumns) &&
               DBCacheTable(cachedb, mainDB, _T("asset_properties"), _T("asset_id,attr_name"), _T("*"));

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

   DB_STATEMENT preparedStatements[LSI_MAX_VALUE];
   memset(preparedStatements, 0, sizeof(preparedStatements));

   // Load built-in object properties
   nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Loading built-in object properties..."));
   g_entireNetwork->loadFromDatabase(hdb, preparedStatements);
   g_infrastructureServiceRoot->loadFromDatabase(hdb, preparedStatements);
   g_templateRoot->loadFromDatabase(hdb, preparedStatements);
	g_mapRoot->loadFromDatabase(hdb, preparedStatements);
	g_dashboardRoot->loadFromDatabase(hdb, preparedStatements);
   g_assetRoot->loadFromDatabase(hdb, preparedStatements);
	g_businessServiceRoot->loadFromDatabase(hdb, preparedStatements);

	// Switch indexes to startup mode
	g_idxObjectById.setStartupMode(true);
   g_idxSubnetById.setStartupMode(true);
	g_idxZoneByUIN.setStartupMode(true);
	g_idxNodeById.setStartupMode(true);
   g_idxAssetById.setStartupMode(true);
   g_idxCircuitById.setStartupMode(true);
	g_idxClusterById.setStartupMode(true);
   g_idxCollectorById.setStartupMode(true);
	g_idxMobileDeviceById.setStartupMode(true);
	g_idxAccessPointById.setStartupMode(true);
	g_idxConditionById.setStartupMode(true);
	g_idxBusinessServicesById.setStartupMode(true);
	g_idxNetMapById.setStartupMode(true);
	g_idxChassisById.setStartupMode(true);
	g_idxSensorById.setStartupMode(true);

   // Load zones
   if (g_flags & AF_ENABLE_ZONING)
   {
      // Load (or create) default zone
      auto zone = make_shared<Zone>();
      zone->generateGuid();
      zone->loadFromDatabase(hdb, BUILTIN_OID_ZONE0, preparedStatements);
      NetObjInsert(zone, false, false);
      NetObj::linkObjects(g_entireNetwork, zone);

      // Load zones from database
      LoadObjectsFromTable<Zone>(_T("zone"), hdb, preparedStatements, _T("zones WHERE id<>4"), nullptr,
         [] (const shared_ptr<Zone>& zone)
         {
            if (!zone->isDeleted())
               NetObj::linkObjects(g_entireNetwork, zone);
         });
   }
   g_idxZoneByUIN.setStartupMode(false);

   // We should load conditions before nodes because
   // DCI cache size calculation uses information from condition objects
   LoadObjectsFromTable<ConditionObject>(_T("condition"), hdb, preparedStatements, _T("conditions"));
   g_idxConditionById.setStartupMode(false);

   if (IsZoningEnabled())
   {
      LoadObjectsFromTable<Subnet>(_T("subnet"), hdb, preparedStatements, _T("subnets"),
         [] (const shared_ptr<Subnet>& subnet)
         {
            if (!subnet->isDeleted())
            {
               shared_ptr<Zone> zone = FindZoneByUIN(subnet->getZoneUIN());
               if (zone != nullptr)
               {
                  NetObj::linkObjects(zone, subnet);
               }
            }
         });
   }
   else
   {
      LoadObjectsFromTable<Subnet>(_T("subnet"), hdb, preparedStatements, _T("subnets"),
         [] (const shared_ptr<Subnet>& subnet)
         {
            if (!subnet->isDeleted())
            {
               NetObj::linkObjects(g_entireNetwork, subnet);
            }
         });
   }
   g_idxSubnetById.setStartupMode(false);

   LoadObjectsFromTable<Rack>(_T("rack"), hdb, preparedStatements, _T("racks"));
   LoadObjectsFromTable<Chassis>(_T("chassis"), hdb, preparedStatements, _T("chassis"));
   g_idxChassisById.setStartupMode(false);
   LoadObjectsFromTable<MobileDevice>(_T("mobile device"), hdb, preparedStatements, _T("mobile_devices"));
   g_idxMobileDeviceById.setStartupMode(false);
   LoadObjectsFromTable<Sensor>(_T("sensor"), hdb, preparedStatements, _T("sensors"));
   g_idxSensorById.setStartupMode(false);

   LoadObjectsFromTable<Node>(_T("node"), hdb, preparedStatements, _T("nodes"), nullptr,
      IsZoningEnabled() ?
         [] (const shared_ptr<Node>& node)
         {
            shared_ptr<Zone> zone = FindZoneByProxyId(node->getId());
            if (zone != nullptr)
            {
               zone->updateProxyStatus(node, false);
            }
         }
      : static_cast<void (*)(const std::shared_ptr<Node>&)>(nullptr));
   g_idxNodeById.setStartupMode(false);

   LoadObjectsFromTable<WirelessDomain>(_T("wireless domain"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_WIRELESSDOMAIN));
   LoadObjectsFromTable<AccessPoint>(_T("access point"), hdb, preparedStatements, _T("access_points"));
   g_idxAccessPointById.setStartupMode(false);
   LoadObjectsFromTable<Interface>(_T("interface"), hdb, preparedStatements, _T("interfaces"));
   LoadObjectsFromTable<NetworkService>(_T("network service"), hdb, preparedStatements, _T("network_services"));
   LoadObjectsFromTable<VPNConnector>(_T("VPN connector"), hdb, preparedStatements, _T("vpn_connectors"));
   LoadObjectsFromTable<Cluster>(_T("cluster"), hdb, preparedStatements, _T("clusters"));
   g_idxClusterById.setStartupMode(false);
   LoadObjectsFromTable<Collector>(_T("collector"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_COLLECTOR));
   g_idxCollectorById.setStartupMode(false);
   LoadObjectsFromTable<Circuit>(_T("circuit"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_CIRCUIT));
   g_idxCircuitById.setStartupMode(false);

   // Start cache loading thread.
   // All data collection targets must be loaded at this point.
   ThreadCreate(CacheLoadingThread);

   LoadObjectsFromTable<Asset>(_T("asset"), hdb, preparedStatements, _T("assets"));
   g_idxAssetById.setStartupMode(false);
   LoadObjectsFromTable<AssetGroup>(_T("asset group"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_ASSETGROUP));

   LoadObjectsFromTable<Template>(_T("template"), hdb, preparedStatements, _T("templates"), nullptr, [](const shared_ptr<Template>& t) { t->calculateCompoundStatus(); });
   LoadObjectsFromTable<NetworkMap>(_T("network map"), hdb, preparedStatements, _T("network_maps"));
   g_idxNetMapById.setStartupMode(false);
   LoadObjectsFromTable<Container>(_T("container"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_CONTAINER));
   LoadObjectsFromTable<TemplateGroup>(_T("template group"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_TEMPLATEGROUP));
   LoadObjectsFromTable<NetworkMapGroup>(_T("map group"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_NETWORKMAPGROUP));
   LoadObjectsFromTable<Dashboard>(_T("dashboard"), hdb, preparedStatements, _T("dashboards"));
   LoadObjectsFromTable<DashboardTemplate>(_T("dashboard templates"), hdb, preparedStatements, _T("dashboard_templates"));
   LoadObjectsFromTable<DashboardGroup>(_T("dashboard group"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_DASHBOARDGROUP));
   LoadObjectsFromTable<BusinessService>(_T("business service"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_BUSINESSSERVICE));
   LoadObjectsFromTable<BusinessServicePrototype>(_T("business service prototype"), hdb, preparedStatements, _T("object_containers WHERE object_class=") AS_STRING(OBJECT_BUSINESSSERVICEPROTO));

   g_idxBusinessServicesById.setStartupMode(false);
   g_idxObjectById.setStartupMode(false);

   // Free prepared statements used during object load
   for(int i = 0; i < LSI_MAX_VALUE; i++)
      DBFreeStatement(preparedStatements[i]);

	// Load custom object classes provided by modules
   CALL_ALL_MODULES(pfLoadObjects, ());

   // Execute post-load hooks on objects
   nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Executing post-load object hooks..."));
	g_idxObjectById.forEach(
	   [] (NetObj *object) -> EnumerationCallbackResult
	   {
	      object->postLoad();
	      return _CONTINUE;
	   });

	// Link custom object classes provided by modules
   CALL_ALL_MODULES(pfLinkObjects, ());

   // Allow objects to change it's modification flag
   g_modificationsLocked = false;

   // Prune custom attributes if required
   if (MetaDataReadInt32(_T("PruneCustomAttributes"), 0) > 0)
   {
      g_idxObjectById.forEach(
         [] (NetObj *object) -> EnumerationCallbackResult
         {
            object->pruneCustomAttributes();
            return _CONTINUE;
         });
      DBQuery(mainDB, _T("DELETE FROM metadata WHERE var_name='PruneCustomAttributes'"));
   }
   DBConnectionPoolReleaseConnection(mainDB);

   if (cachedb != nullptr)
      DBCloseInMemoryDatabase(cachedb);

   // Recalculate status for built-in objects
   g_entireNetwork->calculateCompoundStatus();
   g_infrastructureServiceRoot->calculateCompoundStatus();
   g_templateRoot->calculateCompoundStatus();
   g_mapRoot->calculateCompoundStatus();
   g_businessServiceRoot->calculateCompoundStatus();

   // Recalculate status for zone objects
   if (g_flags & AF_ENABLE_ZONING)
   {
		g_idxZoneByUIN.forEach(
		   [] (NetObj *object) -> EnumerationCallbackResult
		   {
		      object->calculateCompoundStatus();
		      return _CONTINUE;
		   });
   }

   // Start template update applying thread
   s_applyTemplateThread = ThreadCreateEx(ApplyTemplateThread);

   // Expand comments macros
   nxlog_debug_tag(_T("obj.comments"), 2, _T("Updating all objects comments macros"));
   g_idxObjectById.forEach(
      [] (NetObj *object) -> EnumerationCallbackResult
      {
         object->expandCommentMacros();
         return _CONTINUE;
      });

   // check links between assets and other objects
   g_idxAssetById.forEach(
      [] (NetObj *object) -> EnumerationCallbackResult
      {
         Asset *asset = static_cast<Asset*>(object);
         if (asset->getLinkedObjectId() != 0)
         {
             shared_ptr<NetObj> linkedObject = g_idxNodeById.get(asset->getLinkedObjectId());
             if (linkedObject == nullptr)
             {
                nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Link between asset \"%s\" [%u] and non-existing object [%u] deleted"),
                   asset->getName(), asset->getId(), asset->getLinkedObjectId());
                asset->setLinkedObjectId(0);
             }
             else if (asset->getId() != linkedObject->getAssetId())
             {
                nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Invalid link between asset \"%s\" [%u] and object \"%s\" [%u] deleted (asset ID mismatch: [%u], expected [%u])"),
                   asset->getName(), asset->getId(), linkedObject->getName(), linkedObject->getId(), linkedObject->getAssetId(), asset->getId());
                asset->setLinkedObjectId(0);
             }
          }
         return _CONTINUE;
       });
   g_idxObjectById.forEach(
      [] (NetObj *object) -> EnumerationCallbackResult
      {
         if (object->getAssetId() != 0)
         {
            shared_ptr<Asset> asset = static_pointer_cast<Asset>(g_idxAssetById.get(object->getAssetId()));
            if (asset == nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Link between object \"%s\" [%u] and non-existing asset [%u] deleted"), object->getName(), object->getId(), object->getAssetId());
               object->setAssetId(0);
            }
            else if (asset->getLinkedObjectId() != object->getId())
            {
               nxlog_debug_tag(DEBUG_TAG_OBJECT_INIT, 2, _T("Invalid link between object \"%s\" [%u] and asset \"%s\" [%u] deleted (linked object ID mismatch: [%u], expected [%u])"),
                  object->getName(), object->getId(), asset->getName(), asset->getId(), asset->getLinkedObjectId(), object->getId());
               object->setAssetId(0);
            }
         }
         return _CONTINUE;
      });

   return true;
}

/**
 * Cleanup objects before shutdown (only needed to eliminate false positives by leak detector)
 */
void CleanupObjects()
{
   g_idxObjectById.forEach(
      [] (NetObj *object) -> EnumerationCallbackResult
      {
         object->cleanup();
         return _CONTINUE;
      });
}

/**
 * Stop object maintenance threads
 */
void StopObjectMaintenanceThreads()
{
   g_templateUpdateQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_applyTemplateThread);
}

/**
 * Delete user or group from all objects' ACLs
 */
void DeleteUserFromAllObjects(uint32_t userId)
{
	g_idxObjectById.forEach(
	   [userId] (NetObj *object) -> EnumerationCallbackResult
	   {
	      object->dropUserAccess(userId);
	      return _CONTINUE;
	   });
}

/**
 * Print object information
 */
static void PrintObjectInfo(ServerConsole *console, uint32_t objectId, const TCHAR *prefix)
{
   if (objectId == 0)
      return;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   ConsolePrintf(console, _T("%s %s [%u]\n"), prefix, (object != nullptr) ? object->getName() : _T("<unknown>"), objectId);
}

/**
 * Print ICMP statistic for node's child object
 */
static void PrintInterfaceIcmpStatistic(ServerConsole *console, const Interface& iface)
{
   auto parentNode = iface.getParentNode();
   if (parentNode == nullptr)
      return;

   wchar_t target[MAX_OBJECT_NAME + 2];
   _sntprintf(target, MAX_OBJECT_NAME + 2, _T("N:%s"), iface.getName());
   uint32_t last, min, max, avg, loss, jitter;
   if (parentNode->getIcmpStatistics(target, &last, &min, &max, &avg, &loss, &jitter))
   {
      ConsolePrintf(console, _T("   ICMP statistics:\n"));
      ConsolePrintf(console, _T("      RTT last.........: %u ms\n"), last);
      ConsolePrintf(console, _T("      RTT min..........: %u ms\n"), min);
      ConsolePrintf(console, _T("      RTT max..........: %u ms\n"), max);
      ConsolePrintf(console, _T("      RTT average......: %u ms\n"), avg);
      ConsolePrintf(console, _T("      Jitter...........: %u ms\n"), jitter);
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
         ConsolePrintf(console, _T("   Primary IP..........: %s\n   Primary hostname....: %s\n   Capabilities........: ") UINT64X_FMT(_T("012")) _T(" (isSNMP=%d isAgent=%d isEIP=%d isBridge=%d isRouter=%d isMgmt=%d)\n   SNMP ObjectId.......: %s\n"),
                       static_cast<const Node&>(object).getIpAddress().toString(buffer),
                       static_cast<const Node&>(object).getPrimaryHostName().cstr(),
                       static_cast<const Node&>(object).getCapabilities(),
                       static_cast<const Node&>(object).isSNMPSupported(),
                       static_cast<const Node&>(object).isNativeAgent(),
                       static_cast<const Node&>(object).isEthernetIPSupported(),
                       static_cast<const Node&>(object).isBridge(),
                       static_cast<const Node&>(object).isRouter(),
                       static_cast<const Node&>(object).isLocalManagement(),
                       static_cast<const Node&>(object).getSNMPObjectId().toString().cstr());
         PrintObjectInfo(console, object.getAssignedZoneProxyId(false), _T("   Primary zone proxy..:"));
         PrintObjectInfo(console, object.getAssignedZoneProxyId(true), _T("   Backup zone proxy...:"));
         ConsolePrintf(console, _T("   ICMP polling........: %s\n"),
                  static_cast<const Node&>(object).isIcmpStatCollectionEnabled() ? _T("ON") : _T("OFF"));
         if (static_cast<const Node&>(object).isIcmpStatCollectionEnabled())
         {
            StringList *collectors = static_cast<const Node&>(object).getIcmpStatCollectors();
            for(int i = 0; i < collectors->size(); i++)
            {
               uint32_t last, min, max, avg, loss, jitter;
               if (static_cast<const Node&>(object).getIcmpStatistics(collectors->get(i), &last, &min, &max, &avg, &loss, &jitter))
               {
                  ConsolePrintf(console, _T("   ICMP statistics (%s):\n"), collectors->get(i));
                  ConsolePrintf(console, _T("      RTT last.........: %u ms\n"), last);
                  ConsolePrintf(console, _T("      RTT min..........: %u ms\n"), min);
                  ConsolePrintf(console, _T("      RTT max..........: %u ms\n"), max);
                  ConsolePrintf(console, _T("      RTT average......: %u ms\n"), avg);
                  ConsolePrintf(console, _T("      Jitter...........: %u ms\n"), jitter);
                  ConsolePrintf(console, _T("      Packet loss......: %u\n"), loss);
               }
            }
            delete collectors;
         }
         {
            shared_ptr<NetworkPath> path = static_cast<const Node&>(object).getLastKnownNetworkPath();
            if (path != nullptr)
            {
               console->print(_T("   Last known network path:\n"));
               path->print(console, 6);
            }
         }
         break;
      case OBJECT_SUBNET:
         ConsolePrintf(console, _T("   IP address..........: %s/%d\n"),
                  static_cast<const Subnet&>(object).getIpAddress().toString(buffer),
                  static_cast<const Subnet&>(object).getIpAddress().getMaskBits());
         break;
      case OBJECT_ACCESSPOINT:
         ConsolePrintf(console, _T("   MAC address.........: %s\n"), static_cast<const AccessPoint&>(object).getMacAddress().toString(buffer));
         ConsolePrintf(console, _T("   IP address..........: %s\n"), static_cast<const AccessPoint&>(object).getIpAddress().toString(buffer));
         break;
      case OBJECT_INTERFACE:
         ConsolePrintf(console, _T("   MAC address.........: %s\n"), static_cast<const Interface&>(object).getMacAddress().toString(buffer));
         for(int n = 0; n < static_cast<const Interface&>(object).getIpAddressList()->size(); n++)
         {
            const InetAddress& a = static_cast<const Interface&>(object).getIpAddressList()->get(n);
            ConsolePrintf(console, _T("   IP address..........: %s/%d\n"), a.toString(buffer), a.getMaskBits());
         }
         ConsolePrintf(console, _T("   Interface index.....: %u\n"), static_cast<const Interface&>(object).getIfIndex());
         ConsolePrintf(console, _T("   Physical port.......: %s\n"), BooleanToString(static_cast<const Interface&>(object).isPhysicalPort()));
         if (static_cast<const Interface&>(object).isPhysicalPort())
         {
            ConsolePrintf(console, _T("   Physical location...: %s\n"),
                     static_cast<const Interface&>(object).getPhysicalLocation().toString(buffer, 256));
         }
         PrintInterfaceIcmpStatistic(console, static_cast<const Interface&>(object));
         break;
      case OBJECT_TEMPLATE:
         ConsolePrintf(console, _T("   Version.............: %d\n"), static_cast<const Template&>(object).getVersion());
         break;
      case OBJECT_ZONE:
         ConsolePrintf(console, _T("   UIN.................: %d\n"),
                  static_cast<const Zone&>(object).getUIN());
         static_cast<const Zone&>(object).dumpState(console);
         break;
      case OBJECT_ASSET:
         ConsolePrintf(console, _T("   Linked object.......: %u\n"),
                  static_cast<const Asset&>(object).getLinkedObjectId());
         static_cast<const Asset&>(object).dumpProperties(console);
         break;
      case OBJECT_NETWORKMAP:
         ConsolePrintf(console, _T("   Map type............: %d\n"), static_cast<const NetworkMap&>(object).getMapType());
         ConsolePrintf(console, _T("   Update failed.......: %s\n"), BooleanToString(static_cast<const NetworkMap&>(object).isUpdateFailed()));
         break;
   }
}

/**
 * Dump objects to debug console
 */
void DumpObjects(ServerConsole *console, const TCHAR *filter)
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
            DumpObject(console, *object);
         }
         else
         {
            console->printf(_T("Object with GUID %s not found\n"), filter);
         }
      }

      TCHAR *eptr;
      uint32_t id = _tcstol(filter, &eptr, 0);
      if ((id != 0) && (*eptr == 0))
      {
         findById = true;
         shared_ptr<NetObj> object = FindObjectById(id);
         if (object != nullptr)
         {
            DumpObject(console, *object);
         }
         else
         {
            console->printf(_T("Object with ID %u not found\n"), id);
         }
      }
   }

   if (!findById)
   {
      g_idxObjectById.forEach(
         [console, filter] (NetObj *object) -> EnumerationCallbackResult
         {
            if ((filter == nullptr) || MatchString(filter, object->getName(), false))
               DumpObject(console, *object);
            return _CONTINUE;
         });
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
         if ((childClass == OBJECT_SUBNET) && !(g_flags & AF_ENABLE_ZONING))
            return true;
			break;
      case OBJECT_SERVICEROOT:
      case OBJECT_CONTAINER:
      case OBJECT_COLLECTOR:
         if ((childClass == OBJECT_ACCESSPOINT) ||
             (childClass == OBJECT_CHASSIS) ||
             (childClass == OBJECT_CIRCUIT) ||
             (childClass == OBJECT_CLUSTER) ||
             (childClass == OBJECT_COLLECTOR) ||
             (childClass == OBJECT_CONDITION) ||
             (childClass == OBJECT_CONTAINER) ||
             (childClass == OBJECT_MOBILEDEVICE) ||
             (childClass == OBJECT_NODE) ||
             (childClass == OBJECT_RACK) ||
             (childClass == OBJECT_SENSOR) ||
             (childClass == OBJECT_SUBNET) ||
             (childClass == OBJECT_WIRELESSDOMAIN))
            return true;
         break;
      case OBJECT_CIRCUIT:
         if (childClass == OBJECT_INTERFACE)
            return true;
         break;
      case OBJECT_CHASSIS:
         if (childClass == OBJECT_NODE)
            return true;
         break;
      case OBJECT_RACK:
         if ((childClass == OBJECT_CHASSIS) ||
             (childClass == OBJECT_NODE))
            return true;
         break;
      case OBJECT_TEMPLATEROOT:
      case OBJECT_TEMPLATEGROUP:
         if ((childClass == OBJECT_TEMPLATEGROUP) || (childClass == OBJECT_TEMPLATE))
            return true;
         break;
      case OBJECT_TEMPLATE:
         if ((childClass == OBJECT_ACCESSPOINT) ||
             (childClass == OBJECT_CHASSIS) ||
             (childClass == OBJECT_CIRCUIT) ||
             (childClass == OBJECT_CLUSTER) ||
             (childClass == OBJECT_COLLECTOR) ||
             (childClass == OBJECT_NODE) ||
             (childClass == OBJECT_MOBILEDEVICE) ||
             (childClass == OBJECT_SENSOR))
            return true;
         break;
      case OBJECT_NETWORKMAPROOT:
      case OBJECT_NETWORKMAPGROUP:
         if ((childClass == OBJECT_NETWORKMAPGROUP) || (childClass == OBJECT_NETWORKMAP))
            return true;
         break;
      case OBJECT_DASHBOARDROOT:
      case OBJECT_DASHBOARDGROUP:
         if ((childClass == OBJECT_DASHBOARDGROUP) || (childClass == OBJECT_DASHBOARD) || (childClass == OBJECT_DASHBOARDTEMPLATE))
            return true;
         break;
      case OBJECT_DASHBOARD:
         if ((childClass == OBJECT_DASHBOARDGROUP) || (childClass == OBJECT_DASHBOARD))
            return true;
         break;
      case OBJECT_NODE:
         if ((childClass == OBJECT_INTERFACE) ||
             (childClass == OBJECT_NETWORKSERVICE) ||
             (childClass == OBJECT_VPNCONNECTOR))
            return true;
         break;
      case OBJECT_CLUSTER:
         if (childClass == OBJECT_NODE)
            return true;
         break;
      case OBJECT_ASSETROOT:
      case OBJECT_ASSETGROUP:
         if ((childClass == OBJECT_ASSETGROUP) || (childClass == OBJECT_ASSET))
            return true;
         break;
		case OBJECT_BUSINESSSERVICEROOT:
      case OBJECT_BUSINESSSERVICE:
			if ((childClass == OBJECT_BUSINESSSERVICE) || (childClass == OBJECT_BUSINESSSERVICEPROTO))
            return true;
         break;
      case OBJECT_WIRELESSDOMAIN:
         if ((childClass == OBJECT_ACCESSPOINT) || (childClass == OBJECT_NODE))
            return true;
         break;
      case OBJECT_ZONE:
         if ((childClass == OBJECT_SUBNET) && (g_flags & AF_ENABLE_ZONING))
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
 * Check if given class is a valid taget for asset link
 */
bool IsValidAssetLinkTargetClass(int objectClass)
{
   return (objectClass == OBJECT_ACCESSPOINT) || (objectClass == OBJECT_CHASSIS) || (objectClass == OBJECT_MOBILEDEVICE) || (objectClass == OBJECT_NODE) || (objectClass == OBJECT_RACK) || (objectClass == OBJECT_SENSOR);
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
	       (objectClass == OBJECT_CIRCUIT) ||
	       (objectClass == OBJECT_COLLECTOR) ||
          (objectClass == OBJECT_CONTAINER) ||
	       (objectClass == OBJECT_CLUSTER) ||
			 (objectClass == OBJECT_MOBILEDEVICE) ||
			 (objectClass == OBJECT_ACCESSPOINT) ||
          (objectClass == OBJECT_SENSOR);
}

/**
 * Check if object1 is parent of object2 (also indirect parent)
 */
bool NXCORE_EXPORTABLE IsParentObject(uint32_t object1, uint32_t object2)
{
   shared_ptr<NetObj> p = FindObjectById(object2);
   return (p != nullptr) ? p->isParent(object1) : false;
}

/**
 * Callback data for CreateObjectAccessSnapshot
 */
struct CreateObjectAccessSnapshot_CallbackData
{
   uint32_t userId;
   StructArray<ACL_ELEMENT> *accessList;
};

/**
 * Callback for CreateObjectAccessSnapshot
 */
static EnumerationCallbackResult CreateObjectAccessSnapshot_Callback(NetObj *object, void *arg)
{
   CreateObjectAccessSnapshot_CallbackData *data = (CreateObjectAccessSnapshot_CallbackData *)arg;
   uint32_t accessRights = object->getUserRights(data->userId);
   if (accessRights != 0)
   {
      ACL_ELEMENT e;
      e.userId = object->getId();
      e.accessRights = accessRights;
      data->accessList->add(&e);
   }
   return _CONTINUE;
}

/**
 * Create access snapshot for given user and object class
 */
bool NXCORE_EXPORTABLE CreateObjectAccessSnapshot(uint32_t userId, int objClass)
{
   ObjectIndex *index = GetObjectIndexByClass(objClass);

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
               auto e = accessList.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, e->userId);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, e->accessRights);
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
static EnumerationCallbackResult NodeDependencyCheckCallback(NetObj *object, void *context)
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

   return _CONTINUE;
}

/**
 * Get dependent nodes
 */
unique_ptr<StructArray<DependentNode>> GetNodeDependencies(uint32_t nodeId)
{
   NodeDependencyCheckData data;
   data.nodeId = nodeId;
   data.dependencies = new StructArray<DependentNode>();
   g_idxNodeById.forEach(NodeDependencyCheckCallback, &data);
   return unique_ptr<StructArray<DependentNode>>(data.dependencies);
}

/**
 * Callback for cleaning expired DCI data on node
 */
static EnumerationCallbackResult ResetPollTimers(NetObj *object, void *data)
{
   if (object->isPollable())
      object->getAsPollable()->resetPollTimers();
   return _CONTINUE;
}

/**
 * Reset poll timers for all nodes
 */
void ResetObjectPollTimers(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   nxlog_debug_tag(_T("poll.system"), 2, _T("Resetting object poll timers"));
   g_idxNodeById.forEach(ResetPollTimers, nullptr); //FIXME: maybe we can use g_idxObjectById here now?
   g_idxClusterById.forEach(ResetPollTimers, nullptr);
   g_idxMobileDeviceById.forEach(ResetPollTimers, nullptr);
   g_idxCircuitById.forEach(ResetPollTimers, nullptr);
   g_idxCollectorById.forEach(ResetPollTimers, nullptr);
   g_idxSensorById.forEach(ResetPollTimers, nullptr);
   g_idxAccessPointById.forEach(ResetPollTimers, nullptr);
   g_idxChassisById.forEach(ResetPollTimers, nullptr);
}

#if WITH_PRIVATE_EXTENSIONS || (defined(_WIN32) && !defined(WIN32_UNRESTRICTED_BUILD))

/**
 * Unmanage nodes that exceeed node limit
 */
static void UnmanageExtraNodes()
{
   if (g_idxNodeById.size() > 250)
   {
      int count = 0;
      g_idxNodeById.forEach(
         [&count](NetObj *node) -> EnumerationCallbackResult
         {
            if (node->getStatus() != STATUS_UNMANAGED)
            {
               count++;
               if (count > 250)
               {
                  node->setMgmtStatus(false);
                  nxlog_write_tag(NXLOG_WARNING, _T("licensing"), _T("Node \"%s\" [%u] set to unmanaged state because limit of number of managed nodes is exceeded"), node->getName(), node->getId());
               }
            }
            return _CONTINUE;
         });
   }

   // Schedule next run in 1 hour
   ThreadPoolScheduleRelative(g_mainThreadPool, 3600000, UnmanageExtraNodes);
}

#endif

/**
 * Check restrictions on node count
 */
void CheckNodeCountRestrictions()
{
#if WITH_PRIVATE_EXTENSIONS || (defined(_WIN32) && !defined(WIN32_UNRESTRICTED_BUILD))
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   int enterpriseEditionCount = CountLicensesForProduct(hdb, "NXEE");
#if defined(_WIN32) && !WITH_PRIVATE_EXTENSIONS
   bool unrestrictedWindowsBuild = (CountLicensesForProduct(hdb, "WINEXT") > 0);
#endif
   DBConnectionPoolReleaseConnection(hdb);

   if (enterpriseEditionCount > 0)
   {
      if (enterpriseEditionCount == 0x7fffffff)
      {
         nxlog_write_tag(NXLOG_INFO, _T("licensing"), _T("Initialized without restriction on number of managed nodes"));
         InterlockedOr64(&g_flags, AF_UNLIMITED_NODES);
      }
      else
      {
         nxlog_write_tag(NXLOG_INFO, _T("licensing"), _T("Number of managed nodes restricted to %d"), enterpriseEditionCount);
      }
      return;
   }

#if defined(_WIN32) && !WITH_PRIVATE_EXTENSIONS
   if (unrestrictedWindowsBuild)
   {
      nxlog_write_tag(NXLOG_INFO, _T("licensing"), _T("Initialized without restriction on number of managed nodes"));
      InterlockedOr64(&g_flags, AF_UNLIMITED_NODES);
      return;
   }
#endif

   nxlog_write_tag(NXLOG_INFO, _T("licensing"), _T("Number of managed nodes restricted to 250"));
   UnmanageExtraNodes();
#else
   nxlog_write_tag(NXLOG_INFO, _T("licensing"), _T("Initialized without restriction on number of managed nodes"));
   InterlockedOr64(&g_flags, AF_UNLIMITED_NODES);
#endif
}

/**
 * Get maximum allowed number of managed nodes
 */
int GetMaxAllowedNodeCount()
{
   if (g_flags & AF_UNLIMITED_NODES)
      return 0x7fffffff;

   if (!(g_flags & AF_ENTERPRISE_EDITION))
#if defined(_WIN32) && !defined(WIN32_UNRESTRICTED_BUILD)
      return 250;
#else
      return 0x7fffffff;
#endif

#if WITH_PRIVATE_EXTENSIONS
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   int count = CountLicensesForProduct(hdb, "NXEE");
   DBConnectionPoolReleaseConnection(hdb);
   return count;
#else
   return 0;
#endif
}
