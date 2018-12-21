/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client;

/**
 * Generic client library notifications
 */
public class SessionNotification
{
	// Common notification codes
	public static final int CONNECTION_BROKEN = 1;
   public static final int NEW_EVENTLOG_RECORD = 2;
	public static final int USER_DB_CHANGED = 3;
   public static final int OBJECT_CHANGED = 4;
   public static final int DEPLOYMENT_STATUS = 6;
   public static final int NEW_SYSLOG_RECORD = 7;
   public static final int NEW_SNMP_TRAP = 8;
   public static final int JOB_CHANGE = 10;
   public static final int IMAGE_LIBRARY_CHANGED = 11;
   public static final int OBJECT_DELETED = 99;

//   public static final int SITUATION_BASE = 100;
//   public static final int SITUATION_CREATED = 101;
//   public static final int SITUATION_DELETED = 102;
//   public static final int SITUATION_UPDATED = 103;
//   public static final int SITUATION_INSTANCE_UPDATED = 104;
//   public static final int SITUATION_INSTANCE_DELETED = 105;
   
   public static final int IMAGE_UPDATED = 201;
   public static final int IMAGE_DELETED = 202;
   
	public static final int NOTIFY_BASE = 1000;	// Base value for notifications used as subcode for NXC_EVENT_NOTIFICATION in C library
	public static final int SERVER_SHUTDOWN = 1001;
   public static final int RELOAD_EVENT_DB = 1002;
   public static final int ALARM_DELETED = 1003;
   public static final int NEW_ALARM = 1004;
   public static final int ALARM_CHANGED = 1005;
   public static final int ACTION_CREATED = 1006;
   public static final int ACTION_MODIFIED = 1007;
   public static final int ACTION_DELETED = 1008;
   public static final int OBJECT_TOOLS_CHANGED = 1009;
	public static final int DBCON_STATUS_CHANGED = 1010;
   public static final int ALARM_TERMINATED = 1011;
   public static final int PREDEFINED_GRAPHS_CHANGED = 1012;
   public static final int EVENT_TEMPLATE_MODIFIED = 1013;
   public static final int EVENT_TEMPLATE_DELETED = 1014;
   public static final int OBJECT_TOOL_DELETED = 1015;
   public static final int TRAP_CONFIGURATION_CREATED = 1016;
   public static final int TRAP_CONFIGURATION_MODIFIED = 1017;
   public static final int TRAP_CONFIGURATION_DELETED = 1018;
	public static final int MAPPING_TABLE_UPDATED = 1019;
	public static final int MAPPING_TABLE_DELETED = 1020;
   public static final int DCI_SUMMARY_TABLE_UPDATED = 1021;
   public static final int DCI_SUMMARY_TABLE_DELETED = 1022;
   public static final int DCI_CERTIFICATE_CHANGED = 1023;
   public static final int ALARM_STATUS_FLOW_CHANGED = 1024;
   public static final int FILE_LIST_CHANGED = 1025;
   public static final int FILE_MONITORING_FAILED = 1026;
   public static final int SESSION_KILLED = 1027;
   public static final int PREDEFINED_GRAPHS_DELETED = 1028;
   public static final int SCHEDULE_UPDATE = 1029;
   public static final int ALARM_CATEGORY_UPDATED = 1030;
   public static final int ALARM_CATEGORY_DELETED = 1031;
   public static final int MULTIPLE_ALARMS_TERMINATED = 1032;
   public static final int MULTIPLE_ALARMS_RESOLVED = 1033;
   public static final int FORCE_DCI_POLL = 1034;
   public static final int DCI_UPDATE = 1035;
   public static final int DCI_DELETE = 1036;
   public static final int DCI_STATE_CHANGE = 1037;
   public static final int OBJECTS_OUT_OF_SYNC = 1038;
   public static final int POLICY_MODIFIED = 1039;
   public static final int POLICY_DELETED = 1040;
	
	public static final int CUSTOM_MESSAGE = 2000;
   public static final int OBJECT_SYNC_COMPLETED = 2001;
   public static final int USER_DISCONNECT = 2002;
	
   // Reporting server notification
   public static final int RS_SCHEDULES_MODIFIED = 3000;
   public static final int RS_RESULTS_MODIFIED = 3001;

	// Subcodes for user database changes
	public static final int USER_DB_OBJECT_CREATED = 0;
	public static final int USER_DB_OBJECT_DELETED = 1;
	public static final int USER_DB_OBJECT_MODIFIED = 2;

   protected static final int UPDATE_LISTENER_LIST = 32766;
   protected static final int STOP_PROCESSING_THREAD = 32767;
	
	protected int code;
	protected long subCode;
	protected Object object;

	/**
	 * @param code The notification code
	 * @param object The notification object
	 */
	public SessionNotification(int code, Object object)
	{
		this.code = code;
		this.subCode = 0;
		this.object = object;
	}

	/**
	 * @param code The notification code
	 * @param subCode The notification subcode
	 */
	public SessionNotification(int code, long subCode)
	{
		this.code = code;
		this.subCode = subCode;
		this.object = null;
	}

	/**
	 * @param code The notification code
	 * @param subCode The notification subcode
	 * @param object The notification object
	 */
	public SessionNotification(int code, long subCode, Object object)
	{
		this.code = code;
		this.subCode = subCode;
		this.object = object;
	}

	/**
	 * @param code The notification code
	 */
	public SessionNotification(int code)
	{
		this.code = code;
		this.subCode = 0;
		this.object = null;
	}

	/**
	 * @return Notification's code
	 */
	public final int getCode()
	{
		return code;
	}

	/**
	 * @return Notification's subcode
	 */
	public final long getSubCode()
	{
		return subCode;
	}

	/**
	 * @return Object associated with notification
	 */
	public final Object getObject()
	{
		return object;
	}
}
