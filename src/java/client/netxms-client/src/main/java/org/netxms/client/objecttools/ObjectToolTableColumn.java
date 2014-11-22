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
package org.netxms.client.objecttools;

import org.netxms.base.NXCPMessage;

/**
 * Contains information about object tool's table column
 *
 */
public class ObjectToolTableColumn
{
	public static final int FORMAT_STRING   = 0;
	public static final int FORMAT_INTEGER  = 1;
	public static final int FORMAT_FLOAT    = 2;
	public static final int FORMAT_IP_ADDR  = 3;
	public static final int FORMAT_MAC_ADDR = 4;
	public static final int FORMAT_IFINDEX  = 5;
	
	private String name;
	private String snmpOid;
	private int format;
	private int substringIndex;
	
	/**
	 * Create new column object
	 * 
	 * @param name column name
	 */
	public ObjectToolTableColumn(String name)
	{
		this.name = name;
		this.snmpOid = "";
		this.format = FORMAT_STRING;
		this.substringIndex = 1;
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public ObjectToolTableColumn(ObjectToolTableColumn src)
	{
		this.name = src.name;
		this.snmpOid = src.snmpOid;
		this.format = src.format;
		this.substringIndex = src.substringIndex;
	}
	
	/**
	 * Create column info object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	protected ObjectToolTableColumn(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		snmpOid = msg.getFieldAsString(baseId + 1);
		format = msg.getFieldAsInt32(baseId + 2);
		substringIndex = msg.getFieldAsInt32(baseId + 3);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the snmpOid
	 */
	public String getSnmpOid()
	{
		return snmpOid;
	}

	/**
	 * @param snmpOid the snmpOid to set
	 */
	public void setSnmpOid(String snmpOid)
	{
		this.snmpOid = snmpOid;
	}

	/**
	 * @return the format
	 */
	public int getFormat()
	{
		return format;
	}

	/**
	 * @param format the format to set
	 */
	public void setFormat(int format)
	{
		this.format = format;
	}

	/**
	 * @return the substringIndex
	 */
	public int getSubstringIndex()
	{
		return substringIndex;
	}

	/**
	 * @param substringIndex the substringIndex to set
	 */
	public void setSubstringIndex(int substringIndex)
	{
		this.substringIndex = substringIndex;
	}
}
