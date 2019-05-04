/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Raden Solutions
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

Network NXCORE_EXPORTABLE *g_pEntireNet = NULL;
ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot = NULL;
TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot = NULL;
NetworkMapRoot NXCORE_EXPORTABLE *g_pMapRoot = NULL;
DashboardRoot NXCORE_EXPORTABLE *g_pDashboardRoot = NULL;
BusinessServiceRoot NXCORE_EXPORTABLE *g_pBusinessServiceRoot = NULL;

UINT32 NXCORE_EXPORTABLE g_dwMgmtNode = 0;

Queue g_templateUpdateQueue;

ObjectIndex g_idxObjectById;
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
      TEMPLATE_UPDATE_INFO *pInfo = (TEMPLATE_UPDATE_INFO *)g_templateUpdateQueue.getOrBlock();
      if (pInfo == INVALID_POINTER_VALUE)
         break;

		DbgPrintf(5, _T("ApplyTemplateThread: template=%d(%s) updateType=%d target=%d removeDci=%s"),
         pInfo->pTemplate->getId(), pInfo->pTemplate->getName(), pInfo->updateType, pInfo->targetId, pInfo->removeDCI ? _T("true") : _T("false"));
      BOOL bSuccess = FALSE;
      NetObj *dcTarget = FindObjectById(pInfo->targetId);
      if (dcTarget != NULL)
      {
         if (dcTarget->isDataCollectionTarget())
         {
            switch(pInfo->updateType)
            {
               case APPLY_TEMPLATE:
                  pInfo->pTemplate->applyToTarget((DataCollectionTarget *)dcTarget);
                  ((DataCollectionTarget *)dcTarget)->applyDCIChanges();
                  bSuccess = TRUE;
                  break;
               case REMOVE_TEMPLATE:
                  ((DataCollectionTarget *)dcTarget)->unbindFromTemplate(pInfo->pTemplate->getId(), pInfo->removeDCI);
                  ((DataCollectionTarget *)dcTarget)->applyDCIChanges();
                  bSuccess = TRUE;
                  break;
               default:
                  bSuccess = TRUE;
                  break;
            }
         }
      }

      if (bSuccess)
      {
			DbgPrintf(8, _T("ApplyTemplateThread: success"));
			pInfo->pTemplate->decRefCount();
         free(pInfo);
      }
      else
      {
			DbgPrintf(8, _T("ApplyTemplateThread: failed"));
         g_templateUpdateQueue.put(pInfo);    // Requeue
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
	ObjectArray<NetObj> *objects = idx->getObjects(true);
   for(int i = 0; i < objects->size(); i++)
   {
      DataCollectionTarget *t = static_cast<DataCollectionTarget*>(objects->get(i));
      t->updateDciCache();
      t->decRefCount();
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
	((NetworkMap *)object)->updateContent();
   ((NetworkMap *)object)->calculateCompoundStatus();
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
		g_idxNetMapById.forEach(UpdateMapCallback, NULL);
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
   g_pEntireNet = new Network;
   NetObjInsert(g_pEntireNet, false, false);

   // Create "Service Root" object
   g_pServiceRoot = new ServiceRoot;
   NetObjInsert(g_pServiceRoot, false, false);

   // Create "Template Root" object
   g_pTemplateRoot = new TemplateRoot;
   NetObjInsert(g_pTemplateRoot, false, false);

	// Create "Network Maps Root" object
   g_pMapRoot = new NetworkMapRoot;
   NetObjInsert(g_pMapRoot, false, false);

	// Create "Dashboard Root" object
   g_pDashboardRoot = new DashboardRoot;
   NetObjInsert(g_pDashboardRoot, false, false);

   // Create "Business Service Root" object
   g_pBusinessServiceRoot = new BusinessServiceRoot;
   NetObjInsert(g_pBusinessServiceRoot, false, false);

	DbgPrintf(1, _T("Built-in objects created"));
}

/**
 * Insert new object into network
 */
void NetObjInsert(NetObj *pObject, bool newObject, bool importedObject)
{
   if (newObject)
   {
      // Assign unique ID to new object
      pObject->setId(CreateUniqueId(IDG_NETWORK_OBJECT));
      if (!importedObject && pObject->getGuid().isNull()) // imported objects already have valid GUID
         pObject->generateGuid();

      // Create tables for storing data collection values
      if (pObject->isDataCollectionTarget() && !(g_flags & AF_SINGLE_TABLE_PERF_DATA))
      {
         TCHAR szQuery[256], szQueryTemplate[256];
         UINT32 i;

         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         MetaDataReadStr(_T("IDataTableCreationCommand"), szQueryTemplate, 255, _T(""));
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId());
         DBQuery(hdb, szQuery);

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("IDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId(), pObject->getId());
               DBQuery(hdb, szQuery);
            }
         }

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("TDataTableCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId(), pObject->getId());
               DBQuery(hdb, szQuery);
            }
         }

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("TDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId(), pObject->getId());
               DBQuery(hdb, szQuery);
            }
         }

         DBConnectionPoolReleaseConnection(hdb);
		}
   }
	g_idxObjectById.put(pObject->getId(), pObject);
   if (!pObject->isDeleted())
   {
      switch(pObject->getObjectClass())
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
         case OBJECT_AGENTPOLICY_LOGPARSER:
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
				g_idxNodeById.put(pObject->getId(), pObject);
            if (!(((Node *)pObject)->getFlags() & NF_REMOTE_AGENT))
            {
			      if (IsZoningEnabled())
			      {
				      Zone *zone = (Zone *)g_idxZoneByUIN.get(((Node *)pObject)->getZoneUIN());
				      if (zone != NULL)
				      {
					      zone->addToIndex((Node *)pObject);
				      }
				      else
				      {
					      DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"),
					                (int)((Node *)pObject)->getZoneUIN(), pObject->getName(), (int)pObject->getId());
				      }
               }
               else
               {
                  if (((Node *)pObject)->getIpAddress().isValidUnicast())
					      g_idxNodeByAddr.put(((Node *)pObject)->getIpAddress(), pObject);
               }
            }
            break;
			case OBJECT_CLUSTER:
            g_idxClusterById.put(pObject->getId(), pObject);
            break;
			case OBJECT_MOBILEDEVICE:
				g_idxMobileDeviceById.put(pObject->getId(), pObject);
            break;
			case OBJECT_ACCESSPOINT:
				g_idxAccessPointById.put(pObject->getId(), pObject);
            MacDbAddAccessPoint((AccessPoint *)pObject);
            break;
         case OBJECT_CHASSIS:
            g_idxChassisById.put(pObject->getId(), pObject);
            break;
         case OBJECT_SUBNET:
            g_idxSubnetById.put(pObject->getId(), pObject);
            if (((Subnet *)pObject)->getIpAddress().isValidUnicast())
            {
					if (IsZoningEnabled())
					{
						Zone *zone = (Zone *)g_idxZoneByUIN.get(((Subnet *)pObject)->getZoneUIN());
						if (zone != NULL)
						{
							zone->addToIndex((Subnet *)pObject);
						}
						else
						{
							DbgPrintf(2, _T("Cannot find zone object with GUID=%d for subnet object %s [%d]"),
							          (int)((Subnet *)pObject)->getZoneUIN(), pObject->getName(), (int)pObject->getId());
						}
					}
					else
					{
						g_idxSubnetByAddr.put(((Subnet *)pObject)->getIpAddress(), pObject);
					}
               if (newObject)
               {
                  PostEvent(EVENT_SUBNET_ADDED, g_dwMgmtNode, "isAd", pObject->getId(), pObject->getName(),
                     &((Subnet *)pObject)->getIpAddress(), ((Subnet *)pObject)->getIpAddress().getMaskBits());
               }
            }
            break;
         case OBJECT_INTERFACE:
            if (!((Interface *)pObject)->isExcludedFromTopology())
            {
					if (IsZoningEnabled())
					{
						Zone *zone = (Zone *)g_idxZoneByUIN.get(((Interface *)pObject)->getZoneUIN());
						if (zone != NULL)
						{
							zone->addToIndex((Interface *)pObject);
						}
						else
						{
							DbgPrintf(2, _T("Cannot find zone object with GUID=%d for interface object %s [%d]"),
							          (int)((Interface *)pObject)->getZoneUIN(), pObject->getName(), (int)pObject->getId());
						}
					}
					else
					{
						g_idxInterfaceByAddr.put(((Interface *)pObject)->getIpAddressList(), pObject);
					}
            }
            MacDbAddInterface((Interface *)pObject);
            break;
         case OBJECT_ZONE:
				g_idxZoneByUIN.put(((Zone *)pObject)->getUIN(), pObject);
            break;
         case OBJECT_CONDITION:
				g_idxConditionById.put(pObject->getId(), pObject);
            break;
			case OBJECT_SLMCHECK:
				g_idxServiceCheckById.put(pObject->getId(), pObject);
            break;
			case OBJECT_NETWORKMAP:
				g_idxNetMapById.put(pObject->getId(), pObject);
            break;
			case OBJECT_SENSOR:
				g_idxSensorById.put(pObject->getId(), pObject);
            break;
         default:
				{
					bool processed = false;
					for(UINT32 i = 0; i < g_dwNumModules; i++)
					{
						if (g_pModuleList[i].pfNetObjInsert != NULL)
						{
							if (g_pModuleList[i].pfNetObjInsert(pObject))
								processed = true;
						}
					}
					if (!processed)
						nxlog_write(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->getObjectClass());
				}
            break;
      }
   }

	// Notify modules about object creation
	if (newObject)
	{
      CALL_ALL_MODULES(pfPostObjectCreate, (pObject));
      pObject->executeHookScript(_T("PostObjectCreate"));
	}
   else
   {
      CALL_ALL_MODULES(pfPostObjectLoad, (pObject));
   }
}

