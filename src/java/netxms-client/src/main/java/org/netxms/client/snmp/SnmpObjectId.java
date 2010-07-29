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
package org.netxms.client.snmp;

import java.util.Arrays;

/**
 * This class represents SNMP Object Id (OID)
 *
 */
public class SnmpObjectId
{
	private long[] value;
	
	/**
	 * Create empty OID
	 */
	public SnmpObjectId()
	{
		value = new long[0];
	}
	
	/**
	 * Create OID from array of OID elements
	 * @param value OID elements
	 */
	public SnmpObjectId(long[] value)
	{
		this.value = Arrays.copyOf(value, value.length);
	}
	
	/**
	 * Parse string argument as SNMP OID. Expected to be ion format .n.n.n.n.n
	 * @param s string to parse
	 * @return SNMP OID object
	 * @throws SnmpObjectIdFormatException when string passed is not a valid SNMP OID
	 */
	public static SnmpObjectId parseSnmpObjectId(String s) throws SnmpObjectIdFormatException
	{
		if (s.trim().charAt(0) != '.')
			throw new SnmpObjectIdFormatException("OID shoud start with . character");
			
		String[] parts = s.split("\\.");
		long[] value = new long[parts.length - 1];
		for(int i = 1; i < parts.length; i++)
		{
			String p = parts[i].trim();
			if (p.isEmpty())
				throw new SnmpObjectIdFormatException("Empty OID element");
			try
			{
				value[i - 1] = Long.parseLong(p);
			}
			catch(NumberFormatException e)
			{
				throw new SnmpObjectIdFormatException("OID element #" + i + " is not a whole number");
			}
		}
		return new SnmpObjectId(value);
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (!(obj instanceof SnmpObjectId))
			return false;
		return Arrays.equals(value, ((SnmpObjectId)obj).value);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		return Arrays.hashCode(value);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		final StringBuilder sb = new StringBuilder(value.length * 3);
		for(int i = 0; i < value.length; i++)
		{
			sb.append('.');
			sb.append(value[i]);
		}
		return sb.toString();
	}
}
