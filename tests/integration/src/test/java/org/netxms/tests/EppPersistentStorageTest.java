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
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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
 * Test cases for event processing policy persistent storage
 */
public class EppPersistentStorageTest extends AbstractSessionTest
{
   private final String pStoragekey = "TestKey";
   
   private NXCSession session;
   private EventProcessingPolicy policy = null;
   private EventProcessingPolicyRule testRule = null;

   /**
    * Test persistent storage value change by event processing policy 
    * @throws Exception
    */
   @Test
   public void testChangePersistantStorage() throws Exception
   {
      session = connectAndLogin();
      session.syncObjects();

      final String templateName = "Test Name for Persistent Storage Test";
      final String commentForSearching = "Rule for testing persistant storage";
      AbstractObject node = TestHelper.findManagementServer(session);
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);

      policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed;
      
      testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearching, eventTestTemplate, node);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);

      assertTrue(testRule.getPStorageSet().isEmpty()); // checking that PS in the rule is empty
      assertNull(TestHelperForEpp.findPsValueByKey(session, pStoragekey));// checking that PS does not contain a value for the given key

      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(pStoragekey, "testValue");// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(200);
      assertEquals(rulePsSetList.get(pStoragekey), TestHelperForEpp.findPsValueByKey(session, pStoragekey));
      // Checking that the value in the PS is the same as the value in the PS rule

      rulePsSetList.put(pStoragekey, "%c");// setting macros to the specified value to pass into the rule.
      testRule.setPStorageSet(rulePsSetList);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(200);
      assertEquals(String.valueOf(eventTestTemplate.getCode()), TestHelperForEpp.findPsValueByKey(session, pStoragekey));
      // checking for correct macros expansion

      List<String> rulePsListForDelete = new ArrayList<>();
      rulePsListForDelete.add("nonExistentKey"); // Setting a nonexistent key for deletion
      testRule.setPStorageDelete(rulePsListForDelete);
      Map<String, String> rulePsSet = new HashMap<>(); // and empty PS set, to simulate the creation of a new rule
      testRule.setPStorageSet(rulePsSet);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(200);
      assertEquals(String.valueOf(eventTestTemplate.getCode()), TestHelperForEpp.findPsValueByKey(session, pStoragekey)); // checking that value wasn't deleted from PS

      rulePsListForDelete.add(pStoragekey); // Setting a key for deletion
      testRule.setPStorageDelete(rulePsListForDelete);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(200);
      assertNull(TestHelperForEpp.findPsValueByKey(session, pStoragekey)); // checking that value was deleted from PS

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
         testRule.setPStorageDelete(new ArrayList<String>());
         testRule.setPStorageSet(new HashMap<String, String>());
         session.deletePersistentStorageValue(pStoragekey);
         session.saveEventProcessingPolicy(policy);         
         session.closeEventProcessingPolicy();
         session.disconnect();
      }      
   }
}
