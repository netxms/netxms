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
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
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

	}

   @Test
	public void testGetPublicVariable() throws Exception
	{
      final NXCSession session = connectAndLogin();

      String value = session.getPublicServerVariable("Client.DashboardDataExport.EnableInterpolation");
      assertNotNull(value);
      System.out.println("value=" + value);

	}

   /**
    * Secret (password type) variables are write-only: server reports placeholder instead of non-empty value, and
    * placeholder sent as new value keeps the current one.
    */
   @Test
   public void testSecretVariables() throws Exception
   {
      final NXCSession session = connectAndLogin();

      final String secretVariable = "RADIUS.SecondarySecret";
      final String placeholder = "********";

      try
      {
         session.setServerVariable(secretVariable, "test-secret");
         assertEquals(placeholder, session.getServerVariables().get(secretVariable).getValue());

         session.setServerVariable(secretVariable, placeholder);
         assertEquals(placeholder, session.getServerVariables().get(secretVariable).getValue());

         session.setServerVariable(secretVariable, "");
         assertEquals("", session.getServerVariables().get(secretVariable).getValue());

         // Placeholder keeps current value, so empty secret stays empty
         session.setServerVariable(secretVariable, placeholder);
         assertEquals("", session.getServerVariables().get(secretVariable).getValue());
      }
      finally
      {
         // Original value cannot be read back, so variable is returned to its default
         session.setServerVariable(secretVariable, "netxms");
         session.disconnect();
      }
   }

   /**
    * Check that setting variable to given value is rejected with RCC.INVALID_ARGUMENT
    */
   private static void assertRejected(NXCSession session, String name, String value)
   {
      NXCException e = assertThrows(NXCException.class, () -> session.setServerVariable(name, value), "value \"" + value + "\" accepted for " + name);
      assertEquals(RCC.INVALID_ARGUMENT, e.getErrorCode());
   }

   /**
    * Check that setting variable to given value is accepted and stored as is
    */
   private static void assertAccepted(NXCSession session, String name, String value) throws Exception
   {
      session.setServerVariable(name, value);
      assertEquals(value, session.getServerVariables().get(name).getValue());
   }

   @Test
   public void testValueValidation() throws Exception
   {
      final NXCSession session = connectAndLogin();

      final String integerVariable = "Objects.Nodes.DefaultDecommissionExpirationTime";
      final String portVariable = "Syslog.ListenPort";
      final String booleanVariable = "Topology.AdHocRequest.IncludePhysicalLinks";
      final String shiftVariable = "Objects.StatusCalculation.Shift";
      final String choiceVariable = "Agent.DefaultCacheMode";

      Map<String, ServerVariable> variables = session.getServerVariables();
      String[] names = { integerVariable, portVariable, booleanVariable, shiftVariable, choiceVariable };
      String[] originalValues = new String[names.length];
      for(int i = 0; i < names.length; i++)
         originalValues[i] = variables.get(names[i]).getValue();

      try
      {
         for(String v : new String[] { "60junk", "abc", "", "-1", "4294967296", "-4294967236", "4294967356" })
            assertRejected(session, integerVariable, v);
         assertAccepted(session, integerVariable, "45");

         assertRejected(session, portVariable, "70000");
         assertAccepted(session, portVariable, "65535");

         assertRejected(session, booleanVariable, "maybe");
         assertAccepted(session, booleanVariable, "true");
         assertAccepted(session, booleanVariable, "0");

         assertRejected(session, shiftVariable, "-5");
         assertRejected(session, shiftVariable, "5");
         assertRejected(session, shiftVariable, "abc");
         assertAccepted(session, shiftVariable, "-2");

         assertRejected(session, choiceVariable, "7");
         assertAccepted(session, choiceVariable, "1");

         // Variables not known to the server are created as strings and not validated
         assertAccepted(session, "TestVariable", "60junk");
      }
      finally
      {
         for(int i = 0; i < names.length; i++)
            session.setServerVariable(names[i], originalValues[i]);
         session.deleteServerVariable("TestVariable");
      }
   }
}
