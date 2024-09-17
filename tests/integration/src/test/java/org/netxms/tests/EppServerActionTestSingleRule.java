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
import java.util.List;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.Script;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ServerActionType;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;
/**
 * Testing the creation of a server action and timer cancellation and its functionality. 
 * In this class, tests are performed with a single rule.
 */
public class EppServerActionTestSingleRule extends AbstractSessionTest
{
   final String TEMPLATE_NAME_A = "template name for server action";
   final String COMMENT_FOR_SEARCHING_RULE_A = "comment for testing server action";
   final String SOURCE_FOR_RULE_A = "if (ReadPersistentStorage(\"Key to set A\") == null)\n" + 
         "{\n" + 
         "  WritePersistentStorage(\"Key to set A\", $event.code);\n" + 
         "}\n" + 
         "else\n" + 
         "{\n" + 
         "  WritePersistentStorage(\"Key to set A2\", $event.code);\n" + 
         "}";
   final String SCRIPT_NAME_FOR_SEARCHING_A = "scriptNameForSingleRuleTesting";
   final String ACTION_NAME_A = "test action name for single rule";
   final String TIMER_KEY = "timer key";
   final String TIME_4_SEK = "4";
   
   /**
    * Find the name of the script in the list or created based on the specified parameters
    * 
    * @param session
    * @param scriptName
    * @param scriptSource
    * @return scriptName
    * @throws Exception
    */
   public String findOrCreateScript(NXCSession session, String scriptName, String scriptSource) throws Exception
   {
      List<Script> scriptsList = session.getScriptLibrary();
      for(Script script : scriptsList)
      {
         if (script.getName().equals(scriptName))
         {
            return scriptName;
         }
      }
      session.modifyScript(0, scriptName, scriptSource);
      return scriptName;
   }

