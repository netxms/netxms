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
package org.netxms.client.snmp;

/**
 * Value of SNMP object
 */
public class SnmpValue
{
	private String name;
	private SnmpObjectId objectId;
	private int type;
	private String value;
	private long nodeId;
	
	/**
	 * @param name
	 * @param type
	 * @param value
	 */
	public SnmpValue(String name, int type, String value, long nodeId)
	{
		this.name = name;
		this.objectId = null;
		this.type = type;
		this.value = value;
		this.nodeId = nodeId;
	}
	
	/**
	 * @param objectId
	 * @param type
	 * @param value
	 */
	public SnmpValue(SnmpObjectId objectId, int type, String value, long nodeId)
	{
		this.name = objectId.toString();
		this.objectId = objectId;
		this.type = type;
		this.value = value;
		this.nodeId = nodeId;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the objectId
	 */
	public SnmpObjectId getObjectId()
	{
		if (objectId == null)
		{
			try
			{
				objectId = SnmpObjectId.parseSnmpObjectId(name);
			}
			catch(SnmpObjectIdFormatException e)
			{
				objectId = new SnmpObjectId();
			}
		}
		return objectId;
	}

	/**
	 * Get object type. Note that this is an ASN.1 identifier type (defined in {@link org.netxms.client.constants.ASN}}), not type of MibObject
	 * 
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * @return the nodeId
	 */
	public final long getNodeId()
	{
		return nodeId;
	}
}
