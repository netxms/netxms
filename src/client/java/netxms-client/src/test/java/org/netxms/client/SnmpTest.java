/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.io.File;
import java.util.Date;

import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.MibTree;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;

/**
 * Test SNMP-related functionality of client library
 */
public class SnmpTest extends AbstractSessionTest
{
	public void testSnmpObjectId() throws Exception
	{
		// Test parse and equals
		final SnmpObjectId oid1 = new SnmpObjectId(new long[] { 1, 3, 6, 1, 2, 1, 1, 1, 0 });
		final SnmpObjectId oid2 = SnmpObjectId.parseSnmpObjectId(".1.3.6.1.2.1.1.1.0");
		assertEquals(oid1, oid2);
		
		// Test toString
		assertEquals(".1.3.6.1.2.1.1.1.0", oid1.toString());
		
		// Test format exception
		try
		{
			SnmpObjectId.parseSnmpObjectId(".1.2.bad.4");
			assertFalse(true);
		}
		catch(SnmpObjectIdFormatException e)
		{
			System.out.println("Bad OID format: " + e.getMessage());
		}
		try
		{
			SnmpObjectId.parseSnmpObjectId("1.2.3.4");
			assertFalse(true);
		}
		catch(SnmpObjectIdFormatException e)
		{
			System.out.println("Bad OID format: " + e.getMessage());
		}
		try
		{
			SnmpObjectId.parseSnmpObjectId(".");
			assertFalse(true);
		}
		catch(SnmpObjectIdFormatException e)
		{
			System.out.println("Bad OID format: " + e.getMessage());
		}
	}
	
	public void testMibFileDownload() throws Exception
	{
		final NXCSession session = connect(true);
		
		Date ts = session.getMibFileTimestamp();
		System.out.println("MIB file timestamp: " + ts.toString());
		
		File f = session.downloadMibFile();
		System.out.println("MIB file downloaded to: " + f.getPath() + " (size " + f.length() + " bytes)");
		
		session.disconnect();
		
		MibTree tree = new MibTree(f);
		assertNotNull(tree.getRootObject());
		MibObject[] objects = tree.getRootObject().getChildObjects();
		assertNotNull(objects);
		assertTrue(objects.length > 0);
		
		for(int i = 0; i < objects.length; i++)
			System.out.println(objects[i].getObjectId().toString() + " " + objects[i].getName() + " " + objects[i].getFullName());
		
		MibObject o = tree.findObject(SnmpObjectId.parseSnmpObjectId(".1.3.6.1.2.1.1.1"), true);
		assertNotNull(o);
		System.out.println("Found: " + o.getObjectId().toString() + " " + o.getName() + " " + o.getFullName());

		o = tree.findObject(SnmpObjectId.parseSnmpObjectId(".1.3.6.1.100.100.100"), false);
		assertNotNull(o);
		System.out.println("Found: " + o.getObjectId().toString() + " " + o.getName() + " " + o.getFullName());

		o = tree.findObject(SnmpObjectId.parseSnmpObjectId(".1.3.6.1.100.100.100"), true);
		assertNull(o);
	}
}
