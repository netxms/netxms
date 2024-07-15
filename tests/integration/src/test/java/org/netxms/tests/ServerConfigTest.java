/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerVariable;

/**
 * Test functionality related to server configuration cariables
 *
 */
public class ServerConfigTest extends AbstractSessionTest
{
   @Test
	public void testServerVariables() throws Exception
	{
		final NXCSession session = connectAndLogin();

		// Create test variable
		session.setServerVariable("TestVariable", "TestValue");
		
		// Get full list
		Map<String, ServerVariable> varList = session.getServerVariables();
      assertTrue(varList.size() > 0); // Normally server should have at least one variable

		// Get variable TestVariable
		ServerVariable var = varList.get("TestVariable");
      assertNotNull(var);
		assertEquals("TestValue", var.getValue());

		// Delete test variable
		session.deleteServerVariable("TestVariable");
		
		// Get variable list again and check that test variable was deleted
		varList = session.getServerVariables();
		var = varList.get("TestVariable");
      assertNull(var);

		session.disconnect();
	}

   @Test
	public void testGetPublicVariable() throws Exception
	{
      final NXCSession session = connectAndLogin();

      String value = session.getPublicServerVariable("Client.DashboardDataExport.EnableInterpolation");
      assertNotNull(value);
      System.out.println("value=" + value); 

      session.disconnect();
	}
}