   /**
    * Find the action from the session list based on given name or created new based on the specified parameters
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
    * Deletes schedule tasks.
    * 
    * @param session
    * @param timerKey
    * @throws Exception
    */
   public void deleteScheduledTasks(NXCSession session, String timerKey) throws Exception
   {
      List<ScheduledTask> shceduledTaskList = session.getScheduledTasks();
      
      for (ScheduledTask task : shceduledTaskList)
      {
         if (task.getKey().equals(timerKey))
         {
            session.deleteScheduledTask(task.getId());
         }
      }
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
      String testScriptName = findOrCreateScript(session, scriptName, source);
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
   
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|               |               |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction1() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));// make sure that PS doesn't contain an entry with the given key

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      sleep2000();
      
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));//make sure that the script has executed, and the entry has appeared in the PS 
      
      //to run next tests
      session.deletePersistentStorageValue("Key to set A");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      
      session.closeEventProcessingPolicy();
   }
   
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|               |               |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   |
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   // Action inactive
   @Test
   public void testCreateServerActionInactive() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration action = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      action.setActive(false);
      session.saveEventProcessingPolicy(policy);

      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));// make sure that PS doesn't contain an entry with the given key

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      sleep2000();
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));//make sure that the script has not been executed, and the entry in not in PS 
      
      //to run next tests
      session.deletePersistentStorageValue("Key to set A");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      
      session.closeEventProcessingPolicy();
   }

   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|      6        |   "timer key" |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | x 2
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction2() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(TIME_4_SEK);
      actionA.setTimerKey(TIMER_KEY);
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();
      
      assertEquals(countScheduledTasks(session, TIMER_KEY), 2);// make sure that schedule tasks has 2 entries with this key
            
      sleep3000();
      
      // make sure that PS has 2 entries (this script adds '2' to the key if there is already an entry with that key)
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      
      //to run next tests
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set A2"); 
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      
      session.closeEventProcessingPolicy();
   }
   
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|      6        |   "timer key" |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | x 2
   //|      6        |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction3() throws Exception 
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(TIME_4_SEK);
      actionA.setTimerKey(TIMER_KEY);
      actionA.setSnoozeTime(TIME_4_SEK);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();
      
      assertEquals(countScheduledTasks(session, TIMER_KEY), 2);// make sure that schedule tasks has 2 entries with this key
         
      sleep3000();
      
      // make sure that PS has 2 entries (this script adds '2' to the key if there is already an entry with that key)
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      
      //to run next tests
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set A2");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      
      session.closeEventProcessingPolicy();
   }

   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|      6        |   "timer key" |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | x 2
   //|               |   "timer key" |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction4() throws Exception
   {
      
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(TIME_4_SEK);
      actionA.setTimerKey(TIMER_KEY);
      actionA.setBlockingTimerKey(TIMER_KEY);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null); 
      
      sleep2000();
      
      assertEquals(countScheduledTasks(session, TIMER_KEY), 1);// make sure that schedule tasks has only 1 entry with this key (because of blocking key)
         
      sleep3000();
      
      // make sure that PS has only 1 entry (because of blocking key)
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      
      //to run next tests
      session.deletePersistentStorageValue("Key to set A");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();

   }
   
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|               |               |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | x 2
   //|      6        |   "timer key" |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction5() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setSnoozeTime(TIME_4_SEK);
      actionA.setBlockingTimerKey(TIMER_KEY);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);
      
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null); 
      
      sleep2000();

      assertEquals(countScheduledTasks(session, TIMER_KEY), 1);// make sure that schedule tasks has only 1 entry with this key (because of blocking key)
      
      sleep3000();
      
      // make sure that PS has only 1 entry with this key (because of blocking key)
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A2"));
      
      //to run next tests
      session.deletePersistentStorageValue("Key to set A");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      session.closeEventProcessingPolicy();

   }
   
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|      6        |   "timer key" |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|      6        |   "timer key" |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|           "timer key"         |
   //+-------------------------------+
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
      actionA.setTimerKey(TIMER_KEY);
      actionA.setSnoozeTime(TIME_4_SEK);
      actionA.setBlockingTimerKey(TIMER_KEY);
      List<String> cancelationKeyList = new ArrayList<String>();
      cancelationKeyList.add(TIMER_KEY);
      testRuleA.setTimerCancellations(cancelationKeyList);
      
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);// make sure that won't be any entry because it cancels itself.
        
      sleep3000();
      
      // make sure that PS doesn't have entry with this key (because of cancellation key)
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      
      session.closeEventProcessingPolicy();
   }
   
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|      "TEST"   |  "timer key"  |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction7() throws Exception
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay("TEST");
      actionA.setTimerKey(TIMER_KEY);
           
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      // make sure that there is no entry in the schedule, and an entry in PS appears immediately, because delay key is not a number
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0); 
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      //to run next test
      session.deletePersistentStorageValue("Key to set A"); 
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();
   }
   
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|      " "      |  "timer key"  |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction8() throws Exception
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(" ");
      actionA.setTimerKey(TIMER_KEY);
           
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      // make sure that there is no entry in the schedule, and an entry in PS appears immediately, because delay key is not a number
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      //to run next test
      session.deletePersistentStorageValue("Key to set A"); 
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();
   }
   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|"!@#$%^&*()_+" |  "timer key"  |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction9() throws Exception
   {     
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay("\"!@#$%^&*()_+\"");
      actionA.setTimerKey(TIMER_KEY);
           
      session.saveEventProcessingPolicy(policy);
      
      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);
      
      sleep2000();

      //make sure that there is no entry in the schedule, and an entry in PS appears immediately, because delay key is not a number
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      //to run next test
      session.deletePersistentStorageValue("Key to set A"); 
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();
   }

   // +------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|   "АБВГД"     |  "timer key"  |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction10() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay("АБВГД");
      actionA.setTimerKey(TIMER_KEY);

      session.saveEventProcessingPolicy(policy);

      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      // make sure that there is no entry in the schedule, and an entry in PS appears immediately, because delay key is not a number
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      // to run next test
      session.deletePersistentStorageValue("Key to set A");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();
   }
   //+------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|   "-1"        |  "timer key"  |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction11() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay("-1");
      actionA.setTimerKey(TIMER_KEY);

      session.saveEventProcessingPolicy(policy);

      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      // make sure that there is no entry in the schedule, and an entry in PS appears immediately, because delay key is not a
      // positive number
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      // to run next test
      session.deletePersistentStorageValue("Key to set A");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();
   }

   //+------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|   "0"         |  "timer key"  |
   //|_______________|_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction12() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay("0");
      actionA.setTimerKey(TIMER_KEY);

      session.saveEventProcessingPolicy(policy);

      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      // make sure that there is no entry in the schedule, and an entry in PS appears immediately, because delay key is 0
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      // to run next test
      session.deletePersistentStorageValue("Key to set A");
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      session.closeEventProcessingPolicy();
   }

   //+------------------------------+
   //|   DELAY TIME  |   DELAY KEY   |
   //|Long.MAX_VALUE |  "timer key"  |
   //|_- 2000000000_ |_______________|
   //| SNOOZE TIME   |BLOCKING KEY   | 
   //|               |               |
   //|_______________|_______________|
   //|        CANCELATION KEY        |
   //|                               |
   //+-------------------------------+
   @Test
   public void testCreateServerAction13() throws Exception
   {
      // waiting for information on what the maximum allowable input number should be
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRuleA = createTestRule(session, node, policy, TEMPLATE_NAME_A, COMMENT_FOR_SEARCHING_RULE_A);
      ActionExecutionConfiguration actionA = createActionExecution(session, testRuleA, SCRIPT_NAME_FOR_SEARCHING_A, SOURCE_FOR_RULE_A, ACTION_NAME_A);
      actionA.setTimerDelay(Long.toString(Long.MAX_VALUE - 2000000000));
      actionA.setTimerKey(TIMER_KEY);

      session.saveEventProcessingPolicy(policy);

      // make sure that PS and schedule task doesn't contain an entry with the given key
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.sendEvent(0, TEMPLATE_NAME_A, node.getObjectId(), new String[] {}, null, null, null);

      sleep2000();

      // make sure that there is entry in the schedule
      assertEquals(countScheduledTasks(session, TIMER_KEY), 1);
      // There is no entry in the PS because the timestamp is too large, and not enough time has passed for the entry to appear
      assertNull(TestHelperForEpp.findPsValueByKey(session, "Key to set A"));

      // to run next test
      deleteScheduledTasks(session, TIMER_KEY);
      assertEquals(countScheduledTasks(session, TIMER_KEY), 0);

      session.closeEventProcessingPolicy();
   }
   
   /**
    * Cleanup before tests
    * @throws Exception
    */
   @BeforeEach
   void cleanupBeforeTest() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.deletePersistentStorageValue("Key to set A");
      session.deletePersistentStorageValue("Key to set A2");      
   }
}
