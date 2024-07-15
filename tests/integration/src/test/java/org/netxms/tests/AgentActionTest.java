/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.utilities.TestHelper;

/**
 * Tests for agent-related functions
 */
public class AgentActionTest extends AbstractSessionTest
{
   @Test
   public void testExecuteAction() throws Exception
   {
      final NXCSession session = connectAndLogin();

      session.executeAction((TestHelper.findManagementServer(session)).getObjectId(), TestConstants.ACTION, null, true, new TextOutputListener() {
         @Override
         public void messageReceived(String text)
         {
            System.out.print(text);
         }

         @Override
         public void setStreamId(long streamId)
         {
         }

         @Override
         public void onFailure(Exception exception)
         {
            System.out.print(exception.getLocalizedMessage());
         }

         @Override
         public void onSuccess()
         {
         }
      }, null);

      session.disconnect();
   }
}
