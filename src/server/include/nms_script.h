/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nms_script.h
**
**/

#ifndef _nms_script_h_
#define _nms_script_h_

/**
 * NXSL "NetObj" class
 */
class NXSL_NetObjClass : public NXSL_Class
{
public:
   NXSL_NetObjClass();

   virtual void onObjectDelete(NXSL_Object *object) override;
   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual json_t *toJson(NXSL_Object *object, int depth) override;
};

/**
 * NXSL "Subnet" class
 */
class NXSL_SubnetClass : public NXSL_NetObjClass
{
public:
   NXSL_SubnetClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "DataCollectionTarget" class
 */
class NXSL_DCTargetClass : public NXSL_NetObjClass
{
public:
   NXSL_DCTargetClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Node" class
 */
class NXSL_NodeClass : public NXSL_DCTargetClass
{
public:
   NXSL_NodeClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "NetworkMap" class
 */
class NXSL_NetworkMapClass : public NXSL_NetObjClass
{
public:
   NXSL_NetworkMapClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Interface" class
 */
class NXSL_InterfaceClass : public NXSL_NetObjClass
{
public:
   NXSL_InterfaceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "AccessPoint" class
 */
class NXSL_AccessPointClass : public NXSL_DCTargetClass
{
public:
   NXSL_AccessPointClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "RadioInterface" class
 */
class NXSL_RadioInterfaceClass : public NXSL_Class
{
public:
   NXSL_RadioInterfaceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "WirelessStation" class
 */
class NXSL_WirelessStationClass : public NXSL_NetObjClass
{
public:
   NXSL_WirelessStationClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "MobileDevice" class
 */
class NXSL_MobileDeviceClass : public NXSL_DCTargetClass
{
public:
   NXSL_MobileDeviceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Chassis" class
 */
class NXSL_ChassisClass : public NXSL_DCTargetClass
{
public:
   NXSL_ChassisClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Cluster" class
 */
class NXSL_ClusterClass : public NXSL_DCTargetClass
{
public:
   NXSL_ClusterClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Container" class
 */
class NXSL_ContainerClass : public NXSL_NetObjClass
{
public:
   NXSL_ContainerClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Collector" class
 */
class NXSL_CollectorClass : public NXSL_DCTargetClass
{
public:
   NXSL_CollectorClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Circuit" class
 */
class NXSL_CircuitClass : public NXSL_DCTargetClass
{
public:
   NXSL_CircuitClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "ServiceRoot" class
 */
class NXSL_ServiceRootClass : public NXSL_NetObjClass
{
public:
   NXSL_ServiceRootClass();
};

/**
 * NXSL "Template" class
 */
class NXSL_TemplateClass : public NXSL_NetObjClass
{
public:
   NXSL_TemplateClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "WirelessDomain" class
 */
class NXSL_WirelessDomainClass : public NXSL_NetObjClass
{
public:
   NXSL_WirelessDomainClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Zone" class
 */
class NXSL_ZoneClass : public NXSL_NetObjClass
{
public:
   NXSL_ZoneClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Sensor" class
 */
class NXSL_SensorClass : public NXSL_DCTargetClass
{
public:
   NXSL_SensorClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "Asset" class
 */
class NXSL_AssetClass : public NXSL_NetObjClass
{
public:
   NXSL_AssetClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "AssetProperties" class
 */
class NXSL_AssetPropertiesClass : public NXSL_Class
{
public:
   NXSL_AssetPropertiesClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "BusinessService" class
 */
class NXSL_BusinessServiceClass : public NXSL_NetObjClass
{
public:
   NXSL_BusinessServiceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "BusinessServiceCheck" class
 */
class NXSL_BusinessServiceCheckClass : public NXSL_Class
{
public:
   NXSL_BusinessServiceCheckClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "Event" class
 */
class NXSL_EventClass : public NXSL_Class
{
public:
   NXSL_EventClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual bool setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "Alarm" class
 */
class NXSL_AlarmClass : public NXSL_Class
{
public:
   NXSL_AlarmClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "AlarmComment" class
 */
class NXSL_AlarmCommentClass : public NXSL_Class
{
public:
   NXSL_AlarmCommentClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "DataPoint" class
 */
class NXSL_DataPointClass : public NXSL_Class
{
public:
   NXSL_DataPointClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "DCI" class
 */
class NXSL_DciClass : public NXSL_Class
{
public:
   NXSL_DciClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "ScoredDciValue" class
 */
class NXSL_ScoredDciValueClass : public NXSL_Class
{
public:
   NXSL_ScoredDciValueClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "SNMP_Transport" class
 */
class NXSL_SNMPTransportClass : public NXSL_Class
{
public:
   NXSL_SNMPTransportClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * "SNMP_VarBind" class
 */
class NXSL_SNMPVarBindClass : public NXSL_Class
{
public:
   NXSL_SNMPVarBindClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "NodeDependency" class
 */
class NXSL_NodeDependencyClass : public NXSL_Class
{
public:
   NXSL_NodeDependencyClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * Server's default script environment
 */
class NXCORE_EXPORTABLE NXSL_ServerEnv : public NXSL_Environment
{
protected:
   CONSOLE_CTX m_console;

public:
   NXSL_ServerEnv(bool enableSqlFunctions = false);

