/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Locale;
import org.junit.jupiter.api.Test;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.RCC;

/**
 * Tests for ProtocolVersionException
 */
public class ProtocolVersionExceptionTest
{
   private static ProtocolVersion createProtocolVersion()
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED);
      msg.setField(NXCPCodes.VID_PROTOCOL_VERSION_EX, new long[] { 62, 4, 1, 1, 1, 56, 1, 2 });
      return new ProtocolVersion(msg);
   }

   @Test
   public void testAccessors()
   {
      ProtocolVersion protocolVersion = createProtocolVersion();
      ProtocolVersionException e = new ProtocolVersionException(protocolVersion, "5.3.1", "5.3.1-2043");

      assertEquals(RCC.BAD_PROTOCOL, e.getErrorCode());
      assertSame(protocolVersion, e.getProtocolVersion());
      assertEquals("5.3.1", e.getServerVersion());
      assertEquals("5.3.1-2043", e.getServerBuild());
      assertEquals(protocolVersion.toString(), e.getAdditionalInfo());
   }

   @Test
   public void testNullServerBuild()
   {
      ProtocolVersionException e = new ProtocolVersionException(createProtocolVersion(), "5.3.1", null);
      assertNull(e.getServerBuild());
      assertEquals(RCC.BAD_PROTOCOL, e.getErrorCode());
   }

   @Test
   public void testLocalizedMessage()
   {
      Locale savedLocale = Locale.getDefault();
      try
      {
         Locale.setDefault(new Locale("en"));
         ProtocolVersion protocolVersion = createProtocolVersion();
         ProtocolVersionException e = new ProtocolVersionException(protocolVersion, "5.3.1", "5.3.1-2043");

         String expected = "Server uses incompatible version of communication protocol (" + protocolVersion.toString() + ")";
         assertEquals(expected, e.getLocalizedMessage());
         assertEquals(expected, e.getMessage());
         assertTrue(e.getLocalizedMessage().contains("server=[62, 4, 1, 1, 1, 56, 1, 2]"));
      }
      finally
      {
         Locale.setDefault(savedLocale);
      }
   }
}
