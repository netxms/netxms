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

import java.net.InetAddress;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;

/**
 * @author Victor
 *
 */
public class ObjectTest extends SessionTest
{
	public void testObjectSync() throws Exception
	{
		final NXCSession session = connect(true);
		
		session.syncObjects();
		final AbstractObject obj = session.findObjectById(1);
		assertEquals(true, obj != null);
		assertEquals(1, obj.getObjectId());
		assertEquals("Entire Network", obj.getObjectName());
		
		session.disconnect();
	}
	
	public void testPartialObjectSync() throws Exception
	{
		final NXCSession session = connect();

		session.syncObjectSet(new long[] { 1, 2, 3 }, false);
		Thread.sleep(1000);

		final AbstractObject obj = session.findObjectById(1);
		assertEquals(true, obj != null);
		assertEquals(1, obj.getObjectId());
		assertEquals("Entire Network", obj.getObjectName());
		
		assertNull(session.findObjectById(4));
		
		session.disconnect();
	}

	public void testObjectIsParent() throws Exception
	{
		final NXCSession session = connect();
		
		session.syncObjects();
		
		AbstractObject object = session.findObjectById(2);
		assertFalse(object.isChildOf(1));
		
		object = session.findObjectById(100);
		assertTrue(object.isChildOf(1));
		
		session.disconnect();
	}
	
	public void testSetObjectName() throws Exception
	{
		final NXCSession session = connect();
		
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
	
	public void testObjectCreateAndDelete() throws Exception
	{
		final NXCSession session = connect();
		
		session.syncObjects();
		
		AbstractObject object = session.findObjectById(2);
		assertNotNull(object);
		
		NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, "TestNode", 2);
		cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
		cd.setIpAddress(InetAddress.getByName("192.168.10.1"));
		long id = session.createObject(cd);
		assertFalse(id == 0);

		Thread.sleep(1000);	// Object update should be received from server

		object = session.findObjectById(id);
		assertNotNull(object);
		assertEquals("TestNode", object.getObjectName());
		
		session.deleteObject(id);

		Thread.sleep(1000);	// Object update should be received from server
		object = session.findObjectById(id);
		assertNull(object);
		
		session.disconnect();
	}

	public void testObjectFind() throws Exception
	{
		final NXCSession session = connect();
		
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
		
		for(AbstractObject o : object.getChildsAsArray())
			printObject(o, level + 2);
	}
	
	public void testObjectTree() throws Exception
	{
		final NXCSession session = connect();
		
		session.syncObjects();

		AbstractObject object = session.findObjectById(1, EntireNetwork.class);
		assertNotNull(object);
		assertEquals(1, object.getObjectId());

		printObject(object, 0);
		
		session.disconnect();
	}
}
