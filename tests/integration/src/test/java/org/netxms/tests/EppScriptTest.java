/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.io.IOException;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;
/**
 * Testing script creating functionality in EPPR actions
 */
public class EppScriptTest extends AbstractSessionTest
{
   private final String key = "Key to set";
   
   private NXCSession session;
   private EventProcessingPolicy policy = null;
   private EventProcessingPolicyRule testRule = null;
   
   @Test
   public void testCreatePersistant() throws Exception
   {
      session = connectAndLogin();
      session.syncObjects();

      final String templateName = "Name for script test";
      final String commentForSearching = "Rule for testing script";
      AbstractObject node = TestHelper.findManagementServer(session);
      final String testActionScript = "WritePersistentStorage(\"Key to set\", \"Value to set\");";
      final String testActionScript2 = "WritePersistentStorage(\"Key to set\", $event.code);";
      final String testActionScript3 = "WritePersistentStorage(\"Key to set\", $nonExist.code);";

      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);

      policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed;
      
      testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearching, eventTestTemplate, node);
      assertTrue(testRule.getActionScript() == null || testRule.getActionScript().equals("")); // checking that CA in the rule is empty 
       
      testRule.setActionScript(testActionScript);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(200);
      assertEquals(TestHelperForEpp.findPsValueByKey(session, key), "Value to set");

      testRule.setActionScript(testActionScript2);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(200);
      assertEquals(TestHelperForEpp.findPsValueByKey(session, key), String.valueOf(eventTestTemplate.getCode()));
      
      session.deletePersistentStorageValue(key);
      
      testRule.setActionScript(testActionScript3);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(200);
      assertNull(TestHelperForEpp.findPsValueByKey(session, key));

      testRule.setActionScript("");
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
   }
   
   /**
    * Function will restore all data to initial state for test run next time
    * 
    * @throws IOException
    * @throws NXCException
    */
   @AfterEach
   void resetDataForTest() throws IOException, NXCException
   {
      if (policy != null && testRule != null)
      {
         testRule.setActionScript("");
         session.saveEventProcessingPolicy(policy);  
         session.closeEventProcessingPolicy();
         session.deletePersistentStorageValue(key);
         session.disconnect();
      }      
   }
}
