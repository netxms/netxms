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
import java.net.InetAddress;
import java.util.Set;
import org.junit.jupiter.api.Test;
import org.netxms.base.InetAddressEx;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Subnet;
import org.netxms.utilities.TestHelper;

/**
 * @author Victor
 *
 */
public class ObjectTest extends AbstractSessionTest
{
   @Test
	public void testObjectSync() throws Exception
	{
      final NXCSession session = connectAndLogin();

		session.syncObjects();
		final AbstractObject obj = session.findObjectById(1);
      assertNotNull(obj);
		assertEquals(1, obj.getObjectId());
		assertEquals("Entire Network", obj.getObjectName());

		session.disconnect();
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

		session.disconnect();
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
		
		session.disconnect();
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
		
		session.disconnect();
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
	      session.disconnect();   
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
		
		session.disconnect();
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
		
		session.disconnect();
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
      
      session.disconnect();
	}
}
