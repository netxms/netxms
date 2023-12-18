package org.netxms.tests;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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
   /**
    * 
    * @param session
    * @param key
    * @return value in persistent storage based on the key
    * @throws Exception
    */
   public String findPsValueByKey(NXCSession session, String key) throws Exception 
   {
      HashMap<String, String> allPersistentStorageValue = session.getPersistentStorageList();
      return allPersistentStorageValue.get(key);
   }

   /**
    * Test persistent storage value change by event processing policy 
    * @throws Exception
    */
   public void testChangePersistantStorage() throws Exception
   {
      final NXCSession session = connect();
      session.syncObjects();

      final String templateName = "Test Name for Persistent Storage Test";
      final String commentForSearching = "Rule for testing persistant storage";
      AbstractObject node = TestHelper.findManagementServer(session);
      final String pStoragekey = "TestKey";
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);

      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed;
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearching, eventTestTemplate, node);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);

      
      assertTrue(testRule.getPStorageSet().isEmpty()); // checking that PS in the rule is empty
      assertNull(findPsValueByKey(session, pStoragekey));// checking that PS does not contain a value for the given key

      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(pStoragekey, "testValue");// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      assertEquals(rulePsSetList.get(pStoragekey), findPsValueByKey(session, pStoragekey));
      // Checking that the value in the PS is the same as the value in the PS rule

      rulePsSetList.put(pStoragekey, "%c");// setting macros to the specified value to pass into the rule.
      testRule.setPStorageSet(rulePsSetList);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      assertEquals(String.valueOf(eventTestTemplate.getCode()), findPsValueByKey(session, pStoragekey));
      // checking for correct macros expansion

      List<String> rulePsListForDelete = new ArrayList<>();
      rulePsListForDelete.add("nonExistentKey"); // Setting a nonexistent key for deletion
      testRule.setPStorageDelete(rulePsListForDelete);
      Map<String, String> rulePsSet = new HashMap<>(); // and empty PS set, to simulate the creation of a new rule
      testRule.setPStorageSet(rulePsSet);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      assertEquals(String.valueOf(eventTestTemplate.getCode()), findPsValueByKey(session, pStoragekey)); // checking that value wasn't deleted from PS

      rulePsListForDelete.add(pStoragekey); // Setting a key for deletion
      testRule.setPStorageDelete(rulePsListForDelete);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      assertNull(findPsValueByKey(session, pStoragekey)); // checking that value was deleted from PS

      rulePsListForDelete.clear(); // remove keys for deleting, to repeat test multiple times

      testRule.setPStorageDelete(rulePsListForDelete);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);

      session.closeEventProcessingPolicy();
      session.disconnect();

   }
}
