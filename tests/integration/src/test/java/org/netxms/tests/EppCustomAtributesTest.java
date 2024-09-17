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
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

public class EppCustomAtributesTest extends AbstractSessionTest
{
   /**
    * testing custom attribute creating functionality in EPPR actions
    * 
    * @throws Exception
    */
   @Test
   public void testCreateCustomAtribute() throws Exception
   {

      final NXCSession session = connectAndLogin();
      session.syncObjects();

      final String templateName = "Name for custom atributes test template";
      final String commentForSearching = "Rule for testing custom atributes";
      Node node = TestHelper.findManagementServer(session);
      final String customAttributeName = "CA name";

      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);

      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearching, eventTestTemplate, node);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);

      assertTrue(testRule.getCustomAttributeStorageSet().isEmpty()); // checking that CA in the rule is empty
      assertNull(node.getCustomAttributeValue(customAttributeName));// checking that CA of node does not contain a value for the
                                                                    // given key

      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put(customAttributeName, "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      node = TestHelper.findManagementServer(session);
      assertEquals(ruleCaSetList.get(customAttributeName), node.getCustomAttributeValue(customAttributeName));
      // Checking that the value in the CA is the same as the value in the CA rule

      ruleCaSetList.put(customAttributeName, "%c");// setting macros to the specified value to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      node = TestHelper.findManagementServer(session);
      assertEquals(String.valueOf(eventTestTemplate.getCode()), node.getCustomAttributeValue(customAttributeName));
      // checking for correct macros expansion

      List<String> ruleCaListForDelete = new ArrayList<>();
      ruleCaListForDelete.add("nonExistentKey"); // Setting a non existent key for deletion
      testRule.setCustomAttributeStorageDelete(ruleCaListForDelete);
      Map<String, String> ruleCaSet = new HashMap<>(); // and empty CA set, to simulate the creation of a new rule
      testRule.setCustomAttributeStorageSet(ruleCaSet);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      node = TestHelper.findManagementServer(session);
      assertEquals(String.valueOf(eventTestTemplate.getCode()), node.getCustomAttributeValue(customAttributeName));
      // checking that value wasn't deleted from PS

      ruleCaListForDelete.add(customAttributeName); // Setting a key for deletion
      testRule.setCustomAttributeStorageDelete(ruleCaListForDelete);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);
      node = TestHelper.findManagementServer(session);
      assertNull(node.getCustomAttributeValue(customAttributeName)); // checking that value was deleted from CA

      ruleCaListForDelete.clear();// remove keys for deleting, to repeat test multiple times
      testRule.setCustomAttributeStorageDelete(ruleCaListForDelete);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null, null);

      session.closeEventProcessingPolicy();
      session.disconnect();

   }
}
