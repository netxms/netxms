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
package org.netxms.ui.eclipse.snmp;

import java.util.HashMap;
import java.util.Map;

/**
 * SNMP-related constants
 *
 */
public class SnmpConstants
{
	private static final String[] mibObjectStatus = { "", "Mandatory", "Optional", "Obsolete", "Deprecated", "Current" };
	private static final String[] mibObjectAccess = { "", "Read", "Read/Write", "Write", "None", "Notify", "Create" };
	private static final String[] mibObjectType =
	{
		"Other", "Import Item", "Object ID", "Bit String", "Integer", "Integer 32bits",
		"Integer 64bits", "Unsigned Int 32bits", "Counter", "Counter 32bits", "Counter 64bits",
		"Gauge", "Gauge 32bits", "Timeticks", "Octet String", "Opaque", "IP Address",
		"Physical Address", "Network Address", "Named Type", "Sequence ID", "Sequence",
		"Choice", "Textual Convention", "Macro", "MODCOMP", "Trap", "Notification",
		"Module ID", "NSAP Address", "Agent Capability", "Unsigned Integer", "Null",
		"Object Group", "Notification Group"
	};
	private static final Map<Integer, String> asnType = new HashMap<Integer, String>();
	
	static
	{
		asnType.put(0x02, "INTEGER");
		asnType.put(0x03, "BIT STRING");
		asnType.put(0x04, "STRING");
		asnType.put(0x05, "NULL");
		asnType.put(0x06, "OBJECT IDENTIFIER");
		asnType.put(0x30, "SEQUENCE");
		asnType.put(0x40, "IP ADDRESS");
		asnType.put(0x41, "COUNTER32");
		asnType.put(0x42, "GAUGE32");
		asnType.put(0x43, "TIMETICKS");
		asnType.put(0x44, "OPAQUE");
		asnType.put(0x45, "NSAP ADDRESS");
		asnType.put(0x46, "COUNTER64");
		asnType.put(0x47, "UINTEGER32");
	}

	/**
	 * Get text for code safely.
	 * 
	 * @param texts Array with texts
	 * @param code Code to get text for
	 * @param defaultValue Default value
	 * @return text for given code or default value in case of error
	 */
	private static String safeGetText(String[] texts, int code, String defaultValue)
	{
		String result;
		try
		{
			result = texts[code];
		}
		catch(Exception e)
		{
			result = defaultValue;
		}
		return result;
	}

	/**
	 * Get MIB object status name
	 * 
	 * @param status
	 * @return
	 */
	public static String getObjectStatusName(int status)
	{
		return safeGetText(mibObjectStatus, status, "Unknown");
	}

	/**
	 * Get MIB object access name
	 * 
	 * @param access
	 * @return
	 */
	public static String getObjectAccessName(int access)
	{
		return safeGetText(mibObjectAccess, access, "Unknown");
	}
	
	/**
	 * Get MIB object type name
	 * 
	 * @param type
	 * @return
	 */
	public static String getObjectTypeName(int type)
	{
		return safeGetText(mibObjectType, type, "Unknown");
	}
	
	/**
	 * Get ASN.1 type name
	 * 
	 * @param type
	 * @return
	 */
	public static String getAsnTypeName(int type)
	{
		String name = asnType.get(type);
		if (name == null)
			name = "0x" + Integer.toHexString(type);
		return name;
	}
}
