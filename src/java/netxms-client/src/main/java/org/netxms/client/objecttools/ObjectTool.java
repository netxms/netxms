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
 * NetXMS object tool representation
 *
 */
public class ObjectTool
{
	public static final int TYPE_INTERNAL    = 0;
	public static final int TYPE_ACTION      = 1;
	public static final int TYPE_TABLE_SNMP  = 2;
	public static final int TYPE_TABLE_AGENT = 3;
	public static final int TYPE_URL         = 4;
	public static final int TYPE_COMMAND     = 5;
	
	public static final int REQUIRES_SNMP         = 0x00000001;
	public static final int REQUIRES_AGENT        = 0x00000002;
	public static final int REQUIRES_OID_MATCH    = 0x00000004;
	public static final int ASK_CONFIRMATION      = 0x00000008;
	public static final int SNMP_INDEXED_BY_VALUE = 0x00010000;
	
	protected long id;
	protected String name;
	protected int type;
	protected int flags;
	protected String description;
	protected String snmpOid;
	protected String data;
	protected String confirmationText;

	/**
	 * Default implicit constructor.
	 */
	protected ObjectTool()
	{
	}
	
	/**
	 * Create object tool from NXCP message. Intended to be called only by NXCSession methods.
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	public ObjectTool(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		name = msg.getVariableAsString(baseId + 1);
		type = msg.getVariableAsInteger(baseId + 2);
		data = msg.getVariableAsString(baseId + 3);
		flags = msg.getVariableAsInteger(baseId + 4);
		description = msg.getVariableAsString(baseId + 5);
		snmpOid = msg.getVariableAsString(baseId + 6);
		confirmationText = msg.getVariableAsString(baseId + 7);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the snmpOid
	 */
	public String getSnmpOid()
	{
		return snmpOid;
	}

	/**
	 * @return the data
	 */
	public String getData()
	{
		return data;
	}

	/**
	 * @return the confirmationText
	 */
	public String getConfirmationText()
	{
		return confirmationText;
	}
}