/**
 * Delete object from indexes
 * If object has an IP address, this function will delete it from
 * appropriate index. Normally this function should be called from
 * NetObj::deleteObject() method.
 */
void NetObjDeleteFromIndexes(NetObj *pObject)
{
   switch(pObject->getObjectClass())
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
			g_idxNodeById.remove(pObject->getId());
         if (!(((Node *)pObject)->getFlags() & NF_REMOTE_AGENT))
         {
			   if (IsZoningEnabled())
			   {
				   Zone *zone = (Zone *)g_idxZoneByUIN.get(((Node *)pObject)->getZoneUIN());
				   if (zone != NULL)
				   {
					   zone->removeFromIndex((Node *)pObject);
				   }
				   else
				   {
					   DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"),
					             (int)((Node *)pObject)->getZoneUIN(), pObject->getName(), (int)pObject->getId());
				   }
            }
            else
            {
			      if (((Node *)pObject)->getIpAddress().isValidUnicast())
				      g_idxNodeByAddr.remove(((Node *)pObject)->getIpAddress());
            }
         }
         break;
		case OBJECT_CLUSTER:
			g_idxClusterById.remove(pObject->getId());
         break;
      case OBJECT_MOBILEDEVICE:
			g_idxMobileDeviceById.remove(pObject->getId());
         break;
		case OBJECT_ACCESSPOINT:
			g_idxAccessPointById.remove(pObject->getId());
         MacDbRemove(((AccessPoint *)pObject)->getMacAddr());
         break;
		case OBJECT_CHASSIS:
         g_idxChassisById.remove(pObject->getId());
         break;
      case OBJECT_SUBNET:
         g_idxSubnetById.remove(pObject->getId());
         if (((Subnet *)pObject)->getIpAddress().isValidUnicast())
         {
				if (IsZoningEnabled())
				{
					Zone *zone = (Zone *)g_idxZoneByUIN.get(((Subnet *)pObject)->getZoneUIN());
					if (zone != NULL)
					{
						zone->removeFromIndex((Subnet *)pObject);
					}
					else
					{
						DbgPrintf(2, _T("Cannot find zone object with GUID=%d for subnet object %s [%d]"),
						          (int)((Subnet *)pObject)->getZoneUIN(), pObject->getName(), (int)pObject->getId());
					}
				}
				else
				{
					g_idxSubnetByAddr.remove(((Subnet *)pObject)->getIpAddress());
				}
         }
         break;
      case OBJECT_INTERFACE:
			if (IsZoningEnabled())
			{
				Zone *zone = (Zone *)g_idxZoneByUIN.get(((Interface *)pObject)->getZoneUIN());
				if (zone != NULL)
				{
					zone->removeFromIndex((Interface *)pObject);
				}
				else
				{
					DbgPrintf(2, _T("Cannot find zone object with GUID=%d for interface object %s [%d]"),
					          (int)((Interface *)pObject)->getZoneUIN(), pObject->getName(), (int)pObject->getId());
				}
			}
			else
			{
            const ObjectArray<InetAddress> *list = ((Interface *)pObject)->getIpAddressList()->getList();
            for(int i = 0; i < list->size(); i++)
            {
               InetAddress *addr = list->get(i);
               if (addr->isValidUnicast())
               {
				      NetObj *o = g_idxInterfaceByAddr.get(*addr);
				      if ((o != NULL) && (o->getId() == pObject->getId()))
				      {
					      g_idxInterfaceByAddr.remove(*addr);
				      }
               }
            }
			}
         MacDbRemove(((Interface *)pObject)->getMacAddr());
         break;
      case OBJECT_ZONE:
			g_idxZoneByUIN.remove(((Zone *)pObject)->getUIN());
         break;
      case OBJECT_CONDITION:
			g_idxConditionById.remove(pObject->getId());
         break;
      case OBJECT_SLMCHECK:
			g_idxServiceCheckById.remove(pObject->getId());
         break;
		case OBJECT_NETWORKMAP:
			g_idxNetMapById.remove(pObject->getId());
         break;
		case OBJECT_SENSOR:
			g_idxSensorById.remove(pObject->getId());
         break;
      default:
			{
				bool processed = false;
				for(UINT32 i = 0; i < g_dwNumModules; i++)
				{
					if (g_pModuleList[i].pfNetObjDelete != NULL)
					{
						if (g_pModuleList[i].pfNetObjDelete(pObject))
							processed = true;
					}
				}
				if (!processed)
					nxlog_write(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->getObjectClass());
			}
         break;
   }
}

