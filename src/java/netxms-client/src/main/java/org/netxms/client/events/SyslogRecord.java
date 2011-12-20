/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.events;

import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Object representing single syslog record
 */
public class SyslogRecord
{
	// Facility codes
	public static final int KERNEL = 0;
	public static final int USER = 1;
	public static final int MAIL = 2;
	public static final int SYSTEM = 3;
	public static final int AUTH = 4;
	public static final int SYSLOG = 5;
	public static final int LPR = 6;
	public static final int NEWS = 7;
	public static final int UUCP = 8;
	public static final int CRON = 9;
	public static final int SECURITY = 10;
	public static final int FTPD = 11;
	public static final int NTP = 12;
	public static final int LOG_AUDIT = 13;
	public static final int LOG_ALERT = 14;
	public static final int CLOCKD = 15;
	public static final int LOCAL0 = 16;
	public static final int LOCAL1 = 17;
	public static final int LOCAL2 = 18;
	public static final int LOCAL3 = 19;
	public static final int LOCAL4 = 20;
	public static final int LOCAL5 = 21;
	public static final int LOCAL6 = 22;
	public static final int LOCAL7 = 23;
	
	// Severity codes
	public static final int EMERGENCY = 0;
	public static final int ALERT = 1;
	public static final int CRITCAL = 2;
	public static final int ERROR = 3;
	public static final int WARNING = 4;
	public static final int NOTICE = 5;
	public static final int INFORMATIONAL = 6;
	public static final int DEBUG = 7;
	
	private long id;
	private Date timestamp;
	private int facility;
	private int severity;
	private long sourceObjectId;
	private String hostname;
	private String tag;
	private String message;
	
	/**
	 * Create syslog record object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public SyslogRecord(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		timestamp = msg.getVariableAsDate(baseId + 1);
		facility = msg.getVariableAsInteger(baseId + 2);
		severity = msg.getVariableAsInteger(baseId + 3);
		sourceObjectId = msg.getVariableAsInt64(baseId + 4);
		hostname = msg.getVariableAsString(baseId + 5);
		tag = msg.getVariableAsString(baseId + 6);
		message = msg.getVariableAsString(baseId + 7);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the timestamp
	 */
	public Date getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the facility
	 */
	public int getFacility()
	{
		return facility;
	}

	/**
	 * @return the severity
	 */
	public int getSeverity()
	{
		return severity;
	}

	/**
	 * @return the sourceObjectId
	 */
	public long getSourceObjectId()
	{
		return sourceObjectId;
	}

	/**
	 * @return the hostname
	 */
	public String getHostname()
	{
		return hostname;
	}

	/**
	 * @return the tag
	 */
	public String getTag()
	{
		return tag;
	}

	/**
	 * @return the message
	 */
	public String getMessage()
	{
		return message;
	}
}
