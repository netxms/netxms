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
package org.netxms.client;

import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.netxms.base.CompatTools;

/**
 * MAC address representation
 *
 */
public class MacAddress
{
	private byte[] value;
	
	/**
	 * Create MAC address with value of 00:00:00:00:00:00
	 */
	public MacAddress()
	{
		value = new byte[6];
		Arrays.fill(value, (byte)0);
	}
	
	/**
	 * Create MAC address object from byte array
	 * 
	 * @param src byte array containing MAC address value
	 */
	public MacAddress(byte[] src)
	{
		if (src != null)
		{
			value = CompatTools.arrayCopy(src, 6);
		}
		else
		{
			value = new byte[6];
			Arrays.fill(value, (byte)0);
		}
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		StringBuilder sb = new StringBuilder();
		int ub = (int)value[0] & 0xFF;
		if (ub < 0x10)
			sb.append('0');
		sb.append(Integer.toHexString(ub));
		for(int i = 1; i < value.length; i++)
		{
			sb.append(':');
			ub = (int)value[i] & 0xFF;
			if (ub < 0x10)
				sb.append('0');
			sb.append(Integer.toHexString(ub));
		}
		return sb.toString().toUpperCase();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object arg0)
	{
		if (arg0 instanceof MacAddress)
			return Arrays.equals(value, ((MacAddress)arg0).value);
		return super.equals(arg0);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		return Arrays.hashCode(value);
	}

	/**
	 * Parse MAC address string representation. Supported representations are 6 groups of
	 * two hex digits, separated by spaces, minuses, or colons, or unseparated. Examples of valid 
	 * MAC address strings:
	 * 00:10:FA:23:11:7A
	 * 01 02 fa c4 10 dc
	 * 00-90-0b-11-01-29
	 * 0203fcd456c1
	 * 
	 * @param str MAC address string
	 * @return MAC address object
	 * @throws MacAddressFormatException if MAC address sting is invalid
	 */
	public static MacAddress parseMacAddress(String str) throws MacAddressFormatException
	{
		Pattern pattern = Pattern.compile("^([0-9a-fA-F]{2})[ :\\-]?([0-9a-fA-F]{2})[ :\\-]?([0-9a-fA-F]{2})[ :\\-]?([0-9a-fA-F]{2})[ :\\-]?([0-9a-fA-F]{2})[ :\\-]?([0-9a-fA-F]{2})$");
		Matcher matcher = pattern.matcher(str.trim());
		if (matcher.matches())
		{
			byte[] bytes = new byte[6];
			try
			{
				for(int i = 0; i < 6; i++)
					bytes[i] = (byte)Integer.parseInt(matcher.group(i + 1), 16);
			}
			catch(NumberFormatException e)
			{
				throw new MacAddressFormatException();
			}
			return new MacAddress(bytes);
		}
		else
		{
			throw new MacAddressFormatException();
		}
	}

	/**
	 * @return the value
	 */
	public byte[] getValue()
	{
		return CompatTools.arrayCopy(value, 6);
	}
	
	/**
	 * Compare this MAC address to another MAC address.
	 * 
	 * @param dst
	 * @return 0 if two MAC addresses are equal, negative if this MAC address is "lower", and positive if "higher"
	 */
	public int compareTo(MacAddress dst)
	{
		if (dst == null)
			return 1;
		int len = Math.min(value.length, dst.value.length);
		for(int i = 0; i < len; i++)
		{
			int v1 = (int)value[i] & 0xFF;
			int v2 = (int)dst.value[i] & 0xFF;
			if (v1 < v2)
				return -1;
			if (v1 > v2)
				return 1;
		}
		return value.length - dst.value.length;
	}
}
