/**
 * Client library notification
 */
package org.netxms.client;

/**
 * @author victor
 *
 */
public class NXCNotification
{
	// Notification codes
	public static final int CONNECTION_BROKEN = 1;
	public static final int NEW_EVENTLOG_RECORD = 2;
	public static final int USER_DB_CHANGED = 3;
	public static final int OBJECT_CHANGED = 4;
	public static final int DEPLOYMENT_STATUS = 6;
	public static final int NEW_SYSLOG_RECORD = 7;
	public static final int NEW_SNMP_TRAP = 8;
	public static final int SITUATION_UPDATE = 9;
	public static final int JOB_CHANGE = 10;
	
	public static final int NOTIFY_BASE = 1000;	// Base value for notifications used as subcode for NXC_EVENT_NOTIFICATION in C library
	public static final int SERVER_SHUTDOWN = 1001;
	public static final int EVENT_DB_CHANGED = 1002;
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
	
	public static final int CUSTOM_MESSAGE = 2000;
	
	// Subcodes for user database changes
	public static final int USER_DB_OBJECT_CREATED = 0;
	public static final int USER_DB_OBJECT_DELETED = 1;
	public static final int USER_DB_OBJECT_MODIFIED = 2;
	
	// Notification object data
	private int code;
	private long subCode;
	private Object object;

	
	/**
	 * @param code
	 * @param object
	 */
	public NXCNotification(int code, Object object)
	{
		this.code = code;
		this.subCode = 0;
		this.object = object;
	}

	/**
	 * @param code
	 * @param subCode
	 */
	public NXCNotification(int code, long subCode)
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
	public NXCNotification(int code, long subCode, Object object)
	{
		this.code = code;
		this.subCode = subCode;
		this.object = object;
	}

	/**
	 * @param code
	 */
	public NXCNotification(int code)
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
