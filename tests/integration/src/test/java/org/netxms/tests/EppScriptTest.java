package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import org.junit.jupiter.api.Test;
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
   @Test
   public void testCreatePersistant() throws Exception
   {
      final NXCSession session = connect();
      session.syncObjects();

      final String templateName = "Name for script test";
      final String commentForSearching = "Rule for testing script";
      AbstractObject node = TestHelper.findManagementServer(session);
      final String testActionScript = "WritePersistentStorage(\"Key to set\", \"Value to set\");";
      final String testActionScript2 = "WritePersistentStorage(\"Key to set\", $event->code);";
      final String testActionScript3 = "WritePersistentStorage(\"Key to set\", $nonExist->code);";
      final String key = "Key to set";

      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);

      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed;
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearching, eventTestTemplate, node);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);

      assertTrue(testRule.getActionScript() == null || testRule.getActionScript().equals("")); // checking that CA in the rule is empty 
       
      testRule.setActionScript(testActionScript);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      Thread.sleep(200);
      assertEquals(TestHelperForEpp.findPsValueByKey(session, key), "Value to set");

      testRule.setActionScript(testActionScript2);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      Thread.sleep(200);
      assertEquals(TestHelperForEpp.findPsValueByKey(session, key), String.valueOf(eventTestTemplate.getCode()));
      
      session.deletePersistentStorageValue(key);
      
      testRule.setActionScript(testActionScript3);
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      Thread.sleep(200);
      assertNull(TestHelperForEpp.findPsValueByKey(session, key));

      testRule.setActionScript("");
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateName, node.getObjectId(), new String[] {}, null, null);
      
      session.closeEventProcessingPolicy();
      session.disconnect();

   }
}
