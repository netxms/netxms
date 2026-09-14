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

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.Test;
import org.netxms.base.InetAddressEx;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.interfaces.AutoBindObject;

/**
 * Tests for object deletion racing with object linking. Historically an autobind poll (or any other bind path) that
 * reached an object after its deletion had already cleared the relation lists re-added the deleted object to the
 * parent's child list, leaving a zombie that consoles counted but could not display.
 */
public class ObjectDeletionRaceTest extends AbstractSessionTest
{
   private static final long WAIT_TIMEOUT = 120000;
   private static final int SCRIPT_DELAY = 8000;

   /**
    * Output collector for poller / script output
    */
   private static class OutputCollector implements TextOutputListener
   {
      final StringBuilder output = new StringBuilder();
      Exception failure = null;

      @Override
      public synchronized void messageReceived(String text)
      {
         output.append(text);
      }

      @Override
      public void setStreamId(long streamId)
      {
      }

      @Override
      public void onSuccess()
      {
      }

      @Override
      public void onFailure(Exception exception)
      {
         failure = exception;
      }

      synchronized String getOutput()
      {
         return output.toString();
      }
   }

   /**
    * Open second session for long-running server side operation, so that the main session stays free for deletion
    * and object synchronization.
    */
   private static NXCSession openSecondSession() throws Exception
   {
      NXCSession session = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
      session.connect(new int[] { ProtocolVersion.INDEX_FULL });
      session.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);
      return session;
   }

   /**
    * Create unmanaged test node under given parent
    */
   private static long createTestNode(NXCSession session, String name, String address) throws Exception
   {
      NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, name, GenericObject.SERVICEROOT);
      cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
      cd.setIpAddress(new InetAddressEx(InetAddress.getByName(address), 0));
      return session.createObjectSync(cd).getObjectId();
   }

   /**
    * Wait until object disappears from client cache (deletion is asynchronous on server side)
    */
   private static void waitForObjectDeletion(NXCSession session, long objectId) throws Exception
   {
      long deadline = System.currentTimeMillis() + WAIT_TIMEOUT;
      while(session.findObjectById(objectId) != null)
      {
         assertTrue(System.currentTimeMillis() < deadline, "Object " + objectId + " was not deleted within timeout");
         Thread.sleep(200);
      }
   }

   /**
    * Delete object if it still exists (cleanup helper)
    */
   /**
    * Wait until node gets an interface object (created by configuration poll)
    */
   private static long waitForInterface(NXCSession session, long nodeId) throws Exception
   {
      long deadline = System.currentTimeMillis() + WAIT_TIMEOUT;
      while(true)
      {
         session.syncObjects();
         AbstractObject node = session.findObjectById(nodeId);
         if (node != null)
         {
            for(AbstractObject o : node.getChildrenAsArray())
               if (o instanceof Interface)
                  return o.getObjectId();
         }
         assertTrue(System.currentTimeMillis() < deadline, "Interface was not created for node " + nodeId + " within timeout");
         Thread.sleep(1000);
      }
   }

   private static void deleteIfExists(NXCSession session, long objectId)
   {
      try
      {
         if ((objectId != 0) && (session.findObjectById(objectId) != null))
            session.deleteObject(objectId);
      }
      catch(Exception e)
      {
         System.out.println("Cleanup: cannot delete object " + objectId + ": " + e.getMessage());
      }
   }

   private static boolean contains(long[] list, long id)
   {
      for(long v : list)
         if (v == id)
            return true;
      return false;
   }

   /**
    * Node is deleted while container autobind poll is about to bind it. The filter script sleeps for the test node only,
    * so the poll reaches the bind step after the deletion has already unlinked the node. The container must not end up
    * with a reference to the deleted node.
    */
   @Test
   public void testNodeDeletedDuringAutobind() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      String suffix = Long.toString(System.currentTimeMillis());
      String nodeName = "IT-DeleteRace-Node-" + suffix;
      long containerId = 0;
      long nodeId = 0;
      NXCSession pollSession = null;
      try
      {
         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_CONTAINER, "IT-DeleteRace-Container-" + suffix, GenericObject.SERVICEROOT);
         containerId = session.createObjectSync(cd).getObjectId();
         nodeId = createTestNode(session, nodeName, "192.168.254.201");

         // Bind filter: delay only for the test node, so that deletion can run while the poll is "evaluating" it
         NXCObjectModificationData md = new NXCObjectModificationData(containerId);
         md.setAutoBindFilter("if ($object.id == " + nodeId + ") { sleep(" + SCRIPT_DELAY + "); return true; }\nreturn false;");
         md.setAutoBindFlags(AutoBindObject.OBJECT_BIND_FLAG);
         session.modifyObject(md);

         final long pollTarget = containerId;
         final OutputCollector pollOutput = new OutputCollector();
         final AtomicReference<Exception> pollError = new AtomicReference<Exception>();
         pollSession = openSecondSession();
         final NXCSession ps = pollSession;
         Thread pollThread = new Thread(new Runnable() {
            @Override
            public void run()
            {
               try
               {
                  ps.pollObject(pollTarget, ObjectPollType.AUTOBIND, pollOutput);
               }
               catch(Exception e)
               {
                  pollError.set(e);
               }
            }
         }, "autobind-poll");
         pollThread.start();

         // Let the poll reach the test node and enter the script delay, then delete the node
         Thread.sleep(SCRIPT_DELAY / 4);
         session.deleteObject(nodeId);
         waitForObjectDeletion(session, nodeId);

         pollThread.join(WAIT_TIMEOUT);
         assertFalse(pollThread.isAlive(), "Autobind poll did not complete within timeout");
         assertNull(pollError.get(), "Autobind poll failed: " + pollError.get());

         // Poll must have reached the bind step for the test node, otherwise the race was not exercised
         String output = pollOutput.getOutput();
         assertTrue(output.contains("Binding object " + nodeName), "Autobind poll did not attempt to bind test node; poller output:\n" + output);

         Thread.sleep(1000); // Object updates should be received from server
         session.syncObjects();

         assertNull(session.findObjectById(nodeId), "Deleted node is still visible");
         AbstractObject container = session.findObjectById(containerId);
         assertNotNull(container);
         assertFalse(contains(container.getChildIdList(), nodeId), "Container still references deleted node " + nodeId);
      }
      finally
      {
         if (pollSession != null)
            pollSession.disconnect();
         deleteIfExists(session, nodeId);
         deleteIfExists(session, containerId);
      }
   }

   /**
    * Mirror case: container is deleted while a script is about to bind a live node to it. The node must not end up with
    * a reference to the deleted container in its parent list.
    */
   @Test
   public void testContainerDeletedDuringBind() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      String suffix = Long.toString(System.currentTimeMillis());
      long containerId = 0;
      long nodeId = 0;
      NXCSession scriptSession = null;
      try
      {
         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_CONTAINER, "IT-DeleteRace-Container-" + suffix, GenericObject.SERVICEROOT);
         containerId = session.createObjectSync(cd).getObjectId();
         nodeId = createTestNode(session, "IT-DeleteRace-Node-" + suffix, "192.168.254.202");

         // Script runs with the container as $object and holds a reference to it while it sleeps
         final String script = "sleep(" + SCRIPT_DELAY + ");\nprintln(\"BIND=\" .. BindObject($object, FindObject(" + nodeId + ")));";
         final long scriptTarget = containerId;
         final OutputCollector scriptOutput = new OutputCollector();
         final AtomicReference<Exception> scriptError = new AtomicReference<Exception>();
         scriptSession = openSecondSession();
         final NXCSession ss = scriptSession;
         Thread scriptThread = new Thread(new Runnable() {
            @Override
            public void run()
            {
               try
               {
                  ss.executeScript(scriptTarget, script, new ArrayList<String>(), scriptOutput);
               }
               catch(Exception e)
               {
                  scriptError.set(e);
               }
            }
         }, "bind-script");
         scriptThread.start();

         // Let the script start and enter its delay, then delete the container
         Thread.sleep(SCRIPT_DELAY / 4);
         session.deleteObject(containerId);
         waitForObjectDeletion(session, containerId);

         scriptThread.join(WAIT_TIMEOUT);
         assertFalse(scriptThread.isAlive(), "Script execution did not complete within timeout");
         assertNull(scriptError.get(), "Script execution failed: " + scriptError.get());

         String output = scriptOutput.getOutput();
         assertTrue(output.contains("BIND="), "Script did not reach BindObject; script output:\n" + output);
         System.out.println("testContainerDeletedDuringBind: script output: " + output.trim());

         Thread.sleep(1000); // Object updates should be received from server
         session.syncObjects();

         assertNull(session.findObjectById(containerId), "Deleted container is still visible");
         AbstractObject node = session.findObjectById(nodeId);
         assertNotNull(node);
         assertFalse(contains(node.getParentIdList(), containerId), "Node still references deleted container " + containerId);
      }
      finally
      {
         if (scriptSession != null)
            scriptSession.disconnect();
         deleteIfExists(session, nodeId);
         deleteIfExists(session, containerId);
      }
   }

   /**
    * Node deleted while its configuration poll is running. Deletion waits for the poll to finish, and the poll creates a
    * pseudo-interface for the primary IP after the deletion flag is already set. The interface must not survive as an
    * object without parent.
    */
   @Test
   public void testInterfaceCreatedDuringNodeDeletion() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      String suffix = Long.toString(System.currentTimeMillis());
      final String nodeAddress = "192.0.2.1"; // TEST-NET-1, never answers, so agent and SNMP probes run into their timeouts
      long nodeId = 0;
      NXCSession pollSession = null;
      try
      {
         // All probes disabled: creation poll is fast and creates only the pseudo-interface for primary IP
         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, "IT-DeleteRace-Node-" + suffix, GenericObject.SERVICEROOT);
         cd.setCreationFlags(NXCObjectCreationData.CF_DISABLE_NXCP | NXCObjectCreationData.CF_DISABLE_SNMP | NXCObjectCreationData.CF_DISABLE_ETHERNET_IP | NXCObjectCreationData.CF_DISABLE_SSH);
         cd.setIpAddress(new InetAddressEx(InetAddress.getByName(nodeAddress), 0));
         nodeId = session.createObjectSync(cd).getObjectId();

         // Remove pseudo-interface so that next configuration poll has to create it again
         long ifaceId = waitForInterface(session, nodeId);
         session.deleteObject(ifaceId);
         waitForObjectDeletion(session, ifaceId);

         // Enable agent and SNMP: next poll spends time on their timeouts before it gets to interface configuration
         NXCObjectModificationData md = new NXCObjectModificationData(nodeId);
         md.setObjectFlags(0, AbstractNode.NF_DISABLE_NXCP | AbstractNode.NF_DISABLE_SNMP);
         session.modifyObject(md);

         final long pollTarget = nodeId;
         final OutputCollector pollOutput = new OutputCollector();
         final AtomicReference<Exception> pollError = new AtomicReference<Exception>();
         final AtomicReference<Long> pollFinished = new AtomicReference<Long>();
         pollSession = openSecondSession();
         final NXCSession ps = pollSession;
         Thread pollThread = new Thread(new Runnable() {
            @Override
            public void run()
            {
               try
               {
                  ps.pollObject(pollTarget, ObjectPollType.CONFIGURATION, pollOutput);
               }
               catch(Exception e)
               {
                  pollError.set(e);
               }
               pollFinished.set(System.currentTimeMillis());
            }
         }, "configuration-poll");
         pollThread.start();

         // Let the poll start its probes, then delete the node
         Thread.sleep(1000);
         long deleteRequested = System.currentTimeMillis();
         session.deleteObject(nodeId);
         waitForObjectDeletion(session, nodeId);
         pollThread.join(WAIT_TIMEOUT);
         assertFalse(pollThread.isAlive(), "Configuration poll did not complete within timeout");
         assertNull(pollError.get(), "Configuration poll failed: " + pollError.get());
         assertTrue(pollFinished.get() > deleteRequested, "Configuration poll finished before deletion was requested, race was not exercised");
         String output = pollOutput.getOutput();
         assertTrue(output.contains("Checking interface configuration"), "Configuration poll did not reach interface configuration; poller output:\n" + output);

         Thread.sleep(1000); // Object updates should be received from server
         session.syncObjects();
         assertNull(session.findObjectById(nodeId), "Deleted node is still visible");
         for(AbstractObject o : session.getAllObjects())
         {
            if (!(o instanceof Interface))
               continue;
            for(InetAddressEx a : ((Interface)o).getIpAddressList())
               if (a.getAddress().getHostAddress().equals(nodeAddress))
                  fail("Interface " + o.getObjectName() + " [" + o.getObjectId() + "] of deleted node " + nodeId + " still exists (parents: " + o.getParentIdList().length + ")");
         }
      }
      finally
      {
         if (pollSession != null)
            pollSession.disconnect();
         deleteIfExists(session, nodeId);
      }
   }
}
