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
** File: objects.cpp
**
**/

#include "nxcore.h"

/**
 * Global data
 */
BOOL g_bModificationsLocked = FALSE;

Network NXCORE_EXPORTABLE *g_pEntireNet = NULL;
ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot = NULL;
TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot = NULL;
PolicyRoot NXCORE_EXPORTABLE *g_pPolicyRoot = NULL;
NetworkMapRoot NXCORE_EXPORTABLE *g_pMapRoot = NULL;
DashboardRoot NXCORE_EXPORTABLE *g_pDashboardRoot = NULL;
BusinessServiceRoot NXCORE_EXPORTABLE *g_pBusinessServiceRoot = NULL;

UINT32 NXCORE_EXPORTABLE g_dwMgmtNode = 0;
UINT32 g_dwNumCategories = 0;
CONTAINER_CATEGORY *g_pContainerCatList = NULL;

Queue *g_pTemplateUpdateQueue = NULL;

ObjectIndex g_idxObjectById;
InetAddressIndex g_idxSubnetByAddr;
InetAddressIndex g_idxInterfaceByAddr;
ObjectIndex g_idxZoneByGUID;
ObjectIndex g_idxNodeById;
InetAddressIndex g_idxNodeByAddr;
ObjectIndex g_idxClusterById;
ObjectIndex g_idxMobileDeviceById;
ObjectIndex g_idxAccessPointById;
ObjectIndex g_idxConditionById;
ObjectIndex g_idxServiceCheckById;
ObjectIndex g_idxNetMapById;

const TCHAR *g_szClassName[]={ _T("Generic"), _T("Subnet"), _T("Node"), _T("Interface"),
                               _T("Network"), _T("Container"), _T("Zone"), _T("ServiceRoot"),
                               _T("Template"), _T("TemplateGroup"), _T("TemplateRoot"),
                               _T("NetworkService"), _T("VPNConnector"), _T("Condition"),
                               _T("Cluster"), _T("PolicyGroup"), _T("PolicyRoot"),
                               _T("AgentPolicy"), _T("AgentPolicyConfig"), _T("NetworkMapRoot"),
                               _T("NetworkMapGroup"), _T("NetworkMap"), _T("DashboardRoot"), 
                               _T("Dashboard"), _T("ReportRoot"), _T("ReportGroup"), _T("Report"),
                               _T("BusinessServiceRoot"), _T("BusinessService"), _T("NodeLink"),
                               _T("ServiceCheck"), _T("MobileDevice"), _T("Rack"), _T("AccessPoint")
};

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
static CONDITION s_condUpdateMaps = INVALID_CONDITION_HANDLE;

/**
 * Thread which apply template updates
 */