/**
 * Find access point by MAC address
 */
AccessPoint NXCORE_EXPORTABLE *FindAccessPointByMAC(const BYTE *macAddr)
{
	if (!memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", 6))
		return NULL;

	NetObj *object = MacDbFind(macAddr);
   return ((object != NULL) && (object->getObjectClass() == OBJECT_ACCESSPOINT)) ? (AccessPoint *)object : NULL;
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
MobileDevice NXCORE_EXPORTABLE *FindMobileDeviceByDeviceID(const TCHAR *deviceId)
{
	if ((deviceId == NULL) || (*deviceId == 0))
		return NULL;

	return (MobileDevice *)g_idxMobileDeviceById.find(DeviceIdComparator, (void *)deviceId);
}

static Node *FindNodeByIPInternal(UINT32 zoneUIN, const InetAddress& ipAddr)
{
   Zone *zone = IsZoningEnabled() ? (Zone *)g_idxZoneByUIN.get(zoneUIN) : NULL;

   Node *node = NULL;
   if (IsZoningEnabled())
   {
      if (zone != NULL)
      {
         node = zone->getNodeByAddr(ipAddr);
      }
   }
   else
   {
      node = (Node *)g_idxNodeByAddr.get(ipAddr);
   }
   if (node != NULL)
      return node;

   Interface *iface = NULL;
   if (IsZoningEnabled())
   {
      if (zone != NULL)
      {
         iface = zone->getInterfaceByAddr(ipAddr);
      }
   }
   else
   {
      iface = (Interface *)g_idxInterfaceByAddr.get(ipAddr);
   }
   return (iface != NULL) ? iface->getParentNode() : NULL;
}

/**
 * Data for node find callback
 */
struct NodeFindCBData
{
   const InetAddress *addr;
   Node *node;
};

/**
 * Callback for finding node in all zones
 */
static bool NodeFindCB(NetObj *zone, void *data)
{
   Node *node = ((Zone *)zone)->getNodeByAddr(*((NodeFindCBData *)data)->addr);
   if (node == NULL)
   {
      Interface *iface = ((Zone *)zone)->getInterfaceByAddr(*((NodeFindCBData *)data)->addr);
      if (iface != NULL)
         node = iface->getParentNode();
   }

   if (node == NULL)
      return false;

   ((NodeFindCBData *)data)->node = node;
   return true;
}

/**
 * Find node by IP address
 */
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return NULL;

   if ((zoneUIN == ALL_ZONES) && IsZoningEnabled())
   {
      NodeFindCBData data;
      data.addr = &ipAddr;
      data.node = NULL;
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
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, bool allZones, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return NULL;

   Node *node = FindNodeByIPInternal(zoneUIN, ipAddr);
   if (node != NULL)
      return node;
   return allZones ? FindNodeByIP(ALL_ZONES, ipAddr) : NULL;
}

/**
 * Find node by IP address using first match from IP address list
 */
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, const InetAddressList *ipAddrList)
{
   for(int i = 0; i < ipAddrList->size(); i++)
   {
      Node *node = FindNodeByIP(zoneUIN, ipAddrList->get(i));
      if (node != NULL)
         return node;
   }
   return NULL;
}

/**
 * Find interface by IP address
 */
Interface NXCORE_EXPORTABLE *FindInterfaceByIP(UINT32 zoneUIN, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return NULL;

	Interface *iface = NULL;
	if (IsZoningEnabled())
	{
	   Zone *zone = (Zone *)g_idxZoneByUIN.get(zoneUIN);
		if (zone != NULL)
		{
			iface = zone->getInterfaceByAddr(ipAddr);
		}
	}
	else
	{
		iface = (Interface *)g_idxInterfaceByAddr.get(ipAddr);
	}
	return iface;
}

/**
 * Find node by MAC address
 */
Node NXCORE_EXPORTABLE *FindNodeByMAC(const BYTE *macAddr)
{
	Interface *pInterface = FindInterfaceByMAC(macAddr);
	return (pInterface != NULL) ? pInterface->getParentNode() : NULL;
}

/**
 * Find interface by MAC address
 */
Interface NXCORE_EXPORTABLE *FindInterfaceByMAC(const BYTE *macAddr)
{
	if (!memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", 6))
		return NULL;

	NetObj *object = MacDbFind(macAddr);
   return ((object != NULL) && (object->getObjectClass() == OBJECT_INTERFACE)) ? (Interface *)object : NULL;
}

struct NodeFindHostnameData
{
   TCHAR *hostname;
   UINT32 zoneUIN;
};

/**
 * Interface description comparator
 */
static bool HostnameComparator(NetObj *object, void *data)
{
   TCHAR primaryName[MAX_DNS_NAME];
   if ((object->getObjectClass() == OBJECT_NODE) && !object->isDeleted())
   {
      _tcscpy(primaryName, ((Node *)object)->getPrimaryName());
      _tcsupr(primaryName);
      _tcsupr(((NodeFindHostnameData *)data)->hostname);
   }
   else
      return false;

   return ((_tcsstr(primaryName, ((NodeFindHostnameData *)data)->hostname) != NULL) &&
            (IsZoningEnabled() ? (((Node *)object)->getZoneUIN() == ((NodeFindHostnameData *)data)->zoneUIN) : true));
}

/**
 * Find a list of nodes that contain the hostname
 */
ObjectArray<NetObj> *FindNodesByHostname(TCHAR *hostname, UINT32 zoneUIN)
{
   NodeFindHostnameData data;
   data.hostname = hostname;
   data.zoneUIN = zoneUIN;

   ObjectArray<NetObj> *nodes = g_idxNodeById.findObjects(HostnameComparator, &data);
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
Interface NXCORE_EXPORTABLE *FindInterfaceByDescription(const TCHAR *description)
{
	return (Interface *)g_idxObjectById.find(DescriptionComparator, (void *)description);
}

/**
 * LLDP ID comparator
 */
static bool LldpIdComparator(NetObj *object, void *lldpId)
{
	const TCHAR *id = ((Node *)object)->getLLDPNodeId();
	return (id != NULL) && !_tcscmp(id, (const TCHAR *)lldpId);
}

/**
 * Find node by LLDP ID
 */
Node NXCORE_EXPORTABLE *FindNodeByLLDPId(const TCHAR *lldpId)
{
	return (Node *)g_idxNodeById.find(LldpIdComparator, (void *)lldpId);
}

/**
 * SNMP sysName comparator
 */
static bool SysNameComparator(NetObj *object, void *sysName)
{
   const TCHAR *n = ((Node *)object)->getSysName();
   return (n != NULL) && !_tcscmp(n, (const TCHAR *)sysName);
}

/**
 * Find node by SNMP sysName
 */
Node NXCORE_EXPORTABLE *FindNodeBySysName(const TCHAR *sysName)
{
   if ((sysName == NULL) || (sysName[0] == 0))
      return NULL;

   // return NULL if multiple nodes with same sysName found
   ObjectArray<NetObj> *objects = g_idxNodeById.findObjects(SysNameComparator, (void *)sysName);
   Node *node = (objects->size() == 1) ? (Node *)objects->get(0) : NULL;
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
Node NXCORE_EXPORTABLE *FindNodeByBridgeId(const BYTE *bridgeId)
{
	return (Node *)g_idxNodeById.find(BridgeIdComparator, (void *)bridgeId);
}

/**
 * Find subnet by IP address
 */
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(UINT32 zoneUIN, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return NULL;

	Subnet *subnet = NULL;
	if (IsZoningEnabled())
	{
		Zone *zone = (Zone *)g_idxZoneByUIN.get(zoneUIN);
		if (zone != NULL)
		{
			subnet = zone->getSubnetByAddr(ipAddr);
		}
	}
	else
	{
		subnet = (Subnet *)g_idxSubnetByAddr.get(ipAddr);
	}
	return subnet;
}

/**
 * Subnet matching data
 */
struct SUBNET_MATCHING_DATA
{
   InetAddress ipAddr; // IP address to find subnet for
   int maskLen;        // Current match mask length
   Subnet *subnet;     // search result
};

/**
 * Subnet matching callback
 */
static void SubnetMatchCallback(const InetAddress& addr, NetObj *object, void *arg)
{
   SUBNET_MATCHING_DATA *data = (SUBNET_MATCHING_DATA *)arg;
   if (((Subnet *)object)->getIpAddress().contain(data->ipAddr))
   {
      int maskLen = ((Subnet *)object)->getIpAddress().getMaskBits();
      if (maskLen > data->maskLen)
      {
         data->maskLen = maskLen;
         data->subnet = (Subnet *)object;
      }
   }
}

/**
 * Find subnet for given IP address
 */
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(UINT32 zoneUIN, const InetAddress& nodeAddr)
{
   if (!nodeAddr.isValidUnicast())
      return NULL;

   SUBNET_MATCHING_DATA matchData;
   matchData.ipAddr = nodeAddr;
   matchData.maskLen = -1;
   matchData.subnet = NULL;
	if (IsZoningEnabled())
	{
		Zone *zone = (Zone *)g_idxZoneByUIN.get(zoneUIN);
		if (zone != NULL)
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
bool AdjustSubnetBaseAddress(InetAddress& baseAddr, UINT32 zoneUIN)
{
   InetAddress addr = baseAddr.getSubnetAddress();
   while(FindSubnetByIP(zoneUIN, addr) != NULL)
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
NetObj NXCORE_EXPORTABLE *FindObjectById(UINT32 dwId, int objClass)
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

   NetObj *object = index->get(dwId);
	if ((object == NULL) || (objClass == -1))
		return object;
	return (objClass == object->getObjectClass()) ? object : NULL;
}

/**
 * Object name regex and class matching data
 */
struct OBJECT_NAME_REGEX_CLASS_DATA
{
   int objClass;
   regex_t *nameRegex;
};

/**
 * Filter for matching object name by regex and its class
 *
 * @param object
 * @param userData
 * @return
 */
static bool ObjectNameRegexAndClassFilter(NetObj *object, void *userData)
{
   OBJECT_NAME_REGEX_CLASS_DATA *data = static_cast<OBJECT_NAME_REGEX_CLASS_DATA *>(userData);
   return !object->isDeleted() && (object->getObjectClass() == data->objClass) && (_tregexec(data->nameRegex, object->getName(), 0, NULL, 0) == 0);
}

/**
 * Find objects whose name matches the regex
 * (refCounter is increased for each object)
 *
 * @param regex for matching object name
 * @param objClass
 * @return
 */
ObjectArray<NetObj> NXCORE_EXPORTABLE *FindObjectsByRegex(const TCHAR *regex, int objClass)
{
   regex_t preg;
   if (_tregcomp(&preg, regex, REG_EXTENDED | REG_NOSUB) != 0)
      return NULL;

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

   OBJECT_NAME_REGEX_CLASS_DATA data;
   data.nameRegex = &preg;
   data.objClass = objClass;

   ObjectArray<NetObj> *result = index->getObjects(true, ObjectNameRegexAndClassFilter, &data);
   regfree(&preg);
   return result;
}

/**
 * Get object name by ID
 */
const TCHAR NXCORE_EXPORTABLE *GetObjectName(DWORD id, const TCHAR *defaultName)
{
	NetObj *object = g_idxObjectById.get(id);
   return (object != NULL) ? object->getName() : defaultName;
}

/**
 * Generic object finding method
 */
NetObj NXCORE_EXPORTABLE *FindObject(bool (* comparator)(NetObj *, void *), void *userData, int objClass)
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
   NetObj *object = index->find(comparator, userData);
   return ((object == NULL) || (objClass == -1)) ? object : ((object->getObjectClass() == objClass) ? object : NULL);
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
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass)
{
	struct __find_object_by_name_data data;
	data.objClass = objClass;
	data.name = name;
	return FindObject(ObjectNameComparator, &data, objClass);
}

/**
 * Callback data for FindObjectByGUID
 */
struct __find_object_by_guid_data
{
   int objClass;
   uuid guid;
};

/**
 * GUID comparator for FindObjectByGUID
 */
static bool ObjectGuidComparator(NetObj *object, void *data)
{
   struct __find_object_by_guid_data *fd = (struct __find_object_by_guid_data *)data;
   return ((fd->objClass == -1) || (fd->objClass == object->getObjectClass())) &&
          !object->isDeleted() && object->getGuid().equals(fd->guid);
}

/**
 * Find object by GUID
 */
NetObj NXCORE_EXPORTABLE *FindObjectByGUID(const uuid& guid, int objClass)
{
   struct __find_object_by_guid_data data;
   data.objClass = objClass;
   data.guid = guid;
   return FindObject(ObjectGuidComparator, &data, objClass);
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
Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName)
{
	return (Template *)g_idxObjectById.find(TemplateNameComparator, (void *)pszName);
}

/**
 * Data structure for IsClusterIP callback
 */
struct __cluster_ip_data
{
	InetAddress ipAddr;
	UINT32 zoneUIN;
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
bool NXCORE_EXPORTABLE IsClusterIP(UINT32 zoneUIN, const InetAddress& ipAddr)
{
	struct __cluster_ip_data data;
	data.zoneUIN = zoneUIN;
	data.ipAddr = ipAddr;
	return g_idxObjectById.find(ClusterIPComparator, &data) != NULL;
}

/**
 * Find zone object by UIN (unique identification number)
 */
Zone NXCORE_EXPORTABLE *FindZoneByUIN(UINT32 uin)
{
	return static_cast<Zone*>(g_idxZoneByUIN.get(uin));
}

/**
 * Comparator for FindZoneByProxyId
 */
static bool ZoneProxyIdComparator(NetObj *object, void *data)
{
   return static_cast<Zone*>(object)->isProxyNode(*static_cast<UINT32*>(data));
}

/**
 * Find zone object by proxy node ID. Can be used to determine if given node is a proxy for any zone.
 */
Zone NXCORE_EXPORTABLE *FindZoneByProxyId(UINT32 proxyId)
{
   return static_cast<Zone*>(g_idxZoneByUIN.find(ZoneProxyIdComparator, &proxyId));
}

/**
 * Zone ID selector data
 */
static Mutex s_zoneUinSelectorLock;
static IntegerArray<UINT32> s_zoneUinSelectorHistory;

/**
 * Find unused zone UIN
 */
UINT32 FindUnusedZoneUIN()
{
   UINT32 uin = 0;
   s_zoneUinSelectorLock.lock();
   for(UINT32 i = 1; i < 0x7FFFFFFF; i++)
   {
      if (g_idxZoneByUIN.get(i) != NULL)
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
	NetObj *object = g_idxNodeById.find(LocalMgmtNodeComparator, NULL);
	return (object != NULL) ? object->getId() : 0;
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
   DB_HANDLE cachedb = (g_flags & AF_CACHE_DB_ON_STARTUP) ? DBOpenInMemoryDatabase() : NULL;
   if (cachedb != NULL)
   {
      static const TCHAR *intColumns[] = { _T("condition_id"), _T("sequence_number"), _T("dci_id"), _T("node_id"), _T("dci_func"), _T("num_pols"),
                                           _T("dashboard_id"), _T("element_id"), _T("element_type"), _T("threshold_id"), _T("item_id"),
                                           _T("check_function"), _T("check_operation"), _T("sample_count"), _T("event_code"), _T("rearm_event_code"),
                                           _T("repeat_interval"), _T("current_state"), _T("current_severity"), _T("match_count"),
                                           _T("last_event_timestamp"), _T("table_id"), _T("flags"), _T("id"), _T("activation_event"),
                                           _T("deactivation_event"), _T("group_id"), _T("iface_id"), _T("vlan_id"), NULL };

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
               DBCacheTable(cachedb, mainDB, _T("network_map_links"), NULL, _T("*")) &&
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
   g_pEntireNet->loadFromDatabase(hdb);
   g_pServiceRoot->loadFromDatabase(hdb);
   g_pTemplateRoot->loadFromDatabase(hdb);
	g_pMapRoot->loadFromDatabase(hdb);
	g_pDashboardRoot->loadFromDatabase(hdb);
	g_pBusinessServiceRoot->loadFromDatabase(hdb);

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
      Zone *pZone;

      DbgPrintf(2, _T("Loading zones..."));

      // Load (or create) default zone
      pZone = new Zone;
      pZone->generateGuid();
      pZone->loadFromDatabase(hdb, BUILTIN_OID_ZONE0);
      NetObjInsert(pZone, false, false);
      g_pEntireNet->AddZone(pZone);

      DB_RESULT hResult = DBSelect(hdb, _T("SELECT id FROM zones WHERE id<>4"));
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            UINT32 id = DBGetFieldULong(hResult, i, 0);
            pZone = new Zone;
            if (pZone->loadFromDatabase(hdb, id))
            {
               if (!pZone->isDeleted())
                  g_pEntireNet->AddZone(pZone);
               NetObjInsert(pZone, false, false);  // Insert into indexes
            }
            else     // Object load failed
            {
               pZone->destroy();
               nxlog_write(MSG_ZONE_LOAD_FAILED, NXLOG_ERROR, "d", id);
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
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         ConditionObject *condition = new ConditionObject();
         if (condition->loadFromDatabase(hdb, id))
         {
            NetObjInsert(condition, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            condition->destroy();
            nxlog_write(MSG_CONDITION_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxConditionById.setStartupMode(false);

   // Load subnets
   DbgPrintf(2, _T("Loading subnets..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM subnets"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Subnet *subnet = new Subnet;
         if (subnet->loadFromDatabase(hdb, id))
         {
            if (!subnet->isDeleted())
            {
               if (g_flags & AF_ENABLE_ZONING)
               {
                  Zone *zone = FindZoneByUIN(subnet->getZoneUIN());
                  if (zone != NULL)
                     zone->addSubnet(subnet);
               }
               else
               {
                  g_pEntireNet->AddSubnet(subnet);
               }
            }
            NetObjInsert(subnet, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            subnet->destroy();
            nxlog_write(MSG_SUBNET_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxSubnetById.setStartupMode(false);

   // Load racks
   DbgPrintf(2, _T("Loading racks..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM racks"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Rack *rack = new Rack;
         if (rack->loadFromDatabase(hdb, id))
         {
            NetObjInsert(rack, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(MSG_RACK_LOAD_FAILED, NXLOG_ERROR, "d", id);
            rack->destroy();
         }
      }
      DBFreeResult(hResult);
   }

   // Load chassis
   DbgPrintf(2, _T("Loading chassis..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM chassis"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Chassis *chassis = new Chassis;
         if (chassis->loadFromDatabase(hdb, id))
         {
            NetObjInsert(chassis, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(MSG_CHASSIS_LOAD_FAILED, NXLOG_ERROR, "d", id);
            chassis->destroy();
         }
      }
      DBFreeResult(hResult);
   }
   g_idxChassisById.setStartupMode(false);

   // Load mobile devices
   DbgPrintf(2, _T("Loading mobile devices..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM mobile_devices"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         MobileDevice *md = new MobileDevice;
         if (md->loadFromDatabase(hdb, id))
         {
            NetObjInsert(md, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            md->destroy();
            nxlog_write(MSG_MOBILEDEVICE_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxMobileDeviceById.setStartupMode(false);

   // Load sensors
   DbgPrintf(2, _T("Loading sensors..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM sensors"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Sensor *sensor = new Sensor;
         if (sensor->loadFromDatabase(hdb, id))
         {
            NetObjInsert(sensor, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            sensor->destroy();
            nxlog_write(MSG_SENSOR_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxSensorById.setStartupMode(false);

   // Load nodes
   DbgPrintf(2, _T("Loading nodes..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM nodes"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Node *node = new Node;
         if (node->loadFromDatabase(hdb, id))
         {
            NetObjInsert(node, false, false);  // Insert into indexes
            if (IsZoningEnabled())
            {
               Zone *zone = FindZoneByProxyId(id);
               if (zone != NULL)
               {
                  zone->updateProxyStatus(node, false);
               }
            }
         }
         else     // Object load failed
         {
            node->destroy();
            nxlog_write(MSG_NODE_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxNodeById.setStartupMode(false);

   // Load access points
   DbgPrintf(2, _T("Loading access points..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM access_points"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         AccessPoint *ap = new AccessPoint;
         if (ap->loadFromDatabase(hdb, id))
         {
            NetObjInsert(ap, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(MSG_AP_LOAD_FAILED, NXLOG_ERROR, "d", id);
            ap->destroy();
         }
      }
      DBFreeResult(hResult);
   }
   g_idxAccessPointById.setStartupMode(false);

   // Load interfaces
   DbgPrintf(2, _T("Loading interfaces..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM interfaces"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Interface *iface = new Interface;
         if (iface->loadFromDatabase(hdb, id))
         {
            NetObjInsert(iface, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(MSG_INTERFACE_LOAD_FAILED, NXLOG_ERROR, "d", id);
            iface->destroy();
         }
      }
      DBFreeResult(hResult);
   }

   // Load network services
   DbgPrintf(2, _T("Loading network services..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM network_services"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         NetworkService *service = new NetworkService;
         if (service->loadFromDatabase(hdb, id))
         {
            NetObjInsert(service, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            service->destroy();
            nxlog_write(MSG_NETSRV_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load VPN connectors
   DbgPrintf(2, _T("Loading VPN connectors..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM vpn_connectors"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         VPNConnector *connector = new VPNConnector;
         if (connector->loadFromDatabase(hdb, id))
         {
            NetObjInsert(connector, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            connector->destroy();
            nxlog_write(MSG_VPNC_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load clusters
   DbgPrintf(2, _T("Loading clusters..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM clusters"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Cluster *cluster = new Cluster;
         if (cluster->loadFromDatabase(hdb, id))
         {
            NetObjInsert(cluster, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            cluster->destroy();
            nxlog_write(MSG_CLUSTER_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }
   g_idxClusterById.setStartupMode(false);

   // Start cache loading thread.
   // All data collection targets must be loaded at this point.
   ThreadCreate(CacheLoadingThread, 0, NULL);

   // Load templates
   DbgPrintf(2, _T("Loading templates..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM templates"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Template *tmpl = new Template;
         if (tmpl->loadFromDatabase(hdb, id))
         {
            NetObjInsert(tmpl, false, false);  // Insert into indexes
				tmpl->calculateCompoundStatus();	// Force status change to NORMAL
         }
         else     // Object load failed
         {
            tmpl->destroy();
            nxlog_write(MSG_TEMPLATE_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load network maps
   DbgPrintf(2, _T("Loading network maps..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM network_maps"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         NetworkMap *map = new NetworkMap;
         if (map->loadFromDatabase(hdb, id))
         {
            NetObjInsert(map, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            map->destroy();
            nxlog_write(MSG_NETMAP_LOAD_FAILED, NXLOG_ERROR, "d", id);
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
   if (hResult != NULL)
   {
      Container *pContainer;

      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         pContainer = new Container;
         if (pContainer->loadFromDatabase(hdb, id))
         {
            NetObjInsert(pContainer, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            pContainer->destroy();
            nxlog_write(MSG_CONTAINER_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load template group objects
   DbgPrintf(2, _T("Loading template groups..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_TEMPLATEGROUP);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      TemplateGroup *pGroup;

      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         pGroup = new TemplateGroup;
         if (pGroup->loadFromDatabase(hdb, id))
         {
            NetObjInsert(pGroup, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            pGroup->destroy();
            nxlog_write(MSG_TG_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load map group objects
   DbgPrintf(2, _T("Loading map groups..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_NETWORKMAPGROUP);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         NetworkMapGroup *group = new NetworkMapGroup;
         if (group->loadFromDatabase(hdb, id))
         {
            NetObjInsert(group, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            group->destroy();
            nxlog_write(MSG_MG_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load dashboard objects
   DbgPrintf(2, _T("Loading dashboards..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM dashboards"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         Dashboard *dashboard = new Dashboard;
         if (dashboard->loadFromDatabase(hdb, id))
         {
            NetObjInsert(dashboard, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            dashboard->destroy();
            nxlog_write(MSG_DASHBOARD_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Load dashboard group objects
   DbgPrintf(2, _T("Loading dashboard groups..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_DASHBOARDGROUP);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         DashboardGroup *group = new DashboardGroup;
         if (group->loadFromDatabase(hdb, id))
         {
            NetObjInsert(group, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            group->destroy();
            nxlog_write(MSG_MG_LOAD_FAILED, NXLOG_ERROR, "d", id);
         }
      }
      DBFreeResult(hResult);
   }

   // Loading business service objects
   DbgPrintf(2, _T("Loading business services..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_BUSINESSSERVICE);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
	   int count = DBGetNumRows(hResult);
	   for(int i = 0; i < count; i++)
	   {
		   UINT32 id = DBGetFieldULong(hResult, i, 0);
		   BusinessService *service = new BusinessService;
		   if (service->loadFromDatabase(hdb, id))
		   {
			   NetObjInsert(service, false, false);  // Insert into indexes
		   }
		   else     // Object load failed
		   {
			   service->destroy();
			   nxlog_write(MSG_BUSINESS_SERVICE_LOAD_FAILED, NXLOG_ERROR, "d", id);
		   }
	   }
	   DBFreeResult(hResult);
   }

   // Loading business service objects
   DbgPrintf(2, _T("Loading node links..."));
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT id FROM object_containers WHERE object_class=%d"), OBJECT_NODELINK);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
	   int count = DBGetNumRows(hResult);
	   for(int i = 0; i < count; i++)
	   {
		   UINT32 id = DBGetFieldULong(hResult, i, 0);
		   NodeLink *nl = new NodeLink;
		   if (nl->loadFromDatabase(hdb, id))
		   {
			   NetObjInsert(nl, false, false);  // Insert into indexes
		   }
		   else     // Object load failed
		   {
			   nl->destroy();
			   nxlog_write(MSG_NODE_LINK_LOAD_FAILED, NXLOG_ERROR, "d", id);
		   }
	   }
	   DBFreeResult(hResult);
   }

   // Load service check objects
   DbgPrintf(2, _T("Loading service checks..."));
   hResult = DBSelect(hdb, _T("SELECT id FROM slm_checks"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         SlmCheck *check = new SlmCheck;
         if (check->loadFromDatabase(hdb, id))
         {
            NetObjInsert(check, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            check->destroy();
            nxlog_write(MSG_SERVICE_CHECK_LOAD_FAILED, NXLOG_ERROR, "d", id);
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
	g_idxObjectById.forEach(LinkObjects, NULL);

	// Link custom object classes provided by modules
   CALL_ALL_MODULES(pfLinkObjects, ());

   // Allow objects to change it's modification flag
   g_bModificationsLocked = FALSE;

   // Recalculate status for built-in objects
   g_pEntireNet->calculateCompoundStatus();
   g_pServiceRoot->calculateCompoundStatus();
   g_pTemplateRoot->calculateCompoundStatus();
   g_pMapRoot->calculateCompoundStatus();
   g_pBusinessServiceRoot->calculateCompoundStatus();

   // Recalculate status for zone objects
   if (g_flags & AF_ENABLE_ZONING)
   {
		g_idxZoneByUIN.forEach(RecalcStatusCallback, NULL);
   }

   // Start map update thread
   s_mapUpdateThread = ThreadCreateEx(MapUpdateThread, 0, NULL);

   // Start template update applying thread
   s_applyTemplateThread = ThreadCreateEx(ApplyTemplateThread, 0, NULL);

   if (cachedb != NULL)
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
	TCHAR *buffer;
   const TCHAR *filter;
};

/**
 * Print object information
 */
static void PrintObjectInfo(ServerConsole *console, UINT32 objectId, const TCHAR *prefix)
{
   if (objectId == 0)
      return;

   NetObj *object = FindObjectById(objectId);
   ConsolePrintf(console, _T("%s %s [%u]\n"), prefix, (object != NULL) ? object->getName() : _T("<unknown>"), objectId);
}

/**
 * Enumeration callback for DumpObjects
 */
static void DumpObjectCallback(NetObj *object, void *data)
{
	struct __dump_objects_data *dd = (struct __dump_objects_data *)data;

   // Apply name filter
   if ((dd->filter != NULL) && !MatchString(dd->filter, object->getName(), false))
      return;

	CONSOLE_CTX console = dd->console;

	ConsolePrintf(console, _T("\x1b[1mObject ID %d \"%s\"\x1b[0m\n")
                       _T("   Class=%s  Status=%s  IsModified=%d  IsDeleted=%d  RefCount=%d\n"),
					  object->getId(), object->getName(), object->getObjectClassName(),
                 GetStatusAsText(object->getStatus(), true),
                 object->isModified(), object->isDeleted(), object->getRefCount());
   ConsolePrintf(console, _T("   Parents: <%s>\n   Children: <%s>\n"),
                 object->dbgGetParentList(dd->buffer), object->dbgGetChildList(&dd->buffer[4096]));
	time_t t = object->getTimeStamp();
#if HAVE_LOCALTIME_R
	struct tm tmbuffer;
   struct tm *ltm = localtime_r(&t, &tmbuffer);
#else
	struct tm *ltm = localtime(&t);
#endif
	_tcsftime(dd->buffer, 256, _T("%d.%b.%Y %H:%M:%S"), ltm);
   ConsolePrintf(console, _T("   Last change.........: %s\n"), dd->buffer);
   if (object->isDataCollectionTarget())
      ConsolePrintf(console, _T("   State flags.........: 0x%08x\n"), ((DataCollectionTarget *)object)->getState());
   switch(object->getObjectClass())
   {
      case OBJECT_NODE:
         ConsolePrintf(console, _T("   Primary IP..........: %s\n   Primary hostname....: %s\n   Capabilities........: isSNMP=%d isAgent=%d isLocalMgmt=%d\n   SNMP ObjectId.......: %s\n"),
                       ((Node *)object)->getIpAddress().toString(dd->buffer),
                       ((Node *)object)->getPrimaryName(),
                       ((Node *)object)->isSNMPSupported(),
                       ((Node *)object)->isNativeAgent(),
                       ((Node *)object)->isLocalManagement(),
                       ((Node *)object)->getSNMPObjectId());
         PrintObjectInfo(console, object->getAssignedZoneProxyId(false), _T("   Primary zone proxy..:"));
         PrintObjectInfo(console, object->getAssignedZoneProxyId(true), _T("   Backup zone proxy...:"));
         break;
      case OBJECT_SUBNET:
         ConsolePrintf(console, _T("   IP address..........: %s/%d\n"),
                  static_cast<Subnet*>(object)->getIpAddress().toString(dd->buffer),
                  static_cast<Subnet*>(object)->getIpAddress().getMaskBits());
         break;
      case OBJECT_ACCESSPOINT:
         ConsolePrintf(console, _T("   IP address..........: %s\n"), ((AccessPoint *)object)->getIpAddress().toString(dd->buffer));
         break;
      case OBJECT_INTERFACE:
         ConsolePrintf(console, _T("   MAC address.........: %s\n"), MACToStr(((Interface *)object)->getMacAddr(), dd->buffer));
         for(int n = 0; n < ((Interface *)object)->getIpAddressList()->size(); n++)
         {
            const InetAddress& a = ((Interface *)object)->getIpAddressList()->get(n);
            ConsolePrintf(console, _T("   IP address..........: %s/%d\n"), a.toString(dd->buffer), a.getMaskBits());
         }
         break;
      case OBJECT_TEMPLATE:
         ConsolePrintf(console, _T("   Version.............: %d\n"),
                  static_cast<Template*>(object)->getVersion());
         break;
      case OBJECT_ZONE:
         ConsolePrintf(console, _T("   UIN.................: %d\n"),
                  static_cast<Zone*>(object)->getUIN());
         static_cast<Zone*>(object)->dumpState(console);
         break;
   }
}

/**
 * Dump objects to debug console
 */
void DumpObjects(CONSOLE_CTX pCtx, const TCHAR *filter)
{
	struct __dump_objects_data data;

   data.buffer = (TCHAR *)malloc(128000 * sizeof(TCHAR));
	data.console = pCtx;
   data.filter = filter;
	g_idxObjectById.forEach(DumpObjectCallback, &data);
	free(data.buffer);
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
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfIsValidParentClass != NULL)
		{
			if (g_pModuleList[i].pfIsValidParentClass(childClass, parentClass))
				return true;	// accepted by module
		}
	}

   return false;
}

/**
 * Callback for postponed object deletion
 */
static void DeleteObjectCallback(void *arg)
{
   NetObj *object = (NetObj *)arg;
   while(object->getRefCount() > 0)
      ThreadSleep(1);
   DbgPrintf(4, _T("Executing postponed delete of object %s [%d]"), object->getName(), object->getId());
   delete object;
}

/**
 * Delete object (final step)
 * This function should be called ONLY from syncer thread
 * Object will be removed from index by ID and destroyed.
 */
void NetObjDelete(NetObj *object)
{
	DbgPrintf(4, _T("Final delete step for object %s [%d]"), object->getName(), object->getId());

   // Delete object from index by ID and object itself
	g_idxObjectById.remove(object->getId());
	if (object->getRefCount() == 0)
	{
	   delete object;
	}
	else
	{
	   DbgPrintf(4, _T("Object %s [%d] has %d references at final delete step - postpone deletion"), object->getName(), object->getId());
	   ThreadPoolExecuteSerialized(g_mainThreadPool, _T("DeleteObject"), DeleteObjectCallback, object);
	}
}

/**
 * Update interface index when IP address changes
 */
void UpdateInterfaceIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, Interface *iface)
{
	if (IsZoningEnabled())
	{
		Zone *zone = (Zone *)g_idxZoneByUIN.get(iface->getZoneUIN());
		if (zone != NULL)
		{
			zone->updateInterfaceIndex(oldIpAddr, newIpAddr, iface);
		}
		else
		{
			DbgPrintf(1, _T("UpdateInterfaceIndex: Cannot find zone object for interface %s [%d] (zone id %d)"),
			          iface->getName(), (int)iface->getId(), (int)iface->getZoneUIN());
		}
	}
	else
	{
		g_idxInterfaceByAddr.remove(oldIpAddr);
		g_idxInterfaceByAddr.put(newIpAddr, iface);
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
bool NXCORE_EXPORTABLE IsParentObject(UINT32 object1, UINT32 object2)
{
   NetObj *p = FindObjectById(object1);
   return (p != NULL) ? p->isChild(object2) : false;
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
         if (hStmt != NULL)
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
static int FilterObject(NXSL_VM *vm, NetObj *object, NXSL_VariableSystem **globalVariables)
{
   SetupServerScriptVM(vm, object, NULL);
   vm->setContextObject(object->createNXSLObject(vm));
   NXSL_VariableSystem *expressionVariables = NULL;
   ObjectRefArray<NXSL_Value> args(0);
   if (!vm->run(args, globalVariables, &expressionVariables))
   {
      delete expressionVariables;
      return -1;
   }
   if ((globalVariables != NULL) && (expressionVariables != NULL))
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
      if ((v1 != NULL) && (v2 != NULL))
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
ObjectArray<ObjectQueryResult> *QueryObjects(const TCHAR *query, UINT32 userId, TCHAR *errorMessage,
         size_t errorMessageLen, const StringList *fields, const StringList *orderBy, UINT32 limit)
{
   NXSL_VM *vm = NXSLCompileAndCreateVM(query, errorMessage, errorMessageLen, new NXSL_ServerEnv());
   if (vm == NULL)
      return NULL;

   bool readFields = (fields != NULL);

   // Set class constants
   vm->addConstant("ACCESSPOINT", vm->createValue(OBJECT_ACCESSPOINT));
   vm->addConstant("AGENTPOLICY_LOGPARSER", vm->createValue(OBJECT_AGENTPOLICY_LOGPARSER));
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

   ObjectArray<NetObj> *objects = g_idxObjectById.getObjects(true, FilterAccessibleObjects);
   ObjectArray<ObjectQueryResult> *resultSet = new ObjectArray<ObjectQueryResult>(64, 64, true);
   for(int i = 0; i < objects->size(); i++)
   {
      NetObj *curr = objects->get(i);

      NXSL_VariableSystem *globals = NULL;
      int rc = FilterObject(vm, curr, readFields ? &globals : NULL);
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
               if (v != NULL)
               {
                  objectData->add(v->getValue()->getValueAsCString());
               }
               else
               {
#ifdef UNICODE
                  char attr[MAX_IDENTIFIER_LENGTH];
                  WideCharToMultiByte(CP_UTF8, 0, fields->get(j), -1, attr, MAX_IDENTIFIER_LENGTH - 1, NULL, NULL);
                  attr[MAX_IDENTIFIER_LENGTH - 1] = 0;
                  NXSL_Value *av = object->getClass()->getAttr(object, attr);
#else
                  NXSL_Value *av = object->getClass()->getAttr(object, fields->get(j));
#endif
                  if (av != NULL)
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
            objectData = NULL;
         }
         resultSet->add(new ObjectQueryResult(curr, objectData));
      }
      delete globals;
   }

   delete vm;
   for(int i = 0; i < objects->size(); i++)
      objects->get(i)->decRefCount();
   delete objects;

   // Sort result set and apply limit
   if (resultSet != NULL)
   {
      if ((orderBy != NULL) && !orderBy->isEmpty())
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
   UINT32 nodeId;
   StructArray<DependentNode> *dependencies;
};

/**
 * Node dependency check callback
 */
static void NodeDependencyCheckCallback(NetObj *object, void *context)
{
   NodeDependencyCheckData *d = static_cast<NodeDependencyCheckData*>(context);
   Node *node = static_cast<Node*>(object);

   UINT32 type = 0;
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
StructArray<DependentNode> *GetNodeDependencies(UINT32 nodeId)
{
   NodeDependencyCheckData data;
   data.nodeId = nodeId;
   data.dependencies = new StructArray<DependentNode>();
   g_idxNodeById.forEach(NodeDependencyCheckCallback, &data);
   return data.dependencies;
}
