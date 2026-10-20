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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;

/**
 * @author Victor
 *
 */
public class UserTest extends AbstractSessionTest
{
   @Test
   public void testValidatePassword() throws Exception
   {
      final NXCSession session = connectAndLogin();

      assertTrue(session.validateUserPassword(TestConstants.SERVER_PASSWORD));
   }

   @Test
   public void testUserVariables() throws Exception
   {
      final NXCSession session = connectAndLogin();
      final String name = "IntegrationTest.UserVariable";

      session.deleteUserVariable(name);
      assertNull(session.getUserVariable(name));

      session.setUserVariable(name, "test value");
      assertEquals("test value", session.getUserVariable(name));

      session.setUserVariable(name, "updated value");
      assertEquals("updated value", session.getUserVariable(name));

      Map<String, String> variables = session.getUserVariables();
      assertEquals("updated value", variables.get(name));

      session.deleteUserVariable(name);
      assertNull(session.getUserVariable(name));
      assertFalse(session.getUserVariables().containsKey(name));

      session.deleteUserVariable(name); // deleting non-existent variable is not an error

      boolean invalidName = false;
      try
      {
         session.setUserVariable("invalid name!", "test");
      }
      catch(NXCException e)
      {
         assertEquals(RCC.INVALID_OBJECT_NAME, e.getErrorCode());
         invalidName = true;
      }
      assertTrue(invalidName);
   }
}
