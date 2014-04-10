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
package org.netxms.api.client;

/**
 * Generic client library notifications
 */
public class SessionNotification
{
	// Common notification codes
	public static final int CONNECTION_BROKEN = 1;
	public static final int USER_DB_CHANGED = 3;

	public static final int NOTIFY_BASE = 1000;	// Base value for notifications used as subcode for NXC_EVENT_NOTIFICATION in C library
	public static final int SERVER_SHUTDOWN = 1001;
	public static final int DBCON_STATUS_CHANGED = 1010;
	public static final int MAPPING_TABLE_UPDATED = 1019;
	public static final int MAPPING_TABLE_DELETED = 1020;
	
	// Reporting server notification
	public static final int RS_SCHEDULES_MODIFIED = 1050;
	public static final int RS_RESULTS_MODIFIED = 1051;

	public static final int CUSTOM_MESSAGE = 2000;
	
	// Subcodes for user database changes
	public static final int USER_DB_OBJECT_CREATED = 0;
	public static final int USER_DB_OBJECT_DELETED = 1;
	public static final int USER_DB_OBJECT_MODIFIED = 2;

	protected int code;
	protected long subCode;
	protected Object object;

	/**
	 * @param code
	 * @param object
	 */
	public SessionNotification(int code, Object object)
	{
		this.code = code;
		this.subCode = 0;
		this.object = object;
	}

	/**
	 * @param code
	 * @param subCode
	 */
	public SessionNotification(int code, long subCode)
	{
		this.code = code;
		this.subCode = subCode;
		this.object = null;
	}

	/**
	 * @param code
	 * @param subCode
	 * @param object
	 */
	public SessionNotification(int code, long subCode, Object object)
	{
		this.code = code;
		this.subCode = subCode;
		this.object = object;
	}

	/**
	 * @param code
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
