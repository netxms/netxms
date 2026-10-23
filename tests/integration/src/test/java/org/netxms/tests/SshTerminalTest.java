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
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.TcpProxy;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;

/**
 * Tests for SSH terminal sessions (CMD_SETUP_SSH_TERMINAL / CMD_RESIZE_SSH_CHANNEL). Argument validation and failure paths are
 * always tested. Data flow test requires SSH server on management server node reachable from its SSH proxy and valid credentials
 * in test properties (ssh.login / ssh.password); it is skipped when ssh.login is not set.
 */
public class SshTerminalTest extends AbstractSessionTest
{
   private static final int READ_TIMEOUT = 30000;

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

   /**
    * Authentication failure happens when SSH proxy agent establishes SSH session, so it is reported as REMOTE_CONNECT_FAILED. When
    * node has no usable SSH proxy, setup fails earlier with RESOURCE_NOT_AVAILABLE (proxy node not found) or COMM_FAILURE (no
    * connection to proxy agent); in that case data flow cannot be tested either, so the test fails to make the misconfiguration
    * visible unless SSH credentials are not configured (data flow test skipped anyway).
    */
   @Test
   public void testSetupWithInvalidCredentials() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      try
      {
         TcpProxy proxy = session.setupSshTerminal(node.getObjectId(), "netxms-test-no-such-user", "invalid", 0, "xterm", 80, 24);
         proxy.close();
         fail("SSH session established with invalid credentials");
      }
      catch(NXCException e)
      {
         int rcc = e.getErrorCode();
         if ((rcc == RCC.RESOURCE_NOT_AVAILABLE) || (rcc == RCC.COMM_FAILURE))
         {
            if (TestConstants.SSH_LOGIN.isEmpty())
               Assumptions.abort("SSH proxy for management server node is not available (RCC " + rcc + ")");
            else
               fail("SSH proxy for management server node is not available (RCC " + rcc + ")");
         }
         else
         {
            assertEquals(RCC.REMOTE_CONNECT_FAILED, rcc, "Unexpected RCC " + rcc);
         }
      }
      session.disconnect();
   }

   /**
    * Full data flow: establish session, write command, read its output back through proxy channel, resize, close.
    */
   @Test
   public void testDataFlow() throws Exception
   {
      Assumptions.assumeTrue(!TestConstants.SSH_LOGIN.isEmpty(), "ssh.login is not set in test properties");

      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);

      TcpProxy proxy = session.setupSshTerminal(node.getObjectId(), TestConstants.SSH_LOGIN, TestConstants.SSH_PASSWORD, 0, "xterm", 80, 24);
      try
      {
         // Marker is split in command text so that command echo does not match it; only command output does
         String marker = "NXTEST-" + System.currentTimeMillis();
         String command = "echo " + marker.substring(0, 7) + "''" + marker.substring(7) + "\n";
         proxy.getOutputStream().write(command.getBytes(StandardCharsets.UTF_8));
         proxy.getOutputStream().flush();

         String output = readUntil(proxy.getInputStream(), marker);
         assertTrue(output.contains(marker), "Command output not received through SSH terminal channel; received: " + output);

         session.resizeSshTerminal(proxy, 100, 30);

         proxy.getOutputStream().write("exit\n".getBytes(StandardCharsets.UTF_8));
         proxy.getOutputStream().flush();
         // Consume remaining output until remote side closes the channel
         long deadline = System.currentTimeMillis() + READ_TIMEOUT;
         while(!proxy.isClosed() && (System.currentTimeMillis() < deadline))
         {
            if (proxy.getInputStream().available() > 0)
               proxy.getInputStream().skip(proxy.getInputStream().available());
            else
               Thread.sleep(100);
         }
      }
      finally
      {
         proxy.close();
      }
      assertTrue(proxy.isClosed());
      session.disconnect();
   }

   /**
    * Read from stream until given marker appears in accumulated output or timeout expires. Stream read is blocking without
    * timeout, so data availability is polled instead.
    */
   private static String readUntil(InputStream in, String marker) throws Exception
   {
      StringBuilder sb = new StringBuilder();
      byte[] buffer = new byte[4096];
      long deadline = System.currentTimeMillis() + READ_TIMEOUT;
      while(!sb.toString().contains(marker) && (System.currentTimeMillis() < deadline))
      {
         if (in.available() > 0)
         {
            int bytes = in.read(buffer, 0, Math.min(buffer.length, in.available()));
            if (bytes < 0)
               break;
            sb.append(new String(buffer, 0, bytes, StandardCharsets.UTF_8));
         }
         else
         {
            Thread.sleep(100);
         }
      }
      return sb.toString();
   }
}
