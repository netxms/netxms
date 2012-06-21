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
package org.netxms.ui.eclipse.tools;

import java.net.InetAddress;

/**
 * Helper class for comparators
 */
public class ComparatorHelper
{
	public static int compareInetAddresses(InetAddress a1, InetAddress a2)
	{
		byte[] b1 = a1.getAddress(); 
		byte[] b2 = a2.getAddress();
		int length = Math.min(b1.length, b2.length);
		for(int i = 0; i < length; i++)
		{
			int n1 = (int)b1[i] & 0xFF; 
			int n2 = (int)b2[i] & 0xFF; 
			if (n1 < n2)
				return -1;
			if (n1 > n2)
				return 1;
		}
		return b1.length - b2.length;
	}
}
