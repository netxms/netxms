/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;

/**
 * Test script library functionality
 */
public class ScriptLibraryTest extends AbstractSessionTest
{
   @Test
   public void testGet() throws Exception
   {
      final NXCSession session = connectAndLogin();

      List<Script> library = session.getScriptLibrary();
      assertNotNull(library);
      assertFalse(library.isEmpty());
      assertFalse(library.get(0).getName().isEmpty());

      Script script = session.getScript(library.get(0).getId());
      assertEquals(library.get(0).getId(), script.getId());
      assertEquals(library.get(0).getName(), script.getName());
      assertFalse(script.getSource().isEmpty());

      session.disconnect();
   }

   @Test
   public void testCreateAndExecute() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node testNode = TestHelper.findManagementServer(session);
      assertNotNull(testNode);

      Map<String, String> inputFields = new HashMap<String, String>();
      inputFields.put("inputFieldName", "inputFieldValue");
      inputFields.put("field2", "value2");

      TextOutputChecker listener = new TextOutputChecker();
      String scriptName = "IntegrationTest_" + System.currentTimeMillis();
      String scriptSource = "println($object.id, $INPUT);";
      long scriptId = session.modifyScript(0, scriptName, scriptSource);

      try
      {
         session.executeLibraryScript(testNode.getObjectId(), scriptName, inputFields, /* maskedFields */null, listener);
         assertFalse(listener.executionFailure);
         assertEquals(testNode.getObjectId() + " {inputFieldName=inputFieldValue, field2=value2}", listener.getFirstLine());
      }
      finally
      {
         session.deleteScript(scriptId);
         session.disconnect();
      }
   }

   private static class TextOutputChecker implements TextOutputListener
   {
      StringBuilder output = new StringBuilder();
      boolean executionFailure = false;

      public String getFirstLine()
      {
         String text = output.toString();
         int i = text.indexOf('\n');
         return (i != -1) ? text.substring(0, i).trim() : text.trim();
      }

      /**
       * @see org.netxms.client.TextOutputListener#messageReceived(java.lang.String)
       */
      @Override
      public void messageReceived(String text)
      {
         if (output.length() == 0)
            System.out.println("---- output -----");
         output.append(text);
         System.out.print(text);
      }

      /**
       * @see org.netxms.client.TextOutputListener#setStreamId(long)
       */
      @Override
      public void setStreamId(long streamId)
      {
      }

      /**
       * @see org.netxms.client.TextOutputListener#onSuccess()
       */
      @Override
      public void onSuccess()
      {
         System.out.println("\n---- end of output -----");
      }

      /**
       * @see org.netxms.client.TextOutputListener#onFailure(java.lang.Exception)
       */
      @Override
      public void onFailure(Exception e)
      {
         System.out.println("\n---- end of output -----");
         executionFailure = true;
         if (e != null)
            e.printStackTrace();
      }
   }
}
