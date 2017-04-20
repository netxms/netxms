/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

   virtual void onObjectCreate(NXSL_Object *object);
   virtual void onObjectDelete(NXSL_Object *object);

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Node" class
 */
class NXSL_NodeClass : public NXSL_NetObjClass
{
public:
   NXSL_NodeClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Interface" class
 */
class NXSL_InterfaceClass : public NXSL_NetObjClass
{
public:
   NXSL_InterfaceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "MobileDevice" class
 */
class NXSL_MobileDeviceClass : public NXSL_NetObjClass
{
public:
   NXSL_MobileDeviceClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Chassis" class
 */
class NXSL_ChassisClass : public NXSL_NetObjClass
{
public:
   NXSL_ChassisClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Cluster" class
 */
class NXSL_ClusterClass : public NXSL_NetObjClass
{
public:
   NXSL_ClusterClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Container" class
 */
class NXSL_ContainerClass : public NXSL_NetObjClass
{
public:
   NXSL_ContainerClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Zone" class
 */
class NXSL_ZoneClass : public NXSL_NetObjClass
{
public:
   NXSL_ZoneClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Event" class
 */
class NXSL_EventClass : public NXSL_Class
{
public:
   NXSL_EventClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL "Alarm" class
 */
class NXSL_AlarmClass : public NXSL_Class
{
public:
   NXSL_AlarmClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
	virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * NXSL "DCI" class
 */
class NXSL_DciClass : public NXSL_Class
{
public:
   NXSL_DciClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
   virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * NXSL "SNMP_Transport" class
 */
class NXSL_SNMPTransportClass : public NXSL_Class
{
public:
	NXSL_SNMPTransportClass();

	virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
	virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * "SNMP_VarBind" class
 */
class NXSL_SNMPVarBindClass : public NXSL_Class
{
public:
	NXSL_SNMPVarBindClass();

	virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
	virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * NXSL "Component" class
 */
class NXSL_ComponentClass : public NXSL_Class
{
public:
   NXSL_ComponentClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * Server's default script environment
 */
class NXSL_ServerEnv : public NXSL_Environment
{
protected:
	CONSOLE_CTX m_console;

public:
	NXSL_ServerEnv();

	virtual void print(NXSL_Value *value);
	virtual void trace(int level, const TCHAR *text);

	virtual void configureVM(NXSL_VM *vm);

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

	virtual void print(NXSL_Value *value);
	virtual void trace(int level, const TCHAR *text);
};

class ScheduleParameters;

/**
 * Get server script library
 */
NXSL_Library NXCORE_EXPORTABLE *GetServerScriptLibrary();

/**
 * Create NXSL VM from library script
 */
NXSL_VM NXCORE_EXPORTABLE *CreateServerScriptVM(const TCHAR *name);

/**
 * Functions
 */
void LoadScripts();
void ReloadScript(UINT32 scriptId);
bool IsValidScriptId(UINT32 id);
UINT32 ResolveScriptName(const TCHAR *name);
void CreateScriptExportRecord(String &xml, UINT32 id);
void ImportScript(ConfigEntry *config);
NXSL_VM *FindHookScript(const TCHAR *hookName);
bool ParseValueList(TCHAR **start, ObjectArray<NXSL_Value> &args);

/**
 * Global variables
 */
extern NXSL_AlarmClass g_nxslAlarmClass;
extern NXSL_ChassisClass g_nxslChassisClass;
extern NXSL_ClusterClass g_nxslClusterClass;
extern NXSL_ComponentClass g_nxslComponentClass;
extern NXSL_ContainerClass g_nxslContainerClass;
extern NXSL_DciClass g_nxslDciClass;
extern NXSL_EventClass g_nxslEventClass;
extern NXSL_InterfaceClass g_nxslInterfaceClass;
extern NXSL_MobileDeviceClass g_nxslMobileDeviceClass;
extern NXSL_NetObjClass g_nxslNetObjClass;
extern NXSL_NodeClass g_nxslNodeClass;
extern NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
extern NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
extern NXSL_ZoneClass g_nxslZoneClass;

#endif
