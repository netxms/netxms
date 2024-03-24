/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import static org.junit.jupiter.api.Assertions.assertEquals;
import java.net.InetAddress;
import org.junit.jupiter.api.Test;

/**
 * Tests for InetAddressEx class
 */
public class InetAddressExTest
{
   @Test
   public void testInetAddressEx() throws Exception
   {
      InetAddressEx a = new InetAddressEx(InetAddress.getByName("192.168.10.15"), InetAddress.getByName("255.255.255.0"));
      assertEquals(24, a.getMask());
      assertEquals(8, a.getHostBits());
      assertEquals("255.255.255.0", a.maskFromBits().getHostAddress());
      assertEquals("192.168.10.15", a.getHostAddress());
      assertEquals("192.168.10.15/24", a.toString());

      a.setMask(26);
      assertEquals(26, a.getMask());
      assertEquals(6, a.getHostBits());
      assertEquals("255.255.255.192", a.maskFromBits().getHostAddress());
      assertEquals("192.168.10.15/26", a.toString());

      a.setMask(19);
      assertEquals(19, a.getMask());
      assertEquals(13, a.getHostBits());
      assertEquals("255.255.224.0", a.maskFromBits().getHostAddress());
      assertEquals("192.168.10.15/19", a.toString());

      a = new InetAddressEx();
      assertEquals("UNSPEC", a.toString());

      a = new InetAddressEx(InetAddress.getByName("10.233.1.16"), 15);
      assertEquals(15, a.getMask());
      assertEquals(17, a.getHostBits());
      assertEquals("255.254.0.0", a.maskFromBits().getHostAddress());
      assertEquals("10.233.1.16/15", a.toString());

      a = new InetAddressEx(InetAddress.getByName("fe80::10b4:7301:cf5d:9a87"), 64);
      assertEquals(64, a.getMask());
      assertEquals(64, a.getHostBits());
      assertEquals("fe80::10b4:7301:cf5d:9a87/64", a.toString());
   }
}