static THREAD_RESULT THREAD_CALL ApplyTemplateThread(void *pArg)
{
	DbgPrintf(1, _T("Apply template thread started"));
   while(1)
   {
      TEMPLATE_UPDATE_INFO *pInfo = (TEMPLATE_UPDATE_INFO *)g_pTemplateUpdateQueue->getOrBlock();
      if (pInfo == INVALID_POINTER_VALUE)
         break;

		DbgPrintf(5, _T("ApplyTemplateThread: template=%d(%s) updateType=%d target=%d removeDci=%s"),
         pInfo->pTemplate->getId(), pInfo->pTemplate->getName(), pInfo->updateType, pInfo->targetId, pInfo->removeDCI ? _T("true") : _T("false"));
      BOOL bSuccess = FALSE;
      NetObj *dcTarget = FindObjectById(pInfo->targetId);
      if (dcTarget != NULL)
      {
         if ((dcTarget->getObjectClass() == OBJECT_NODE) || (dcTarget->getObjectClass() == OBJECT_CLUSTER) || (dcTarget->getObjectClass() == OBJECT_MOBILEDEVICE))
         {
            BOOL lock1, lock2;

            switch(pInfo->updateType)
            {
               case APPLY_TEMPLATE:
                  lock1 = pInfo->pTemplate->lockDCIList(0x7FFFFFFF, _T("SYSTEM"), NULL);
                  lock2 = ((DataCollectionTarget *)dcTarget)->lockDCIList(0x7FFFFFFF, _T("SYSTEM"), NULL);
                  if (lock1 && lock2)
                  {
                     pInfo->pTemplate->applyToTarget((DataCollectionTarget *)dcTarget);
                     bSuccess = TRUE;
                  }
                  if (lock1)
                     pInfo->pTemplate->unlockDCIList(0x7FFFFFFF);
                  if (lock2)
                     ((DataCollectionTarget *)dcTarget)->unlockDCIList(0x7FFFFFFF);
                  break;
               case REMOVE_TEMPLATE:
                  if (((DataCollectionTarget *)dcTarget)->lockDCIList(0x7FFFFFFF, _T("SYSTEM"), NULL))
                  {
                     ((DataCollectionTarget *)dcTarget)->unbindFromTemplate(pInfo->pTemplate->getId(), pInfo->removeDCI);
                     ((DataCollectionTarget *)dcTarget)->unlockDCIList(0x7FFFFFFF);
                     bSuccess = TRUE;
                  }
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
         g_pTemplateUpdateQueue->put(pInfo);    // Requeue
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
      DataCollectionTarget *t = (DataCollectionTarget *)objects->get(i);
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
   DbgPrintf(1, _T("Started caching of DCI values"));

	UpdateDataCollectionCache(&g_idxNodeById);
	UpdateDataCollectionCache(&g_idxClusterById);
	UpdateDataCollectionCache(&g_idxMobileDeviceById);
	UpdateDataCollectionCache(&g_idxAccessPointById);

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
	DbgPrintf(2, _T("Map update thread started"));
	while(!SleepAndCheckForShutdown(60))
	{
		DbgPrintf(5, _T("Updating maps..."));
		g_idxNetMapById.forEach(UpdateMapCallback, NULL);
		DbgPrintf(5, _T("Map update completed"));
	}
	DbgPrintf(2, _T("Map update thread stopped"));
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

   g_pTemplateUpdateQueue = new Queue;

	s_condUpdateMaps = ConditionCreate(FALSE);

   // Create "Entire Network" object
   g_pEntireNet = new Network;
   NetObjInsert(g_pEntireNet, FALSE);

   // Create "Service Root" object
   g_pServiceRoot = new ServiceRoot;
   NetObjInsert(g_pServiceRoot, FALSE);

   // Create "Template Root" object
   g_pTemplateRoot = new TemplateRoot;
   NetObjInsert(g_pTemplateRoot, FALSE);

	// Create "Policy Root" object
   g_pPolicyRoot = new PolicyRoot;
   NetObjInsert(g_pPolicyRoot, FALSE);
   
	// Create "Network Maps Root" object
   g_pMapRoot = new NetworkMapRoot;
   NetObjInsert(g_pMapRoot, FALSE);
   
	// Create "Dashboard Root" object
   g_pDashboardRoot = new DashboardRoot;
   NetObjInsert(g_pDashboardRoot, FALSE);
   
   // Create "Business Service Root" object
   g_pBusinessServiceRoot = new BusinessServiceRoot;
   NetObjInsert(g_pBusinessServiceRoot, FALSE);
   
	DbgPrintf(1, _T("Built-in objects created"));

	// Initialize service checks
	SlmCheck::init();

   // Start template update applying thread
   ThreadCreate(ApplyTemplateThread, 0, NULL);
}

/**
 * Insert new object into network
 */
void NetObjInsert(NetObj *pObject, BOOL bNewObject)
{
   if (bNewObject)   
   {
      // Assign unique ID to new object
      pObject->setId(CreateUniqueId(IDG_NETWORK_OBJECT));
		pObject->generateGuid();

      // Create tables for storing data collection values
      if ((pObject->getObjectClass() == OBJECT_NODE) || 
          (pObject->getObjectClass() == OBJECT_MOBILEDEVICE) || 
          (pObject->getObjectClass() == OBJECT_CLUSTER) || 
          (pObject->getObjectClass() == OBJECT_ACCESSPOINT))
      {
         TCHAR szQuery[256], szQueryTemplate[256];
         UINT32 i;

         MetaDataReadStr(_T("IDataTableCreationCommand"), szQueryTemplate, 255, _T(""));
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId());
         DBQuery(g_hCoreDB, szQuery);

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("IDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId(), pObject->getId());
               DBQuery(g_hCoreDB, szQuery);
            }
         }

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("TDataTableCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId(), pObject->getId());
               DBQuery(g_hCoreDB, szQuery);
            }
         }

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("TDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->getId(), pObject->getId());
               DBQuery(g_hCoreDB, szQuery);
            }
         }
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
			case OBJECT_POLICYGROUP:
			case OBJECT_POLICYROOT:
			case OBJECT_AGENTPOLICY:
			case OBJECT_AGENTPOLICY_CONFIG:
			case OBJECT_NETWORKMAPROOT:
			case OBJECT_NETWORKMAPGROUP:
			case OBJECT_DASHBOARDROOT:
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
				      Zone *zone = (Zone *)g_idxZoneByGUID.get(((Node *)pObject)->getZoneId());
				      if (zone != NULL)
				      {
					      zone->addToIndex((Node *)pObject);
				      }
				      else
				      {
					      DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"),
					                (int)((Node *)pObject)->getZoneId(), pObject->getName(), (int)pObject->getId());
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
         case OBJECT_SUBNET:
            if (((Subnet *)pObject)->getIpAddress().isValidUnicast())
            {
					if (IsZoningEnabled())
					{
						Zone *zone = (Zone *)g_idxZoneByGUID.get(((Subnet *)pObject)->getZoneId());
						if (zone != NULL)
						{
							zone->addToIndex((Subnet *)pObject);
						}
						else
						{
							DbgPrintf(2, _T("Cannot find zone object with GUID=%d for subnet object %s [%d]"),
							          (int)((Subnet *)pObject)->getZoneId(), pObject->getName(), (int)pObject->getId());
						}
					}
					else
					{
						g_idxSubnetByAddr.put(((Subnet *)pObject)->getIpAddress(), pObject);
					}
               if (bNewObject)
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
						Zone *zone = (Zone *)g_idxZoneByGUID.get(((Interface *)pObject)->getZoneId());
						if (zone != NULL)
						{
							zone->addToIndex((Interface *)pObject);
						}
						else
						{
							DbgPrintf(2, _T("Cannot find zone object with GUID=%d for interface object %s [%d]"),
							          (int)((Interface *)pObject)->getZoneId(), pObject->getName(), (int)pObject->getId());
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
				g_idxZoneByGUID.put(((Zone *)pObject)->getZoneId(), pObject);
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
	if (bNewObject)
	{
      CALL_ALL_MODULES(pfPostObjectCreate, (pObject));
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
		case OBJECT_POLICYGROUP:
		case OBJECT_POLICYROOT:
		case OBJECT_AGENTPOLICY:
		case OBJECT_AGENTPOLICY_CONFIG:
		case OBJECT_NETWORKMAPROOT:
		case OBJECT_NETWORKMAPGROUP:
		case OBJECT_DASHBOARDROOT:
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
				   Zone *zone = (Zone *)g_idxZoneByGUID.get(((Node *)pObject)->getZoneId());
				   if (zone != NULL)
				   {
					   zone->removeFromIndex((Node *)pObject);
				   }
				   else
				   {
					   DbgPrintf(2, _T("Cannot find zone object with GUID=%d for node object %s [%d]"),
					             (int)((Node *)pObject)->getZoneId(), pObject->getName(), (int)pObject->getId());
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
      case OBJECT_SUBNET:
         if (((Subnet *)pObject)->getIpAddress().isValidUnicast())
         {
				if (IsZoningEnabled())
				{
					Zone *zone = (Zone *)g_idxZoneByGUID.get(((Subnet *)pObject)->getZoneId());
					if (zone != NULL)
					{
						zone->removeFromIndex((Subnet *)pObject);
					}
					else
					{
						DbgPrintf(2, _T("Cannot find zone object with GUID=%d for subnet object %s [%d]"),
						          (int)((Subnet *)pObject)->getZoneId(), pObject->getName(), (int)pObject->getId());
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
				Zone *zone = (Zone *)g_idxZoneByGUID.get(((Interface *)pObject)->getZoneId());
				if (zone != NULL)
				{
					zone->removeFromIndex((Interface *)pObject);
				}
				else
				{
					DbgPrintf(2, _T("Cannot find zone object with GUID=%d for interface object %s [%d]"),
					          (int)((Interface *)pObject)->getZoneId(), pObject->getName(), (int)pObject->getId());
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
			g_idxZoneByGUID.remove(((Zone *)pObject)->getZoneId());
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

/**
 * Find node by IP address
 */
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneId, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return NULL;

	Zone *zone = IsZoningEnabled() ? (Zone *)g_idxZoneByGUID.get(zoneId) : NULL;

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
 * Find node by IP address using first match from IP address list
 */
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneId, const InetAddressList *ipAddrList)
{
   for(int i = 0; i < ipAddrList->size(); i++)
   {
      Node *node = FindNodeByIP(zoneId, ipAddrList->get(i));
      if (node != NULL)
         return node;
   }
   return NULL;
}

/**
 * Find interface by IP address
 */
Interface NXCORE_EXPORTABLE *FindInterfaceByIP(UINT32 zoneId, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return NULL;

	Zone *zone = IsZoningEnabled() ? (Zone *)g_idxZoneByGUID.get(zoneId) : NULL;

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
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(UINT32 zoneId, const InetAddress& ipAddr)
{
   if (!ipAddr.isValidUnicast())
      return NULL;

	Subnet *subnet = NULL;
	if (IsZoningEnabled())
	{
		Zone *zone = (Zone *)g_idxZoneByGUID.get(zoneId);
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
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(UINT32 zoneId, const InetAddress& nodeAddr)
{
   if (!nodeAddr.isValidUnicast())
      return NULL;

   SUBNET_MATCHING_DATA matchData;
   matchData.ipAddr = nodeAddr;
   matchData.maskLen = -1;
   matchData.subnet = NULL;
	if (IsZoningEnabled())
	{
		Zone *zone = (Zone *)g_idxZoneByGUID.get(zoneId);
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
 * Find object by ID
 */
NetObj NXCORE_EXPORTABLE *FindObjectById(UINT32 dwId, int objClass)
{
	NetObj *object = g_idxObjectById.get(dwId);
	if ((object == NULL) || (objClass == -1))
		return object;
	return (objClass == object->getObjectClass()) ? object : NULL;
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
 * Callback data for FindObjectByName
 */
struct __find_object_data
{
	int objClass;
	const TCHAR *name;
};

/**
 * Object name comparator for FindObjectByName
 */
static bool ObjectNameComparator(NetObj *object, void *data)
{
	struct __find_object_data *fd = (struct __find_object_data *)data;
	return ((fd->objClass == -1) || (fd->objClass == object->getObjectClass())) &&
	       !object->isDeleted() && !_tcsicmp(object->getName(), fd->name);
}

/**
 * Find object by name
 */
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass)
{
	struct __find_object_data data;

	data.objClass = objClass;
	data.name = name;
	return g_idxObjectById.find(ObjectNameComparator, &data);
}

/**
 * GUID comparator for FindObjectByGUID
 */
static bool ObjectGuidComparator(NetObj *object, void *data)
{
	uuid_t temp;
	object->getGuid(temp);
	return !object->isDeleted() && !uuid_compare((BYTE *)data, temp);
}

/**
 * Find object by GUID
 */
NetObj NXCORE_EXPORTABLE *FindObjectByGUID(uuid_t guid, int objClass)
{
	NetObj *object = g_idxObjectById.find(ObjectGuidComparator, guid);
	return (object != NULL) ? (((objClass == -1) || (objClass == object->getObjectClass())) ? object : NULL) : NULL;
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
 * Callback for FindClusterByResourceIP
 */
static bool ClusterResourceIPComparator(NetObj *object, void *ipAddr)
{
	return (object->getObjectClass() == OBJECT_CLUSTER) && !object->isDeleted() && ((Cluster *)object)->isVirtualAddr(*((const InetAddress *)ipAddr));
}

/**
 * Find cluster by resource IP
 */
Cluster NXCORE_EXPORTABLE *FindClusterByResourceIP(const InetAddress& ipAddr)
{
	return (Cluster *)g_idxObjectById.find(ClusterResourceIPComparator, (void *)&ipAddr);
}

/**
 * Data structure for IsClusterIP callback
 */
struct __cluster_ip_data
{
	InetAddress ipAddr;
	UINT32 zoneId;
};

/**
 * Cluster IP comparator - returns true if given address in given zone is cluster IP address
 */
static bool ClusterIPComparator(NetObj *object, void *data)
{
	struct __cluster_ip_data *d = (struct __cluster_ip_data *)data;
	return (object->getObjectClass() == OBJECT_CLUSTER) && !object->isDeleted() &&
	       (((Cluster *)object)->getZoneId() == d->zoneId) &&
			 (((Cluster *)object)->isVirtualAddr(d->ipAddr) ||
			  ((Cluster *)object)->isSyncAddr(d->ipAddr));
}

/**
 * Check if given IP address is used by cluster (it's either
 * resource IP or located on one of sync subnets)
 */
bool NXCORE_EXPORTABLE IsClusterIP(UINT32 zoneId, const InetAddress& ipAddr)
{
	struct __cluster_ip_data data;
	data.zoneId = zoneId;
	data.ipAddr = ipAddr;
	return g_idxObjectById.find(ClusterIPComparator, &data) != NULL;
}

/**
 * Find zone object by GUID
 */
Zone NXCORE_EXPORTABLE *FindZoneByGUID(UINT32 dwZoneGUID)
{
	return (Zone *)g_idxZoneByGUID.get(dwZoneGUID);
}

/**
 * Object comparator for FindLocalMgmtNode()
 */
static bool LocalMgmtNodeComparator(NetObj *object, void *data)
{
	return (((Node *)object)->getFlags() & NF_IS_LOCAL_MGMT) ? true : false;
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
 * ObjectIndex::forEach callback which links container child objects
 */
static void LinkChildObjectsCallback(NetObj *object, void *data)
{
	if ((object->getObjectClass() == OBJECT_CONTAINER) ||
		 (object->getObjectClass() == OBJECT_RACK) ||
		 (object->getObjectClass() == OBJECT_TEMPLATEGROUP) ||
		 (object->getObjectClass() == OBJECT_POLICYGROUP) ||
		 (object->getObjectClass() == OBJECT_NETWORKMAPGROUP) ||
		 (object->getObjectClass() == OBJECT_DASHBOARD) ||
		 (object->getObjectClass() == OBJECT_BUSINESSSERVICE) ||
		 (object->getObjectClass() == OBJECT_NODELINK))
	{
		((Container *)object)->linkChildObjects();
	}
}

/**
 * Load objects from database at stratup
 */
BOOL LoadObjects()
{
   DB_RESULT hResult;
   UINT32 i, dwNumRows;
   UINT32 dwId;
   Template *pTemplate;
   TCHAR szQuery[256];

   // Prevent objects to change it's modification flag
   g_bModificationsLocked = TRUE;

   // Load container categories
   DbgPrintf(2, _T("Loading container categories..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT category,name,image_id,description FROM container_categories"));
   if (hResult != NULL)
   {
      g_dwNumCategories = DBGetNumRows(hResult);
      g_pContainerCatList = (CONTAINER_CATEGORY *)malloc(sizeof(CONTAINER_CATEGORY) * g_dwNumCategories);
      for(i = 0; i < (int)g_dwNumCategories; i++)
      {
         g_pContainerCatList[i].dwCatId = DBGetFieldULong(hResult, i, 0);
         DBGetField(hResult, i, 1, g_pContainerCatList[i].szName, MAX_OBJECT_NAME);
         g_pContainerCatList[i].dwImageId = DBGetFieldULong(hResult, i, 2);
         g_pContainerCatList[i].pszDescription = DBGetField(hResult, i, 3, NULL, 0);
         DecodeSQLString(g_pContainerCatList[i].pszDescription);
      }
      DBFreeResult(hResult);
   }

   // Load built-in object properties
   DbgPrintf(2, _T("Loading built-in object properties..."));
   g_pEntireNet->LoadFromDB();
   g_pServiceRoot->LoadFromDB();
   g_pTemplateRoot->LoadFromDB();
	g_pPolicyRoot->LoadFromDB();
	g_pMapRoot->LoadFromDB();
	g_pDashboardRoot->LoadFromDB();
	g_pBusinessServiceRoot->LoadFromDB();

   // Load zones
   if (g_flags & AF_ENABLE_ZONING)
   {
      Zone *pZone;

      DbgPrintf(2, _T("Loading zones..."));

      // Load (or create) default zone
      pZone = new Zone;
      pZone->generateGuid();
      pZone->loadFromDatabase(BUILTIN_OID_ZONE0);
      NetObjInsert(pZone, FALSE);
      g_pEntireNet->AddZone(pZone);

      hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM zones WHERE id<>4"));
      if (hResult != 0)
      {
         dwNumRows = DBGetNumRows(hResult);
         for(i = 0; i < dwNumRows; i++)
         {
            dwId = DBGetFieldULong(hResult, i, 0);
            pZone = new Zone;
            if (pZone->loadFromDatabase(dwId))
            {
               if (!pZone->isDeleted())
                  g_pEntireNet->AddZone(pZone);
               NetObjInsert(pZone, FALSE);  // Insert into indexes
            }
            else     // Object load failed
            {
               delete pZone;
               nxlog_write(MSG_ZONE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
            }
         }
         DBFreeResult(hResult);
      }
   }

   // Load conditions
   // We should load conditions before nodes because
   // DCI cache size calculation uses information from condition objects
   DbgPrintf(2, _T("Loading conditions..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM conditions"));
   if (hResult != NULL)
   {
      Condition *pCondition;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pCondition = new Condition;
         if (pCondition->loadFromDatabase(dwId))
         {
            NetObjInsert(pCondition, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pCondition;
            nxlog_write(MSG_CONDITION_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load subnets
   DbgPrintf(2, _T("Loading subnets..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM subnets"));
   if (hResult != 0)
   {
      Subnet *pSubnet;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pSubnet = new Subnet;
         if (pSubnet->loadFromDatabase(dwId))
         {
            if (!pSubnet->isDeleted())
            {
               if (g_flags & AF_ENABLE_ZONING)
               {
                  Zone *pZone;

                  pZone = FindZoneByGUID(pSubnet->getZoneId());
                  if (pZone != NULL)
                     pZone->addSubnet(pSubnet);
               }
               else
               {
                  g_pEntireNet->AddSubnet(pSubnet);
               }
            }
            NetObjInsert(pSubnet, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pSubnet;
            nxlog_write(MSG_SUBNET_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load mobile devices
   DbgPrintf(2, _T("Loading mobile devices..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM mobile_devices"));
   if (hResult != 0)
   {
      MobileDevice *md;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         md = new MobileDevice;
         if (md->loadFromDatabase(dwId))
         {
            NetObjInsert(md, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete md;
            nxlog_write(MSG_MOBILEDEVICE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load nodes
   DbgPrintf(2, _T("Loading nodes..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM nodes"));
   if (hResult != NULL)
   {
      Node *pNode;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pNode = new Node;
         if (pNode->loadFromDatabase(dwId))
         {
            NetObjInsert(pNode, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pNode;
            nxlog_write(MSG_NODE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load access points
   DbgPrintf(2, _T("Loading access points..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM access_points"));
   if (hResult != NULL)
   {
		AccessPoint *ap;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         ap = new AccessPoint;
         if (ap->loadFromDatabase(dwId))
         {
            NetObjInsert(ap, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(MSG_AP_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
            delete ap;
         }
      }
      DBFreeResult(hResult);
   }

   // Load interfaces
   DbgPrintf(2, _T("Loading interfaces..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM interfaces"));
   if (hResult != 0)
   {
      Interface *pInterface;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pInterface = new Interface;
         if (pInterface->loadFromDatabase(dwId))
         {
            NetObjInsert(pInterface, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(MSG_INTERFACE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
            delete pInterface;
         }
      }
      DBFreeResult(hResult);
   }

   // Load network services
   DbgPrintf(2, _T("Loading network services..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM network_services"));
   if (hResult != 0)
   {
      NetworkService *pService;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pService = new NetworkService;
         if (pService->loadFromDatabase(dwId))
         {
            NetObjInsert(pService, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pService;
            nxlog_write(MSG_NETSRV_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load VPN connectors
   DbgPrintf(2, _T("Loading VPN connectors..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM vpn_connectors"));
   if (hResult != NULL)
   {
      VPNConnector *pConnector;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pConnector = new VPNConnector;
         if (pConnector->loadFromDatabase(dwId))
         {
            NetObjInsert(pConnector, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pConnector;
            nxlog_write(MSG_VPNC_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load clusters
   DbgPrintf(2, _T("Loading clusters..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM clusters"));
   if (hResult != NULL)
   {
      Cluster *pCluster;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pCluster = new Cluster;
         if (pCluster->loadFromDatabase(dwId))
         {
            NetObjInsert(pCluster, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pCluster;
            nxlog_write(MSG_CLUSTER_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Start cache loading thread.
   // All data collection targets must be loaded at this point.
   ThreadCreate(CacheLoadingThread, 0, NULL);

   // Load templates
   DbgPrintf(2, _T("Loading templates..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM templates"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pTemplate = new Template;
         if (pTemplate->loadFromDatabase(dwId))
         {
            NetObjInsert(pTemplate, FALSE);  // Insert into indexes
				pTemplate->calculateCompoundStatus();	// Force status change to NORMAL
         }
         else     // Object load failed
         {
            delete pTemplate;
            nxlog_write(MSG_TEMPLATE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load agent policies
   DbgPrintf(2, _T("Loading agent policies..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id,policy_type FROM ap_common"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         AgentPolicy *policy;

			dwId = DBGetFieldULong(hResult, i, 0);
			int type = DBGetFieldLong(hResult, i, 1);
			switch(type)
			{
				case AGENT_POLICY_CONFIG:
					policy = new AgentPolicyConfig();
					break;
				default:
					policy = new AgentPolicy(type);
					break;
			}
         if (policy->loadFromDatabase(dwId))
         {
            NetObjInsert(policy, FALSE);  // Insert into indexes
				policy->calculateCompoundStatus();	// Force status change to NORMAL
         }
         else     // Object load failed
         {
            delete policy;
            nxlog_write(MSG_AGENTPOLICY_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load network maps
   DbgPrintf(2, _T("Loading network maps..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM network_maps"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         NetworkMap *map = new NetworkMap;
         if (map->loadFromDatabase(dwId))
         {
            NetObjInsert(map, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete map;
            nxlog_write(MSG_NETMAP_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load container objects
   DbgPrintf(2, _T("Loading containers..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_CONTAINER);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      Container *pContainer;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pContainer = new Container;
         if (pContainer->loadFromDatabase(dwId))
         {
            NetObjInsert(pContainer, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pContainer;
            nxlog_write(MSG_CONTAINER_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load racks
   DbgPrintf(2, _T("Loading racks..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM racks"));
   if (hResult != 0)
   {
		Rack *rack;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         rack = new Rack;
         if (rack->loadFromDatabase(dwId))
         {
            NetObjInsert(rack, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            nxlog_write(MSG_RACK_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
            delete rack;
         }
      }
      DBFreeResult(hResult);
   }

   // Load template group objects
   DbgPrintf(2, _T("Loading template groups..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_TEMPLATEGROUP);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      TemplateGroup *pGroup;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pGroup = new TemplateGroup;
         if (pGroup->loadFromDatabase(dwId))
         {
            NetObjInsert(pGroup, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pGroup;
            nxlog_write(MSG_TG_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load policy group objects
   DbgPrintf(2, _T("Loading policy groups..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_POLICYGROUP);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      PolicyGroup *pGroup;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pGroup = new PolicyGroup;
         if (pGroup->loadFromDatabase(dwId))
         {
            NetObjInsert(pGroup, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pGroup;
            nxlog_write(MSG_PG_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load map group objects
   DbgPrintf(2, _T("Loading map groups..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_NETWORKMAPGROUP);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      NetworkMapGroup *pGroup;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pGroup = new NetworkMapGroup;
         if (pGroup->loadFromDatabase(dwId))
         {
            NetObjInsert(pGroup, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pGroup;
            nxlog_write(MSG_MG_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load dashboard objects
   DbgPrintf(2, _T("Loading dashboards..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM dashboards"));
   if (hResult != 0)
   {
      Dashboard *pd;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pd = new Dashboard;
         if (pd->loadFromDatabase(dwId))
         {
            NetObjInsert(pd, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pd;
            nxlog_write(MSG_DASHBOARD_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Loading business service objects
   DbgPrintf(2, _T("Loading business services..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_BUSINESSSERVICE);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
	   dwNumRows = DBGetNumRows(hResult);
	   for(i = 0; i < dwNumRows; i++)
	   {
		   dwId = DBGetFieldULong(hResult, i, 0);
		   BusinessService *service = new BusinessService;
		   if (service->loadFromDatabase(dwId))
		   {
			   NetObjInsert(service, FALSE);  // Insert into indexes
		   }
		   else     // Object load failed
		   {
			   delete service;
			   nxlog_write(MSG_BUSINESS_SERVICE_LOAD_FAILED, NXLOG_ERROR, "d", dwId);
		   }
	   }
	   DBFreeResult(hResult);
   }

   // Loading business service objects
   DbgPrintf(2, _T("Loading node links..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_NODELINK);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
	   dwNumRows = DBGetNumRows(hResult);
	   for(i = 0; i < dwNumRows; i++)
	   {
		   dwId = DBGetFieldULong(hResult, i, 0);
		   NodeLink *nl = new NodeLink;
		   if (nl->loadFromDatabase(dwId))
		   {
			   NetObjInsert(nl, FALSE);  // Insert into indexes
		   }
		   else     // Object load failed
		   {
			   delete nl;
			   nxlog_write(MSG_NODE_LINK_LOAD_FAILED, NXLOG_ERROR, "d", dwId);
		   }
	   }
	   DBFreeResult(hResult);
   }

   // Load service check objects
   DbgPrintf(2, _T("Loading service checks..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM slm_checks"));
   if (hResult != 0)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         SlmCheck *check = new SlmCheck;
         if (check->loadFromDatabase(dwId))
         {
            NetObjInsert(check, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete check;
            nxlog_write(MSG_SERVICE_CHECK_LOAD_FAILED, NXLOG_ERROR, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

	// Load custom object classes provided by modules
   CALL_ALL_MODULES(pfLoadObjects, ());

   // Link childs to container and template group objects
   DbgPrintf(2, _T("Linking objects..."));
	g_idxObjectById.forEach(LinkChildObjectsCallback, NULL);

   // Link childs to root objects
   g_pServiceRoot->LinkChildObjects();
   g_pTemplateRoot->LinkChildObjects();
   g_pPolicyRoot->LinkChildObjects();
   g_pMapRoot->LinkChildObjects();
	g_pDashboardRoot->LinkChildObjects();
	g_pBusinessServiceRoot->LinkChildObjects();

	// Link custom object classes provided by modules
   CALL_ALL_MODULES(pfLinkObjects, ());

   // Allow objects to change it's modification flag
   g_bModificationsLocked = FALSE;

   // Recalculate status for built-in objects
   g_pEntireNet->calculateCompoundStatus();
   g_pServiceRoot->calculateCompoundStatus();
   g_pTemplateRoot->calculateCompoundStatus();
   g_pPolicyRoot->calculateCompoundStatus();
   g_pMapRoot->calculateCompoundStatus();
   g_pBusinessServiceRoot->calculateCompoundStatus();

   // Recalculate status for zone objects
   if (g_flags & AF_ENABLE_ZONING)
   {
		g_idxZoneByGUID.forEach(RecalcStatusCallback, NULL);
   }

   // Start map update thread
   ThreadCreate(MapUpdateThread, 0, NULL);

   return TRUE;
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
 * Enumeration callback for DumpObjects
 */
static void DumpObjectCallback(NetObj *object, void *data)
{
	struct __dump_objects_data *dd = (struct __dump_objects_data *)data;

   // Apply name filter
   if ((dd->filter != NULL) && !MatchString(dd->filter, object->getName(), false))
      return;

	CONSOLE_CTX pCtx = dd->console;
   CONTAINER_CATEGORY *pCat;

	ConsolePrintf(pCtx, _T("Object ID %d \"%s\"\n")
                       _T("   Class: %s  Status: %s  IsModified: %d  IsDeleted: %d\n"),
					  object->getId(), object->getName(), (object->getObjectClass() < OBJECT_CUSTOM) ? g_szClassName[object->getObjectClass()] : _T("Custom"),
                 GetStatusAsText(object->Status(), true),
                 object->isModified(), object->isDeleted());
   ConsolePrintf(pCtx, _T("   Parents: <%s>\n   Childs: <%s>\n"), 
                 object->dbgGetParentList(dd->buffer), object->dbgGetChildList(&dd->buffer[4096]));
	time_t t = object->getTimeStamp();
	struct tm *ltm = localtime(&t);
	_tcsftime(dd->buffer, 256, _T("%d.%b.%Y %H:%M:%S"), ltm);
   ConsolePrintf(pCtx, _T("   Last change: %s\n"), dd->buffer);
   switch(object->getObjectClass())
   {
      case OBJECT_NODE:
         ConsolePrintf(pCtx, _T("   Primary IP: %s\n   IsSNMP: %d IsAgent: %d IsLocal: %d OID: %s\n"),
                       ((Node *)object)->getIpAddress().toString(dd->buffer),
                       ((Node *)object)->isSNMPSupported(),
                       ((Node *)object)->isNativeAgent(),
                       ((Node *)object)->isLocalManagement(),
                       ((Node *)object)->getObjectId());
         break;
      case OBJECT_SUBNET:
         ConsolePrintf(pCtx, _T("   IP address: %s/%d\n"), ((Subnet *)object)->getIpAddress().toString(dd->buffer), ((Subnet *)object)->getIpAddress().getMaskBits());
         break;
      case OBJECT_ACCESSPOINT:
         ConsolePrintf(pCtx, _T("   IP address: %s\n"), ((AccessPoint *)object)->getIpAddress().toString(dd->buffer));
         break;
      case OBJECT_INTERFACE:
         ConsolePrintf(pCtx, _T("   MAC address: %s\n"), MACToStr(((Interface *)object)->getMacAddr(), dd->buffer));
         for(int n = 0; n < ((Interface *)object)->getIpAddressList()->size(); n++)
         {
            const InetAddress& a = ((Interface *)object)->getIpAddressList()->get(n);
            ConsolePrintf(pCtx, _T("   IP address: %s/%d\n"), a.toString(dd->buffer), a.getMaskBits());
         }
         break;
      case OBJECT_CONTAINER:
         pCat = FindContainerCategory(((Container *)object)->getCategory());
         ConsolePrintf(pCtx, _T("   Category: %s\n"), pCat ? pCat->szName : _T("<unknown>"));
         break;
      case OBJECT_TEMPLATE:
         ConsolePrintf(pCtx, _T("   Version: %d.%d\n"), 
                       ((Template *)(object))->getVersionMajor(),
                       ((Template *)(object))->getVersionMinor());
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
bool IsValidParentClass(int iChildClass, int iParentClass)
{
   switch(iParentClass)
   {
		case OBJECT_NETWORK:
			if ((iChildClass == OBJECT_ZONE) && (g_flags & AF_ENABLE_ZONING))
				return true;
			break;
      case OBJECT_SERVICEROOT:
      case OBJECT_CONTAINER:
         if ((iChildClass == OBJECT_CONTAINER) || 
             (iChildClass == OBJECT_RACK) ||
             (iChildClass == OBJECT_NODE) ||
             (iChildClass == OBJECT_CLUSTER) ||
             (iChildClass == OBJECT_MOBILEDEVICE) ||
             (iChildClass == OBJECT_CONDITION) ||
             (iChildClass == OBJECT_SUBNET))
            return true;
         break;
      case OBJECT_RACK:
         if (iChildClass == OBJECT_NODE)
            return true;
         break;
      case OBJECT_TEMPLATEROOT:
      case OBJECT_TEMPLATEGROUP:
         if ((iChildClass == OBJECT_TEMPLATEGROUP) || 
             (iChildClass == OBJECT_TEMPLATE))
            return true;
         break;
      case OBJECT_TEMPLATE:
         if ((iChildClass == OBJECT_NODE) || 
             (iChildClass == OBJECT_CLUSTER) ||
             (iChildClass == OBJECT_MOBILEDEVICE))
            return true;
         break;
      case OBJECT_NETWORKMAPROOT:
      case OBJECT_NETWORKMAPGROUP:
         if ((iChildClass == OBJECT_NETWORKMAPGROUP) || 
             (iChildClass == OBJECT_NETWORKMAP))
            return true;
         break;
      case OBJECT_DASHBOARDROOT:
      case OBJECT_DASHBOARD:
         if (iChildClass == OBJECT_DASHBOARD)
            return true;
         break;
      case OBJECT_POLICYROOT:
      case OBJECT_POLICYGROUP:
         if ((iChildClass == OBJECT_POLICYGROUP) || 
             (iChildClass == OBJECT_AGENTPOLICY) ||
             (iChildClass == OBJECT_AGENTPOLICY_CONFIG))
            return true;
         break;
      case OBJECT_NODE:
         if ((iChildClass == OBJECT_NETWORKSERVICE) ||
             (iChildClass == OBJECT_VPNCONNECTOR) ||
				 (iChildClass == OBJECT_INTERFACE))
            return true;
         break;
      case OBJECT_CLUSTER:
         if (iChildClass == OBJECT_NODE)
            return true;
         break;
		case OBJECT_BUSINESSSERVICEROOT:
			if ((iChildClass == OBJECT_BUSINESSSERVICE) || 
			    (iChildClass == OBJECT_NODELINK))
            return true;
         break;
		case OBJECT_BUSINESSSERVICE:
			if ((iChildClass == OBJECT_BUSINESSSERVICE) || 
			    (iChildClass == OBJECT_NODELINK) ||
			    (iChildClass == OBJECT_SLMCHECK))
            return true;
         break;
		case OBJECT_NODELINK:
			if (iChildClass == OBJECT_SLMCHECK)
            return true;
         break;
      case -1:    // Creating object without parent
         if (iChildClass == OBJECT_NODE)
            return true;   // OK only for nodes, because parent subnet will be created automatically
         break;
   }

   // Additional check by loaded modules
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfIsValidParentClass != NULL)
		{
			if (g_pModuleList[i].pfIsValidParentClass(iChildClass, iParentClass))
				return true;	// accepted by module
		}
	}

   return false;
}

/**
 * Delete object (final step)
 * This function should be called ONLY from syncer thread
 * Object will be removed from index by ID and destroyed.
 */
void NetObjDelete(NetObj *pObject)
{
	DbgPrintf(4, _T("Final delete step for object %s [%d]"), pObject->getName(), pObject->getId());

   // Delete object from index by ID and object itself
	g_idxObjectById.remove(pObject->getId());
   delete pObject;
}

/**
 * Update interface index when IP address changes
 */
void UpdateInterfaceIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, Interface *iface)
{
	if (IsZoningEnabled())
	{
		Zone *zone = (Zone *)g_idxZoneByGUID.get(iface->getZoneId());
		if (zone != NULL)
		{
			zone->updateInterfaceIndex(oldIpAddr, newIpAddr, iface);
		}
		else
		{
			DbgPrintf(1, _T("UpdateInterfaceIndex: Cannot find zone object for interface %s [%d] (zone id %d)"),
			          iface->getName(), (int)iface->getId(), (int)iface->getZoneId());
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
 * Check if given object is an agent policy object
 */
bool IsAgentPolicyObject(NetObj *object)
{
	return (object->getObjectClass() == OBJECT_AGENTPOLICY) || (object->getObjectClass() == OBJECT_AGENTPOLICY_CONFIG);
}

/**
 * Returns true if object of given class can be event source
 */
bool IsEventSource(int objectClass)
{
	return (objectClass == OBJECT_NODE) || 
	       (objectClass == OBJECT_CONTAINER) || 
	       (objectClass == OBJECT_CLUSTER) || 
			 (objectClass == OBJECT_MOBILEDEVICE);
}