   virtual void print(const TCHAR *text) override;
   virtual void trace(int level, const TCHAR *text) override;

   virtual void configureVM(NXSL_VM *vm) override;
   virtual NXSL_Value *getConstantValue(const NXSL_Identifier& name, NXSL_ValueManager *vm) override;

   void setConsole(CONSOLE_CTX console) { m_console = console; }
};

/**
 * Script environment with output redirected to client session
 */
class NXSL_ClientSessionEnv : public NXSL_ServerEnv
{
protected:
   ClientSession *m_session;
   NXCPMessage *m_response;

public:
   NXSL_ClientSessionEnv(ClientSession *session, NXCPMessage *response);

   virtual void print(const TCHAR *text) override;
   virtual void trace(int level, const TCHAR *text) override;
};

/**
 * NXSL "ClientSession" class
 */
class NXSL_ClientSessionClass : public NXSL_Class
{
public:
   NXSL_ClientSessionClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "UserDBObject" class
 */
class NXSL_UserDBObjectClass : public NXSL_Class
{
public:
   NXSL_UserDBObjectClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * NXSL "User" class
 */
class NXSL_UserClass : public NXSL_UserDBObjectClass
{
public:
   NXSL_UserClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL class "UserGroup"
 */
class NXSL_UserGroupClass : public NXSL_UserDBObjectClass
{
public:
   NXSL_UserGroupClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL class "VLAN"
 */
class NXSL_VlanClass : public NXSL_Class
{
public:
   NXSL_VlanClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL class "WebService"
 */
class NXSL_WebServiceClass : public NXSL_Class
{
public:
   NXSL_WebServiceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL class "WebServiceResponse"
 */
class NXSL_WebServiceResponseClass : public NXSL_Class
{
public:
   NXSL_WebServiceResponseClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL class "Tunnel"
 */
class NXSL_TunnelClass : public NXSL_Class
{
public:
   NXSL_TunnelClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * Hardware component information for NXSL
 */
class NXSL_HardwareComponent : public NXSL_Class
{
public:
   NXSL_HardwareComponent();

   virtual NXSL_Value* getAttr(NXSL_Object* object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object* object) override;
};

/**
 * Software package information for NXSL
 */
class NXSL_SoftwarePackage : public NXSL_Class
{
public:
   NXSL_SoftwarePackage();

   virtual NXSL_Value* getAttr(NXSL_Object* object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object* object) override;
};

/**
 * Maintenance journal record
 */
class NXSL_MaintenanceJournalRecordClass : public NXSL_Class
{
public:
   NXSL_MaintenanceJournalRecordClass();

