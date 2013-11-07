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
package org.netxms.base;

/**
 * Compatibility tools, mostly for Android platform
 *
 */
public class CompatTools
{
	/**
	 * Create copy of byte array
	 * 
	 * @param source
	 * @param newLength
	 * @return
	 */
	public static byte[] arrayCopy(byte[] source, int newLength)
	{
      byte[] dst = new byte[newLength];
      System.arraycopy(source, 0, dst, 0, Math.min(source.length, newLength));
      return dst;
	}

	/**
	 * Create copy of long array
	 * 
	 * @param source
	 * @param newLength
	 * @return
	 */
	public static long[] arrayCopy(long[] source, int newLength)
	{
      long[] dst = new long[newLength];
      System.arraycopy(source, 0, dst, 0, Math.min(source.length, newLength));
      return dst;
	}
}
