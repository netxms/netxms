/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.net.InetSocketAddress;
import org.junit.jupiter.api.Test;

/**
 * Tests for NXCSession.parseConnectionAddress()
 */
public class ConnectionAddressTest
{
   private static void assertAddress(String expectedHost, int expectedPort, String address)
   {
      InetSocketAddress a = NXCSession.parseConnectionAddress(address);
      assertTrue(a.isUnresolved(), "address should not be resolved by parser");
      assertEquals(expectedHost, a.getHostString());
      assertEquals(expectedPort, a.getPort());
   }

   @Test
   public void testHostWithoutPort()
   {
      assertAddress("server.example.com", NXCSession.DEFAULT_CONN_PORT, "server.example.com");
      assertAddress("10.0.0.1", NXCSession.DEFAULT_CONN_PORT, "10.0.0.1");
   }

   @Test
   public void testHostWithPort()
   {
      assertAddress("server.example.com", 47010, "server.example.com:47010");
      assertAddress("10.0.0.1", 1, "10.0.0.1:1");
      assertAddress("10.0.0.1", 65535, "10.0.0.1:65535");
   }

   @Test
   public void testIPv6Literal()
   {
      assertAddress("::1", NXCSession.DEFAULT_CONN_PORT, "::1");
      assertAddress("fe80::1:2:3", NXCSession.DEFAULT_CONN_PORT, "fe80::1:2:3");
      assertAddress("::1", NXCSession.DEFAULT_CONN_PORT, "[::1]");
      assertAddress("fe80::1:2:3", 4702, "[fe80::1:2:3]:4702");
   }

   @Test
   public void testInvalidPort()
   {
      assertThrows(IllegalArgumentException.class, () -> NXCSession.parseConnectionAddress("server:abc"));
      assertThrows(IllegalArgumentException.class, () -> NXCSession.parseConnectionAddress("server:"));
      assertThrows(IllegalArgumentException.class, () -> NXCSession.parseConnectionAddress("server:0"));
      assertThrows(IllegalArgumentException.class, () -> NXCSession.parseConnectionAddress("server:-1"));
      assertThrows(IllegalArgumentException.class, () -> NXCSession.parseConnectionAddress("server:65536"));
      assertThrows(IllegalArgumentException.class, () -> NXCSession.parseConnectionAddress("[::1]:abc"));
      assertThrows(IllegalArgumentException.class, () -> NXCSession.parseConnectionAddress("[::1]:"));
   }
}