   virtual NXSL_Value* getAttr(NXSL_Object* object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object* object) override;
   virtual void toString(StringBuffer *sb, NXSL_Object *object) override;
};

/**
 * OSPF area information for NXSL
 */
class NXSL_OSPFAreaClass : public NXSL_Class
{
public:
   NXSL_OSPFAreaClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * OSPF neighbor information for NXSL
 */
class NXSL_OSPFNeighborClass : public NXSL_Class
{
public:
   NXSL_OSPFNeighborClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * Container class for network map link
 */
class NXSL_NetworkMapLinkClass : public NXSL_Class
{
public:
   NXSL_NetworkMapLinkClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
};

/**
 * Link data source class
 */
class NXSL_LinkDataSourceClass : public NXSL_Class
{
public:
   NXSL_LinkDataSourceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "DowntimeInfo" class
 */
class NXSL_DowntimeInfoClass : public NXSL_Class
{
public:
   NXSL_DowntimeInfoClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL "NetworkPathCheckResult" class
 */
class NXSL_NetworkPathCheckResultClass : public NXSL_Class
{
public:
   NXSL_NetworkPathCheckResultClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

class ScheduleParameters;

/**
 * Get server script library
 */
NXSL_Library NXCORE_EXPORTABLE *GetServerScriptLibrary();

/**
 * Setup server script VM. Returns pointer to same VM for convenience.
 */
NXSL_VM NXCORE_EXPORTABLE *SetupServerScriptVM(NXSL_VM *vm, const shared_ptr<NetObj>& object, const shared_ptr<DCObjectInfo>& dciInfo);

/**
 * Create NXSL VM from library script
 */
ScriptVMHandle NXCORE_EXPORTABLE CreateServerScriptVM(const TCHAR *name, const shared_ptr<NetObj>& object, const shared_ptr<DCObjectInfo>& dciInfo = shared_ptr<DCObjectInfo>());

/**
 * Create NXSL VM from compiled script
 */
ScriptVMHandle NXCORE_EXPORTABLE CreateServerScriptVM(const NXSL_Program *script, const shared_ptr<NetObj>& object, const shared_ptr<DCObjectInfo>& dciInfo = shared_ptr<DCObjectInfo>());

/**
 * Compile server script and report error on failure
 */
NXSL_Program NXCORE_EXPORTABLE *CompileServerScript(const TCHAR *source, const TCHAR *context, const NetObj *object, uint32_t dciId, const TCHAR *nameFormat, ...);

/**
 * Report script error
 */
void NXCORE_EXPORTABLE ReportScriptError(const TCHAR *context, const NetObj *object, uint32_t dciId, const TCHAR *errorText, const TCHAR *nameFormat, ...);

/**
 * Extract entry point separated by dot or slash from script name.
 * Will update "name" argument to have only script name and place entry point
 * into "entryPoint". Will set "entryPoint" to empty string if entry point
 * is not part of script name.
 */
void NXCORE_EXPORTABLE ExtractScriptEntryPoint(wchar_t *name, char *entryPoint);

class ImportContext;

/**
 * Functions
 */
void LoadScripts();
void ValidateScripts();
void ReloadScript(uint32_t scriptId);
bool IsValidScriptId(uint32_t id);
uint32_t ResolveScriptName(const TCHAR *name);
bool GetScriptName(uint32_t scriptId, TCHAR *buffer, size_t size);
void CreateScriptExportRecord(TextFileWriter& xml, uint32_t id);
void ImportScript(ConfigEntry *config, bool overwrite, ImportContext *context, bool nxslV5);
ScriptVMHandle NXCORE_EXPORTABLE FindHookScript(const TCHAR *hookName, shared_ptr<NetObj> object);
bool NXCORE_EXPORTABLE ParseValueList(NXSL_VM *vm, TCHAR **start, ObjectRefArray<NXSL_Value> &args, bool hasBrackets);

/**
 * Global variables
 */
extern NXSL_AccessPointClass g_nxslAccessPointClass;
extern NXSL_AlarmClass g_nxslAlarmClass;
extern NXSL_AlarmCommentClass g_nxslAlarmCommentClass;
extern NXSL_AssetClass g_nxslAssetClass;
extern NXSL_AssetPropertiesClass g_nxslAssetPropertiesClass;
extern NXSL_BusinessServiceClass g_nxslBusinessServiceClass;
extern NXSL_BusinessServiceCheckClass g_nxslBusinessServiceCheckClass;
extern NXSL_ChassisClass g_nxslChassisClass;
extern NXSL_CircuitClass g_nxslCircuitClass;
extern NXSL_ClientSessionClass g_nxslClientSessionClass;
extern NXSL_ClusterClass g_nxslClusterClass;
extern NXSL_CollectorClass g_nxslCollectorClass;
extern NXSL_ContainerClass g_nxslContainerClass;
extern NXSL_DataPointClass g_nxslDataPointClass;
extern NXSL_DciClass g_nxslDciClass;
extern NXSL_DowntimeInfoClass g_nxslDowntimeInfoClass;
extern NXSL_EventClass g_nxslEventClass;
extern NXSL_HardwareComponent g_nxslHardwareComponent;
extern NXSL_InterfaceClass g_nxslInterfaceClass;
extern NXSL_LinkDataSourceClass g_nxslLinkDataSourceClass;
extern NXSL_MaintenanceJournalRecordClass g_nxslMaintenanceJournalRecordClass;
extern NXSL_MobileDeviceClass g_nxslMobileDeviceClass;
extern NXSL_NetObjClass g_nxslNetObjClass;
extern NXSL_NetworkMapClass g_nxslNetworkMapClass;
extern NXSL_NetworkMapLinkClass g_nxslNetworkMapLinkClass;
extern NXSL_NetworkPathCheckResultClass g_nxslNetworkPathCheckResultClass;
extern NXSL_NodeClass g_nxslNodeClass;
extern NXSL_NodeDependencyClass g_nxslNodeDependencyClass;
extern NXSL_OSPFAreaClass g_nxslOSPFAreaClass;
extern NXSL_OSPFNeighborClass g_nxslOSPFNeighborClass;
extern NXSL_RadioInterfaceClass g_nxslRadioInterfaceClass;
extern NXSL_ScoredDciValueClass g_nxslScoredDciValueClass;
extern NXSL_SensorClass g_nxslSensorClass;
extern NXSL_ServiceRootClass g_nxslServiceRootClass;
extern NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
extern NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
extern NXSL_SoftwarePackage g_nxslSoftwarePackage;
extern NXSL_SubnetClass g_nxslSubnetClass;
extern NXSL_TemplateClass g_nxslTemplateClass;
extern NXSL_TunnelClass g_nxslTunnelClass;
extern NXSL_UserDBObjectClass g_nxslUserDBObjectClass;
extern NXSL_UserClass g_nxslUserClass;
extern NXSL_UserGroupClass g_nxslUserGroupClass;
extern NXSL_VlanClass g_nxslVlanClass;
extern NXSL_WebServiceClass g_nxslWebServiceClass;
extern NXSL_WebServiceResponseClass g_nxslWebServiceResponseClass;
extern NXSL_WirelessDomainClass g_nxslWirelessDomainClass;
extern NXSL_WirelessStationClass g_nxslWirelessStationClass;
extern NXSL_ZoneClass g_nxslZoneClass;

#endif
