/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import java.io.File;
import java.net.InetAddress;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;
import org.junit.jupiter.api.Test;
import org.netxms.base.InetAddressEx;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.utilities.TestHelper;

/**
 * Download of agent's own log file.
 *
 * Preconditions: management server node has a local agent that lists the server in <code>MasterServers</code>, agent
 * <code>LogFile</code> points to a file (not <code>{syslog}</code> / <code>{systemd}</code>) in text or JSON format, the
 * log is larger than 4 KB and smaller than 64 MB, and it is not rotated while the test runs. The agent keeps writing its log
 * while the tests run, so content checks tolerate growth between and during downloads.
 */
public class AgentLogFileTest extends AbstractSessionTest
{
   private static final long INVALID_OBJECT_ID = 0x7FFFFFF0L;
   private static final long ENTIRE_NETWORK_ID = 1;
   private static final int TAIL_SIZE = 4096;
   private static final long LARGE_LIMIT = 64L * 1024 * 1024;
   private static final long FAST_RETURN_TIME = 30000;
   private static final String USER_NAME = "AgentLogFileTest-user";
   private static final String USER_PASSWORD = "AgentLogFileTest-1";
   private static final Pattern LOG_LINE = Pattern.compile("^\\d{4}\\.\\d{2}\\.\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{3} \\*[A-Z]\\* \\[.*");
   private static final String JSON_LOG_LINE_PREFIX = "{\"timestamp\":";

