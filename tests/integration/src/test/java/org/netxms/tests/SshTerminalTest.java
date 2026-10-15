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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.TcpProxy;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;

/**
 * Tests for SSH terminal session setup (CMD_SETUP_SSH_TERMINAL / CMD_RESIZE_SSH_CHANNEL). Successful session requires
 * SSH server reachable from management server's SSH proxy, so only argument validation and failure paths are tested
 * here; a successful session (if local SSH server accepts provided credentials) is verified for data flow.
 */
public class SshTerminalTest extends AbstractSessionTest
{
   @Test
   public void testInvalidTerminalSize() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      try
      {
         session.setupSshTerminal(node.getObjectId(), "nobody", "", 0, "xterm", 0, 24);
         fail("Expected NXCException");
      }
      catch(NXCException e)
      {
         assertEquals(RCC.INVALID_ARGUMENT, e.getErrorCode());
      }
      session.disconnect();
   }

   @Test
   public void testInvalidNode() throws Exception
   {
      final NXCSession session = connectAndLogin();
      try
      {
         session.setupSshTerminal(0x7FFFFFFF, "nobody", "", 0, "xterm", 80, 24);
         fail("Expected NXCException");
      }
      catch(NXCException e)
      {
         assertEquals(RCC.INVALID_OBJECT_ID, e.getErrorCode());
      }
      session.disconnect();
   }

   @Test
   public void testSetupWithInvalidCredentials() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      try
      {
         TcpProxy proxy = session.setupSshTerminal(node.getObjectId(), "netxms-test-no-such-user", "invalid", 0, "xterm", 80, 24);
         // Server accepted connection (SSH server may allow it in exotic setups): verify that channel works and close it
         session.resizeSshTerminal(proxy, 100, 30);
         proxy.close();
         assertTrue(proxy.isClosed());
      }
      catch(NXCException e)
      {
         // Expected: no SSH proxy, no SSH server, or authentication failure
         int rcc = e.getErrorCode();
         assertTrue((rcc == RCC.REMOTE_CONNECT_FAILED) || (rcc == RCC.RESOURCE_NOT_AVAILABLE) || (rcc == RCC.COMM_FAILURE) || (rcc == RCC.AGENT_ERROR),
               "Unexpected RCC " + rcc);
      }
      session.disconnect();
   }
}
