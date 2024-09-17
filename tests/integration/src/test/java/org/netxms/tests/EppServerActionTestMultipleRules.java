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
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ServerActionType;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

/**
 * Testing the creation of a server action and timer cancellation and its functionality. 
 * In this class, tests are performed with multiple rules.
 */
public class EppServerActionTestMultipleRules extends AbstractSessionTest
{
   final String TEMPLATE_NAME_A = "A template name";
   final String TEMPLATE_NAME_B = "B template name";
   final String COMMENT_FOR_SEARCHING_RULE_A = "Rule A for testing server action";
   final String COMMENT_FOR_SEARCHING_RULE_B = "Rule B for testing server action";
   final String SOURCE_FOR_RULE_A = "WritePersistentStorage(\"Key to set A\", $event.code);";
   final String SOURCE_FOR_RULE_B = "WritePersistentStorage(\"Key to set B\", $event.code);";
   final String SCRIPT_NAME_FOR_SEARCHING_A = "scriptNameA";
   final String SCRIPT_NAME_FOR_SEARCHING_B = "scriptNameB";
   final String ACTION_NAME_A = "A test action name";
   final String ACTION_NAME_B = "B test action name";
   final String TIMER_KEY_1 = "timer key 1";
   final String TIMER_KEY_2 = "timer key 2";
   final String TIME_4_SEK = "4";
   final String TIME_6_SEK = "6";

   /**
    * Find the action from the session list based on given name or created new based on the specified parameters.
    * 
    * @param session
    * @param actionName
    * @param scriptName
    * @return action
    * @throws Exception
    */
   public ServerAction findOrCreateAction(NXCSession session, String actionName, String scriptName) throws Exception
   {
      List<ServerAction> actionList = session.getActions();
      for(ServerAction action : actionList)
      {
         if (action.getName().equals(actionName))
         {
            return action;
         }
      }
      ServerAction action = new ServerAction(session.createAction(actionName));
      action.setName(actionName);
      action.setType(ServerActionType.NXSL_SCRIPT);
      action.setRecipientAddress(scriptName);
      session.modifyAction(action);
      return action;
   }
   
   /**
    * Counts how many entries in the schedule task list have a given key.
    * 
    * @param session
    * @param timerKey
    * @return count
    * @throws Exception
    */
   public int countScheduledTasks(NXCSession session, String timerKey) throws Exception
   {
      List<ScheduledTask> shceduledTaskList = session.getScheduledTasks();
      int count = 0;
      
      for (ScheduledTask task : shceduledTaskList)
      {
         if (task.getKey().equals(timerKey))
         {
            count++;
         }
      }
      return count;
   }

