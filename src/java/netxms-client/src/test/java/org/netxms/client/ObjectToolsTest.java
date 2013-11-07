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

import java.util.List;

import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;

/**
 * 
 * Test object tools functionality
 *
 */
public class ObjectToolsTest extends SessionTest
{
	public void testGet() throws Exception
	{
		final NXCSession session = connect();
		
		List<ObjectTool> tools = session.getObjectTools();
		for(ObjectTool tool : tools)
		{
			System.out.println(" >>Tool>> " + tool.getId() + " " + tool.getName());
		}
		
		session.disconnect();
	}
	
	public void testGetDetails() throws Exception
	{
		final NXCSession session = connect();
		
		ObjectToolDetails td = session.getObjectToolDetails(1);
		System.out.println("Object tool details:");
		System.out.println("   id = " + td.getId());
		System.out.println("   name = " + td.getName());
		System.out.println("   type = " + td.getType());
		System.out.println("   OID = " + td.getSnmpOid());
		System.out.println("   confirmation = " + td.getConfirmationText());
		System.out.println("   columnCount = " + td.getColumns().size());
		
		session.disconnect();
	}
	
	public void testGenerateId() throws Exception
	{
		final NXCSession session = connect();

		long id = session.generateObjectToolId();
		assertFalse(id == 0);
		
		System.out.println("Object tool ID generated: " + id);
		
		session.disconnect();
	}
}
