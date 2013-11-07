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
import org.netxms.base.NXCPMessage;

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
	 * Create child OID - add given value at the end of parent OID.
	 * @param value parent OID
	 */
	public SnmpObjectId(SnmpObjectId parent, long id)
	{
		if (parent != null)
			value = Arrays.copyOf(parent.value, parent.value.length + 1);
		else
			value = new long[1];
		value[value.length - 1] = id;
	}

	/**
	 * Parse string argument as SNMP OID. Expected to be ion format .n.n.n.n.n
	 * @param s string to parse
	 * @return SNMP OID object
	 * @throws SnmpObjectIdFormatException when string passed is not a valid SNMP OID
	 */
	public static SnmpObjectId parseSnmpObjectId(String s) throws SnmpObjectIdFormatException
	{
		String st = s.trim();
		if (st.isEmpty())
			return new SnmpObjectId();	// OID with length 0
		
		if (st.charAt(0) != '.')
			throw new SnmpObjectIdFormatException("OID shoud start with . character");
			
		String[] parts = st.split("\\.");
		if (parts.length == 0)
			throw new SnmpObjectIdFormatException("Empty OID element");
		
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
	
	/**
	 * Get object identifier length.
	 * 
	 * @return OID length
	 */
	public int getLength()
	{
		return value.length;
	}
	
	/**
	 * Set NXCP variable to OID's value
	 *  
	 * @param msg NXCP message
	 * @param varId Variable ID
	 */
	public void setNXCPVariable(NXCPMessage msg, long varId)
	{
		msg.setVariable(varId, value);
	}
	
	/**
	 * Check if this object id starts with given object id.
	 * 
	 * @param oid Object id to check
	 * @return true if this object id starts with given object id
	 */
	public boolean startsWith(SnmpObjectId oid)
	{
		if (oid.value.length > value.length)
			return false;
		
		for(int i = 0; i < oid.value.length; i++)
			if (oid.value[i] != value[i])
				return false;
		return true;
	}
	
	/**
	 * Compare two object identifiers. The return value is -1 if this object identifier is
	 * less than given object identifier, 0 if they are equal, and 1 if this object identifier is greater
	 * then given object identifier.
	 *  
	 * @param oid object identifier to compare with
	 * @return -1, 0, or 1 (see above for details)
	 */
	public int compareTo(final SnmpObjectId oid)
	{
		int maxLen = Math.min(value.length, oid.value.length);
		for(int i = 0; i < maxLen; i++)
			if (value[i] != oid.value[i])
				return Long.signum(value[i] - oid.value[i]);
		return Integer.signum(value.length - oid.value.length);
	}
	
	/**
	 * Get part of object ID from given position.
	 * 
	 * @param pos position
	 * @return part of object id from given position or -1 if position is invalid
	 */
	public long getIdFromPos(int pos)
	{
		return ((pos >= 0) && (pos < value.length)) ? value[pos] : -1;
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
