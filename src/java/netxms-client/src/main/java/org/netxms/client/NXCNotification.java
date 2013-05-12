/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import org.netxms.api.client.SessionNotification;

/**
 * Client library notification
 */
public class NXCNotification extends SessionNotification
{
	// Notification codes
	public static final int NEW_EVENTLOG_RECORD = 2;
	public static final int OBJECT_CHANGED = 4;
	public static final int DEPLOYMENT_STATUS = 6;
	public static final int NEW_SYSLOG_RECORD = 7;
	public static final int NEW_SNMP_TRAP = 8;
	public static final int JOB_CHANGE = 10;
	public static final int IMAGE_LIBRARY_CHANGED = 11;
	public static final int DCI_SUMMARY_TABLE_UPDATED = 21;
	public static final int DCI_SUMMARY_TABLE_DELETED = 22;
	public static final int OBJECT_DELETED = 99;

	public static final int SITUATION_BASE = 100;
	public static final int SITUATION_CREATED = 101;
	public static final int SITUATION_DELETED = 102;
	public static final int SITUATION_UPDATED = 103;
	public static final int SITUATION_INSTANCE_UPDATED = 104;
	public static final int SITUATION_INSTANCE_DELETED = 105;
	
	public static final int EVENT_DB_CHANGED = 1002;
	public static final int ALARM_DELETED = 1003;
	public static final int NEW_ALARM = 1004;
	public static final int ALARM_CHANGED = 1005;
	public static final int ACTION_CREATED = 1006;
	public static final int ACTION_MODIFIED = 1007;
	public static final int ACTION_DELETED = 1008;
	public static final int OBJECT_TOOLS_CHANGED = 1009;
	public static final int ALARM_TERMINATED = 1011;
	public static final int PREDEFINED_GRAPHS_CHANGED = 1012;
	public static final int EVENT_TEMPLATE_MODIFIED = 1013;
	public static final int EVENT_TEMPLATE_DELETED = 1014;
	public static final int OBJECT_TOOL_DELETED = 1015;
	public static final int TRAP_CONFIGURATION_CREATED = 1016;
	public static final int TRAP_CONFIGURATION_MODIFIED = 1017;
	public static final int TRAP_CONFIGURATION_DELETED = 1018;
	public static final int IMAGE_UPDATED = 1021;
	public static final int IMAGE_DELETED = 1022;

	public static final int OBJECT_SYNC_COMPLETED = 2001;
	
	/**
	 * @param code
	 * @param subCode
	 * @param object
	 */
	public NXCNotification(int code, long subCode, Object object)
	{
		super(code, subCode, object);
	}

	/**
	 * @param code
	 * @param subCode
	 */
	public NXCNotification(int code, long subCode)
	{
		super(code, subCode);
	}

	/**
	 * @param code
	 * @param object
	 */
	public NXCNotification(int code, Object object)
	{
		super(code, object);
	}

	/**
	 * @param code
	 */
	public NXCNotification(int code)
	{
		super(code);
	}
}