   /**
    * Find management server node with agent
    */
   private static Node findNode(NXCSession session) throws Exception
   {
      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node, "management server node not found");
      return node;
   }

   /**
    * Read downloaded file as ISO-8859-1 string (byte to char 1:1, so offsets match file offsets)
    */
   private static String read(File file) throws Exception
   {
      return new String(Files.readAllBytes(file.toPath()), StandardCharsets.ISO_8859_1);
   }

   /**
    * Delete downloaded file if present
    */
   private static void delete(File file)
   {
      if (file != null)
         file.delete();
   }

   /**
    * Check that at least one complete line of the content (after <code>skipFirstLine</code> partial line) has agent log
    * text or JSON format. Empty lines and continuation lines of multi-line messages are skipped.
    */
   private static void assertLogContent(String content, boolean skipFirstLine)
   {
      int start = 0;
      if (skipFirstLine)
      {
         start = content.indexOf('\n') + 1;
         assertTrue(start > 0, "no complete line in log tail");
      }
      int end;
      while((end = content.indexOf('\n', start)) >= 0)
      {
         String line = content.substring(start, end);
         if (LOG_LINE.matcher(line).matches() || line.startsWith(JSON_LOG_LINE_PREFIX))
            return;
         start = end + 1;
      }
      fail("no complete line in agent log format");
   }

   /**
    * Check limited download against full log taken before (<code>before</code>) and after (<code>after</code>) it
    */
   private static void assertTail(String before, String tail, String after)
   {
      assertTrue(tail.length() >= TAIL_SIZE, "tail shorter than limit: " + tail.length());
      // Tail starts at (size at request time - limit), size at request time is at least size of the earlier full download
      int position = after.indexOf(tail.substring(0, TAIL_SIZE), before.length() - TAIL_SIZE);
      assertTrue(position >= 0, "tail not found at expected position of the log");
      assertTrue(after.startsWith(tail, position), "tail content does not match the log");
   }

   @Test
   public void testFullDownload() throws Exception
   {
      final NXCSession session = connectAndLogin();
      File file = null;
      try
      {
         Node node = findNode(session);
         file = session.downloadAgentLogFile(node.getObjectId(), 0);
         assertNotNull(file);
         assertTrue(file.length() > 0);
         assertLogContent(read(file), false);
      }
      finally
      {
         delete(file);
         session.disconnect();
      }
   }

   @Test
   public void testSizeLimit() throws Exception
   {
      final NXCSession session = connectAndLogin();
      File before = null, tail = null, after = null;
      try
      {
         Node node = findNode(session);
         before = session.downloadAgentLogFile(node.getObjectId(), 0);
         assertTrue(before.length() > TAIL_SIZE, "agent log must be larger than " + TAIL_SIZE + " bytes");
         tail = session.downloadAgentLogFile(node.getObjectId(), TAIL_SIZE);
         after = session.downloadAgentLogFile(node.getObjectId(), 0);

         String tailContent = read(tail);
         assertTail(read(before), tailContent, read(after));
         assertLogContent(tailContent, true);
      }
      finally
      {
         delete(before);
         delete(tail);
         delete(after);
         session.disconnect();
      }
   }

   @Test
   public void testLimitLargerThanFile() throws Exception
   {
      final NXCSession session = connectAndLogin();
      File full = null, limited = null;
      try
      {
         Node node = findNode(session);
         full = session.downloadAgentLogFile(node.getObjectId(), 0);
         assertTrue(full.length() < LARGE_LIMIT, "agent log must be smaller than " + LARGE_LIMIT + " bytes");
         limited = session.downloadAgentLogFile(node.getObjectId(), LARGE_LIMIT);

         // Limit above file size means offset 0: whole earlier content is a prefix of the later download
         String fullContent = read(full);
         String limitedContent = read(limited);
         assertTrue(limitedContent.length() >= fullContent.length());
         assertTrue(limitedContent.startsWith(fullContent), "download with large limit does not start at beginning of the log");
      }
      finally
      {
         delete(full);
         delete(limited);
         session.disconnect();
      }
   }

   @Test
   public void testWithoutCompression() throws Exception
   {
      final NXCSession session = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, false);
      File before = null, tail = null, after = null;
      try
      {
         session.connect(new int[] { ProtocolVersion.INDEX_FULL });
         session.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);
         Node node = findNode(session);
         before = session.downloadAgentLogFile(node.getObjectId(), 0);
         assertTrue(before.length() > TAIL_SIZE, "agent log must be larger than " + TAIL_SIZE + " bytes");
         String beforeContent = read(before);
         assertLogContent(beforeContent, false);
         tail = session.downloadAgentLogFile(node.getObjectId(), TAIL_SIZE);
         after = session.downloadAgentLogFile(node.getObjectId(), 0);

         String tailContent = read(tail);
         assertTail(beforeContent, tailContent, read(after));
         assertLogContent(tailContent, true);
      }
      finally
      {
         delete(before);
         delete(tail);
         delete(after);
         session.disconnect();
      }
   }

   @Test
   public void testConcurrentDownloads() throws Exception
   {
      final NXCSession session = connectAndLogin();
      final List<File> files = new ArrayList<File>();
      ExecutorService executor = Executors.newFixedThreadPool(2);
      try
      {
         final long nodeId = findNode(session).getObjectId();
         List<Future<File>> results = new ArrayList<Future<File>>();
         for(int i = 0; i < 2; i++)
            results.add(executor.submit(() -> session.downloadAgentLogFile(nodeId, TAIL_SIZE)));
         for(Future<File> f : results)
            files.add(f.get(150, TimeUnit.SECONDS));
         for(File file : files)
         {
            assertTrue(file.length() >= TAIL_SIZE);
            assertLogContent(read(file), true);
         }
      }
      finally
      {
         executor.shutdownNow();
         for(File file : files)
            delete(file);
         session.disconnect();
      }
   }

   @Test
   public void testInvalidObject() throws Exception
   {
      final NXCSession session = connectAndLogin();
      try
      {
         long startTime = System.currentTimeMillis();
         NXCException e = assertThrows(NXCException.class, () -> session.downloadAgentLogFile(INVALID_OBJECT_ID, 0));
         assertEquals(RCC.INVALID_OBJECT_ID, e.getErrorCode());
         assertTrue(System.currentTimeMillis() - startTime < FAST_RETURN_TIME);
      }
      finally
      {
         session.disconnect();
      }
   }

   @Test
   public void testNotANode() throws Exception
   {
      final NXCSession session = connectAndLogin();
      try
      {
         long startTime = System.currentTimeMillis();
         NXCException e = assertThrows(NXCException.class, () -> session.downloadAgentLogFile(ENTIRE_NETWORK_ID, 0));
         assertEquals(RCC.INCOMPATIBLE_OPERATION, e.getErrorCode());
         assertTrue(System.currentTimeMillis() - startTime < FAST_RETURN_TIME);
      }
      finally
      {
         session.disconnect();
      }
   }

   @Test
   public void testNodeWithoutAgent() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject staleNode = session.findObjectByName("AgentLogFileTest-NoAgent");  // left by interrupted run
      if (staleNode != null)
         session.deleteObject(staleNode.getObjectId());
      long nodeId = 0;
      try
      {
         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, "AgentLogFileTest-NoAgent", GenericObject.SERVICEROOT);
         cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
         cd.setIpAddress(new InetAddressEx(InetAddress.getByName("192.0.2.1"), 0));
         nodeId = session.createObjectSync(cd).getObjectId();

         final long id = nodeId;
         long startTime = System.currentTimeMillis();
         NXCException e = assertThrows(NXCException.class, () -> session.downloadAgentLogFile(id, 0));
         assertEquals(RCC.NO_CONNECTION_TO_AGENT, e.getErrorCode());
         assertTrue(System.currentTimeMillis() - startTime < FAST_RETURN_TIME);
      }
      finally
      {
         if (nodeId != 0)
            session.deleteObject(nodeId);
         session.disconnect();
      }
   }

   @Test
   public void testAccessDenied() throws Exception
   {
      final NXCSession admin = connectAndLogin();
      admin.syncObjects();
      AbstractObject staleNode = admin.findObjectByName("AgentLogFileTest-AccessDenied");  // left by interrupted run
      if (staleNode != null)
         admin.deleteObject(staleNode.getObjectId());
      NXCSession user = null;
      long nodeId = 0;
      try
      {
         // User is reused between runs: a deleted user keeps its name until the server purges it
         User u = TestHelper.findOrCreateUser(admin, USER_NAME, USER_PASSWORD);
         admin.setUserPassword(u.getId(), USER_PASSWORD, null);
         u.setSystemRights(0);
         u.setFlags(u.getFlags() & ~AbstractUserObject.CHANGE_PASSWORD);
         admin.modifyUserDBObject(u, AbstractUserObject.MODIFY_ACCESS_RIGHTS | AbstractUserObject.MODIFY_FLAGS);

         // Direct ACL entry with read access only overrides inherited rights for the test user, while admin keeps
         // inherited rights (needed to delete the node). Access check runs before agent connection, so node without agent is sufficient.
         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, "AgentLogFileTest-AccessDenied", GenericObject.SERVICEROOT);
         cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
         cd.setIpAddress(new InetAddressEx(InetAddress.getByName("192.0.2.2"), 0));
         nodeId = admin.createObjectSync(cd).getObjectId();

         NXCObjectModificationData md = new NXCObjectModificationData(nodeId);
         md.setInheritAccessRights(true);
         md.setACL(Collections.singletonList(new AccessListElement(u.getId(), UserAccessRights.OBJECT_ACCESS_READ)));
         admin.modifyObject(md);

         user = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
         user.connect(new int[] { ProtocolVersion.INDEX_FULL });
         user.login(USER_NAME, USER_PASSWORD);

         final NXCSession userSession = user;
         final long id = nodeId;
         long startTime = System.currentTimeMillis();
         NXCException e = assertThrows(NXCException.class, () -> userSession.downloadAgentLogFile(id, 0));
         assertEquals(RCC.ACCESS_DENIED, e.getErrorCode());
         assertTrue(System.currentTimeMillis() - startTime < FAST_RETURN_TIME);
      }
      finally
      {
         if (user != null)
            user.disconnect();
         if (nodeId != 0)
            admin.deleteObject(nodeId);
         admin.disconnect();
      }
   }
}
