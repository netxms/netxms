/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.client.snmp;

import java.net.InetAddress;
import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * Object representing single SNMP trap log record
 */
public class SnmpTrapLogRecord
{
	private long id;
	private Date timestamp;
	private InetAddress sourceAddress;
	private long sourceNode;
	private String trapObjectId;
	private String varbinds;
	
	/**
	 * Create record from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 */
	public SnmpTrapLogRecord(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		timestamp = msg.getVariableAsDate(baseId + 1);
		sourceAddress = msg.getVariableAsInetAddress(baseId + 2);
		sourceNode = msg.getVariableAsInt64(baseId + 3);
		trapObjectId = msg.getVariableAsString(baseId + 4);
		varbinds = msg.getVariableAsString(baseId + 5);
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
	 * @return the sourceAddress
	 */
	public InetAddress getSourceAddress()
	{
		return sourceAddress;
	}

	/**
	 * @return the sourceNode
	 */
	public long getSourceNode()
	{
		return sourceNode;
	}

	/**
	 * @return the trapObjectId
	 */
	public String getTrapObjectId()
	{
		return trapObjectId;
	}

	/**
	 * @return the varbinds
	 */
	public String getVarbinds()
	{
		return varbinds;
	}
}
