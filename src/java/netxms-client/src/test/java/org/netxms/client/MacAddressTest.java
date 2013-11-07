/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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

import junit.framework.TestCase;

/**
 * Tests for MacAddress class
 *
 */
public class MacAddressTest extends TestCase
{
	public void testMacAddress() throws Exception
	{
		MacAddress a1 = new MacAddress();
		assertEquals("00:00:00:00:00:00", a1.toString());

		MacAddress a2 = new MacAddress(new byte[] { 0x02, 0x0C, (byte)0xD4, 0x55, 0x22, 0x1A });
		assertEquals("02:0C:D4:55:22:1A", a2.toString());
		
		MacAddress a3 = MacAddress.parseMacAddress("02:0C:D4:55:22:1A");
		assertEquals(a2, a3);
		
		try
		{
			MacAddress.parseMacAddress("bad mac string");
			assertTrue(false);
		}
		catch(MacAddressFormatException e)
		{
			System.out.println("Exception caught: " + e.getMessage());
		}
	}
}
