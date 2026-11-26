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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.io.IOException;
import java.net.InetAddress;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.Deque;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;
import org.netxms.base.InetAddressEx;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Subnet;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.utilities.TestHelper;

/**
 * @author Victor
 *
 */
public class ObjectTest extends AbstractSessionTest
{
   private static final String PASSWORD = "Rights-Test-1";

   @Test
   public void testEffectiveRightsSentWithObjects() throws Exception
   {
      NXCSession admin = connectAndLogin();
      String suffix = UUID.randomUUID().toString();
      Deque<Integer> principals = new ArrayDeque<>();
      long parentId = 0;
      try
      {
         int directUserId = admin.createUser("direct-" + suffix);
         principals.push(directUserId);
         int groupUserId = admin.createUser("member-" + suffix);
         principals.push(groupUserId);
         int groupId = admin.createUserGroup("group-" + suffix);
         principals.push(groupId);
         enableLogin(admin, directUserId);
         enableLogin(admin, groupUserId);
         UserGroup group = (UserGroup)admin.findUserDBObjectById(groupId, null);
         group.setMembers(new int[] { groupUserId });
         admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);

         parentId = admin.createObject(new NXCObjectCreationData(AbstractObject.OBJECT_CONTAINER, "rights-" + suffix, AbstractObject.SERVICEROOT));
         NXCObjectModificationData acl = new NXCObjectModificationData(parentId);
         acl.setInheritAccessRights(false);
         acl.setACL(Arrays.asList(new AccessListElement(admin.getUserId(), admin.getEffectiveRights(parentId)),
               new AccessListElement(directUserId, UserAccessRights.OBJECT_ACCESS_READ | UserAccessRights.OBJECT_ACCESS_MODIFY),
               new AccessListElement(groupId, UserAccessRights.OBJECT_ACCESS_READ | UserAccessRights.OBJECT_ACCESS_EXECUTE_SCRIPT)));
         admin.modifyObject(acl);
         long childId = admin.createObject(new NXCObjectCreationData(AbstractObject.OBJECT_CONTAINER, "child", parentId));

         for(int userId : new int[] { directUserId, groupUserId })
         {
            NXCSession user = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
            user.connect(new int[] { ProtocolVersion.INDEX_FULL });
            try
            {
               user.login(((User)admin.findUserDBObjectById(userId, null)).getName(), PASSWORD);
               user.syncObjects();
               for(long objectId : new long[] { parentId, childId })
               {
                  AbstractObject object = user.findObjectById(objectId);
                  assertNotNull(object);
                  assertEquals(Long.valueOf(admin.getEffectiveRights(objectId, userId)), object.getCachedEffectiveRights());
               }
            }
            finally
            {
               user.disconnect();
            }
         }
      }
      finally
      {
         if (parentId != 0)
            admin.deleteObject(parentId);
         for(int id : principals)
            admin.deleteUserDBObject(id);
      }
   }

   @Test
   public void testAccessChangeBetweenSyncAndSubscription() throws Exception
   {
      NXCSession admin = connectAndLogin();
      long rootId = 0;
      try
      {
         // Network map groups are not polled, so the only updates they get come from access changes
         rootId = admin.createObject(new NXCObjectCreationData(AbstractObject.OBJECT_NETWORKMAPGROUP, "handoff-" + UUID.randomUUID(), AbstractObject.NETWORKMAPROOT));
         long childId = admin.createObject(new NXCObjectCreationData(AbstractObject.OBJECT_NETWORKMAPGROUP, "child", rootId));
         NXCObjectModificationData acl = new NXCObjectModificationData(rootId);
         acl.setInheritAccessRights(false);
         acl.setACL(Arrays.asList(new AccessListElement(admin.getUserId(), admin.getEffectiveRights(rootId))));

         // ACL is changed after object synchronization has completed but before client subscribes to object changes
         NXCSession observer = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true) {
            private boolean firstSubscription = true;

            @Override
            public void subscribe(String channel) throws IOException, NXCException
            {
               if (firstSubscription && CHANNEL_OBJECTS.equals(channel))
               {
                  firstSubscription = false;
                  admin.modifyObject(acl);
               }
               super.subscribe(channel);
            }
         };
         observer.connect(new int[] { ProtocolVersion.INDEX_FULL });
         try
         {
            observer.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);
            BlockingQueue<Long> childUpdates = new LinkedBlockingQueue<>();
            observer.addListener(n -> {
               if ((n.getCode() == SessionNotification.OBJECT_CHANGED) && (n.getSubCode() == childId))
                  childUpdates.add(childId);
            });
            observer.syncObjects();
            assertNotNull(childUpdates.poll(10, TimeUnit.SECONDS), "Access change made before subscription was not delivered");
         }
         finally
         {
            observer.disconnect();
         }

         // Access change made while unsubscribed after initial synchronization is not replayed on later subscription
         NXCSession resubscriber = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
         resubscriber.connect(new int[] { ProtocolVersion.INDEX_FULL });
         try
         {
            resubscriber.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);
            BlockingQueue<Long> childUpdates = new LinkedBlockingQueue<>();
            resubscriber.addListener(n -> {
               if ((n.getCode() == SessionNotification.OBJECT_CHANGED) && (n.getSubCode() == childId))
                  childUpdates.add(childId);
            });
            resubscriber.syncObjects();
            resubscriber.unsubscribe(NXCSession.CHANNEL_OBJECTS);
            acl.setInheritAccessRights(true);
            admin.modifyObject(acl);
            resubscriber.subscribe(NXCSession.CHANNEL_OBJECTS);
            assertNull(childUpdates.poll(2, TimeUnit.SECONDS));
         }
         finally
         {
            resubscriber.disconnect();
         }
      }
      finally
      {
         if (rootId != 0)
            admin.deleteObject(rootId);
      }
   }

   private static void enableLogin(NXCSession admin, int userId) throws Exception
   {
      admin.setUserPassword(userId, PASSWORD, null);
      admin.syncUserDatabase();
      User user = (User)admin.findUserDBObjectById(userId, null);
      user.setFlags(user.getFlags() & ~AbstractUserObject.CHANGE_PASSWORD);
      admin.modifyUserDBObject(user, AbstractUserObject.MODIFY_FLAGS);
   }

   @Test
	public void testObjectSync() throws Exception
	{
      final NXCSession session = connectAndLogin();

		session.syncObjects();
		final AbstractObject obj = session.findObjectById(1);
      assertNotNull(obj);
		assertEquals(1, obj.getObjectId());
		assertEquals("Entire Network", obj.getObjectName());

	}

   @Test
	public void testPartialObjectSync() throws Exception
	{
		final NXCSession session = connectAndLogin();

      session.syncObjectSet(new long[] { 1, 2, 3 });
		Thread.sleep(1000);

		final AbstractObject obj = session.findObjectById(1);
      assertNotNull(obj);
		assertEquals(1, obj.getObjectId());
		assertEquals("Entire Network", obj.getObjectName());

		assertNull(session.findObjectById(4));

	}

   @Test
	public void testObjectIsParent() throws Exception
	{
		final NXCSession session = connectAndLogin();

		session.syncObjects();

		AbstractObject object = session.findObjectById(2);
		assertFalse(object.isChildOf(1));

		object = TestHelper.findManagementServer(session);
		assertTrue(object.isChildOf(1));

	}

   @Test
	public void testSetObjectName() throws Exception
	{
		final NXCSession session = connectAndLogin();

		session.syncObjects();

		AbstractObject object = session.findObjectById(1);
		assertNotNull(object);

		session.setObjectName(1, "test name");
		Thread.sleep(1000);	// Object update should be received from server
		object = session.findObjectById(1);
		assertNotNull(object);
		assertEquals("test name", object.getObjectName());

		session.setObjectName(1, "Entire Network");
		Thread.sleep(1000);	// Object update should be received from server
		object = session.findObjectById(1);
		assertNotNull(object);
		assertEquals("Entire Network", object.getObjectName());

	}

   @Test
	public void testObjectCreateAndDelete() throws Exception
	{
		final NXCSession session = connectAndLogin();

		session.syncObjects();

		AbstractObject object = session.findObjectById(2);
		assertNotNull(object);

		long id = 0;
		try
		{
   		NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, "TestNode", 2);
   		cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
   		cd.setIpAddress(new InetAddressEx(InetAddress.getByName("192.168.10.1"), 0));
   		id = session.createObject(cd);
   		assertFalse(id == 0);

   		Thread.sleep(1000);	// Object update should be received from server

   		object = session.findObjectById(id);
   		assertNotNull(object);
   		assertEquals("TestNode", object.getObjectName());

   		session.deleteObject(id);

   		Thread.sleep(1000);	// Object update should be received from server
   		object = session.findObjectById(id);
   		assertNull(object);
		}
		finally
		{
		   if (id != 0)
		   {
	         object = session.findObjectById(id);
		      if (object != null)
	            session.deleteObject(id);
		   }
      }
	}

   @Test
	public void testObjectFind() throws Exception
	{
		final NXCSession session = connectAndLogin();

		session.syncObjects();

		AbstractObject object = session.findObjectById(1, EntireNetwork.class);
		assertNotNull(object);
		assertEquals(1, object.getObjectId());

		object = session.findObjectById(1, Node.class);
		assertNull(object);

	}

	private void printObject(AbstractObject object, int level)
	{
		for(int i = 0; i < level; i++)
			System.out.print(' ');
		System.out.println(object.getObjectName());

		for(AbstractObject o : object.getChildrenAsArray())
			printObject(o, level + 2);
	}

	@Test
	public void testObjectTree() throws Exception
	{
		final NXCSession session = connectAndLogin();

		session.syncObjects();

		AbstractObject object = session.findObjectById(1, EntireNetwork.class);
		assertNotNull(object);
		assertEquals(1, object.getObjectId());

		printObject(object, 0);

	}

	@Test
	public void testSubnetMaskBits() throws Exception
	{
      final NXCSession session = connectAndLogin();

      session.syncObjects();

      AbstractObject object = session.findObjectById(1, EntireNetwork.class);
      assertNotNull(object);
      assertEquals(1, object.getObjectId());

      Set<AbstractObject> subnets = object.getAllChildren(AbstractObject.OBJECT_SUBNET);
      for(AbstractObject s : subnets)
      {
         System.out.println(s.getObjectName() + ": " + ((Subnet)s).getNetworkAddress().toString());
      }

	}
}