   /**
    * Creates a new EPP rule with specific parameters.
    * 
    * @param session
    * @param node
    * @param policy
    * @param templateName
    * @param commentForSearch
    * @return EPP rule for test
    * @throws Exception
    */
   public EventProcessingPolicyRule createTestRule(NXCSession session, AbstractObject node, EventProcessingPolicy policy, String templateName, String commentForSearch) throws Exception
   {
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearch, eventTestTemplate, node);
      return testRule;

   }
   
   /**
    * Creates a new action execution configuration with specific parameters.
    * The variable modified locally after the method is used should be persisted on the server.
    * 
    * @param session
    * @param testRule
    * @param scriptName
    * @param source
    * @param actionName
    * @return actionExecution
    * @throws Exception
    */
   public ActionExecutionConfiguration createActionExecution(NXCSession session, EventProcessingPolicyRule testRule, String scriptName, String source, String actionName) throws Exception
   {
      String testScriptName = TestHelper.findOrCreateScript(session, scriptName, source);
      ServerAction action = findOrCreateAction(session, actionName, testScriptName);

      ActionExecutionConfiguration actionExecution = new ActionExecutionConfiguration(action.getId(), null, null, null, null);                                                                                                                             // epp
      List<ActionExecutionConfiguration> actionList = new ArrayList<ActionExecutionConfiguration>();
      actionList.add(actionExecution);
      testRule.setActions(actionList);
      testRule.setTimerCancellations(new ArrayList<String>());

      return actionExecution;
   }
   /**
    * Thread sleep for 2 sek
    * 
    * @throws Exception
    */
   public void sleep2000() throws Exception
   {
      Thread.sleep(2000);
   }
   /**
    * Thread sleep for 3 sek
    * 
    * @throws Exception
    */
   public void sleep3000() throws Exception
   {
      Thread.sleep(3000);
   }
   public void createCustomAtribute(NXCSession session, AbstractObject node, String cusAttName, String cusAttValue) throws Exception
   {
      NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());

      CustomAttribute customAttribute = new CustomAttribute(cusAttValue, 0, 0);
      Map<String, CustomAttribute> customAttributes = new HashMap<>();    
      customAttributes.put(cusAttName, customAttribute);
      md.setCustomAttributes(customAttributes);
     
      session.modifyObject(md);
   }
   
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|       6       |"timer key 1"  |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|               |               |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |         "timer key 1"         |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction1() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(TIME_6_SEK);
      actionA.setTimerKey(TIMER_KEY_1);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add(TIMER_KEY_1);
      testRuleB.setTimerCancellations(cancelationKeyList);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, TIMER_KEY_1), 0);
      
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();
      
      assertEquals(countScheduledTasks(session, TIMER_KEY_1), 1);
      
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      assertEquals(countScheduledTasks(session, TIMER_KEY_1), 0);// make sure that the entry has disappeared from the scheduled tasks
      
      sleep3000();
      
      // make sure that PS has only 1 entry with B rule key (because of cancellation key)
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      //to run next tests
      session.deletePersistentStorageValue("Key to set B"); 
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      
      session.closeEventProcessingPolicy();

   }
   
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|               |               |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      4        | "timer key 2" |             |     4         | "timer key 2" |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |                               |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction2() throws Exception 
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setBlockingTimerKey(TIMER_KEY_2);
      actionA.setSnoozeTime(TIME_4_SEK);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      ActionExecutionConfiguration actionB = createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      actionB.setBlockingTimerKey(TIMER_KEY_2);
      actionB.setSnoozeTime(TIME_4_SEK);

      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, TIMER_KEY_2), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      // make sure that schedule tasks has only 1 entry this key (because of blocking key)
      assertEquals(countScheduledTasks(session, TIMER_KEY_2), 1);
      
      sleep3000();

      // make sure that PS has only 1 entry with A rule key (because of blocking key)
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      //to run next test
      session.deletePersistentStorageValue("Key to set A"); 
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();
   }
   
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|      6        |               |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      6        | "timer key 2" |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |          "timer key 2"        |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction3() throws Exception 
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(TIME_6_SEK);
      actionA.setBlockingTimerKey(TIMER_KEY_2);
      actionA.setSnoozeTime(TIME_6_SEK);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add(TIMER_KEY_2);
      testRuleB.setTimerCancellations(cancelationKeyList);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, TIMER_KEY_2), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      assertEquals(countScheduledTasks(session, TIMER_KEY_2), 1);// can find only 1 entry because A rule doesn't have delay key
      
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      assertEquals(countScheduledTasks(session, TIMER_KEY_2), 0);//entry with key has disappeared from the scheduled tasks

      sleep3000();
      
      // make sure that PS has only 2 entries with A and B ruleS keys
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      //to run next test
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set B");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      
      session.closeEventProcessingPolicy();

   }
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|      6        | "timer key 1" |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      6        | "timer key 2" |             |               | "timer key 2" |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |          "timer key 1"        |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction4() throws Exception 
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerKey(TIMER_KEY_1);
      actionA.setTimerDelay(TIME_6_SEK);
      actionA.setBlockingTimerKey(TIMER_KEY_2);
      actionA.setSnoozeTime(TIME_6_SEK);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      ActionExecutionConfiguration actionB = createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      actionB.setBlockingTimerKey(TIMER_KEY_2);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add(TIMER_KEY_1);
      testRuleB.setTimerCancellations(cancelationKeyList);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, TIMER_KEY_2), 0);
      assertEquals(countScheduledTasks(session, TIMER_KEY_1), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      //can find 2 entries because A rule have blocking and delay keys and time
      assertEquals(countScheduledTasks(session, TIMER_KEY_1), 1);
      assertEquals(countScheduledTasks(session, TIMER_KEY_2), 1);

      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      assertEquals(countScheduledTasks(session, TIMER_KEY_1), 0);

      sleep3000();
      
      //rule A blocked rule B, Rule B canceled rule A
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      
      session.closeEventProcessingPolicy();

   }
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|      4        | null          |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|               |               |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |          ""                   |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction5() throws Exception 
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerKey(null);
      actionA.setTimerDelay(TIME_4_SEK);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add("");
      testRuleB.setTimerCancellations(cancelationKeyList);
      createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      sleep3000();

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      //to run next test
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set B");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.closeEventProcessingPolicy();

   }
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|      4        |               |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      4        |    null       |             |               |    null       |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |                               |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction6() throws Exception 
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(TIME_4_SEK);
      actionA.setBlockingTimerKey(null);
      actionA.setSnoozeTime(TIME_4_SEK);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      ActionExecutionConfiguration actionB = createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      actionB.setBlockingTimerKey(null);

      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      sleep3000();

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      //to run next test
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set B");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.closeEventProcessingPolicy();

   }
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|      4        |     "Akey"    |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      4        | "aKey"        |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |          "akey"               |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction7() throws Exception // at this moment keys are not case-sensitive
   //If keys become case-sensitive, this test will need to be modified 
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerKey("Akey");
      actionA.setTimerDelay(TIME_4_SEK);
      actionA.setBlockingTimerKey("aKey");
      actionA.setSnoozeTime(TIME_4_SEK);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add("akey");
      testRuleB.setTimerCancellations(cancelationKeyList);
      createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task don't contain entries with the given keys
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, "Akey"), 0);
      assertEquals(countScheduledTasks(session, "aKey"), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();
      
      //can find 2 entries ,B rule cancellation key doesn't work, case is not sensitive
      assertEquals(countScheduledTasks(session, "Akey"), 1);
      assertEquals(countScheduledTasks(session, "aKey"), 1);
      
      sleep3000();
 
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A")); 
      
      //to run next test
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set B");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.closeEventProcessingPolicy();

   }
   
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|               |               |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      4        | "Akey"        |             |               |   "akey"      |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |                               |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction8() throws Exception // at this moment keys are not case-sensitive
   //If keys become case-sensitive, this test will need to be modified 
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setBlockingTimerKey("Akey");
      actionA.setSnoozeTime(TIME_4_SEK);
      
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      ActionExecutionConfiguration actionB = createActionExecution(session, testRuleB, SCRIPT_NAME_FOR_SEARCHING_B, SOURCE_FOR_RULE_B, ACTION_NAME_B);
      actionB.setBlockingTimerKey("akey");
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task don't contain entries with the given keys
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, "Akey"), 0);
      assertEquals(countScheduledTasks(session, "akey"), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      assertEquals(countScheduledTasks(session, "Akey"), 1);

      sleep3000();
 
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A")); // Case is not sensitive
      
      //to run next test
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set B");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.closeEventProcessingPolicy();
   }
   
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|      6        |      "%N"     |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      6        |     "%g"      |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |             "%g"              |
   //+-------------------------------+             +-------------------------------+ 
   @Test
   public void testCreateServerAction9() throws Exception
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerKey("%N");
      actionA.setTimerDelay(TIME_6_SEK);
      actionA.setBlockingTimerKey("%g");
      actionA.setSnoozeTime(TIME_6_SEK);
           
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add("%g");
      testRuleB.setTimerCancellations(cancelationKeyList);
    
      session.saveEventProcessingPolicy(policy);
      
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      //checking that the macros expand correctly in blocking and delay key
      assertEquals(countScheduledTasks(session, TEMPLATE_NAME_A), 1);
      assertEquals(countScheduledTasks(session, node.getGuid().toString()), 1);

      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);
     
      sleep2000();
      
      //checking that the macros expand correctly in cancellation key
      assertEquals(countScheduledTasks(session, node.getGuid().toString()), 0);
      
      sleep3000();
 
      //to run next test
      session.deletePersistentStorageValue("Key to set A"); 
      session.deletePersistentStorageValue("Key to set B"); 
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.closeEventProcessingPolicy();
   }
   //           RULE A                                         RULE B
   // +------------------------------+             +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |             |   DELAY TIME  |   DELAY KEY   |
   //|      4        |   "123key"    |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |             | SNOOZE TIME   |BLOCKING KEY   |
   //|      4        |   "3key"      |             |               |               |
   //|_______________|_______________|             |_______________|_______________|
   //|        CANCELATION KEY        |             |        CANCELATION KEY        |
   //|                               |             |          "23key"              |
   //+-------------------------------+             +-------------------------------+
   @Test
   public void testCreateServerAction10() throws Exception 
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerKey("123key");
      actionA.setTimerDelay(TIME_4_SEK);
      actionA.setBlockingTimerKey("3key");
      actionA.setSnoozeTime(TIME_4_SEK);
            
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add("23key");
      testRuleB.setTimerCancellations(cancelationKeyList);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task don't contain entries with the given keys
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, "123key"), 0);
      assertEquals(countScheduledTasks(session, "3key"), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      //make sure that the cancellation key does not work despite the difference in just one character with delay and blockin key
      assertEquals(countScheduledTasks(session, "123key"), 1);
      assertEquals(countScheduledTasks(session, "3key"), 1);

      sleep3000();
 
      //to run next test
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set B");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.closeEventProcessingPolicy();

   }
   //           RULE A                                              RULE B
   // +--------------------------------------------------+         +------------------------------+
   //|   DELAY TIME  |   DELAY KEY                       |         |   DELAY TIME  |   DELAY KEY   |
   //|      6        |   "%{nameForTest}"                |         |               |               |
   //|_______________|___________________________________|         |_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY                       |         | SNOOZE TIME   |BLOCKING KEY   |
   //|      6        |"%{nameForTest:test_default_value}"|         |               |               |
   //|_______________|___________________________________|         |_______________|_______________|
   //|        CANCELATION KEY                            |         |        CANCELATION KEY        |
   //|                                                   |         |          "%{nameForTest}"     |
   //+---------------------------------------------------+         +-------------------------------+
   @Test
   public void testCreateServerAction11() throws Exception 
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerKey("%{nameForTest}");
      actionA.setTimerDelay(TIME_6_SEK);
      actionA.setBlockingTimerKey("%{nameForTest:test_default_value}");
      actionA.setSnoozeTime(TIME_6_SEK);
            
      EventProcessingPolicyRule testRuleB = createTestRule(session, node, policy, TEMPLATE_NAME_B, COMMENT_FOR_SEARCHING_RULE_B);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add("%{nameForTest}");
      testRuleB.setTimerCancellations(cancelationKeyList);

      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task don't contain entries with the given keys
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));
      assertEquals(countScheduledTasks(session, "valueForTest"), 0);
      
      createCustomAtribute(session, node, "nameForTest", "valueForTest"); // creating custom attribute 
      
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      assertEquals(countScheduledTasks(session, "valueForTest"), 2); //make sure that both macros expanded identically and correctly

      session.sendEvent(0, TEMPLATE_NAME_B, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();
      
      //make sure that makros in the cancellation key expanded correctly and canceled both entries in the schedule task
      assertEquals(countScheduledTasks(session, "valueForTest"), 0);

      sleep3000();
 
      //to run next test
      session.deletePersistentStorageValue("Key to set B");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set B"));

      session.closeEventProcessingPolicy();

   }

}
