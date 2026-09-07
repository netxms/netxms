/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.UUID;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ServerActionType;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.client.events.AlarmCategory;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

/**
 * Tests for rule configuration error detection in event processing policy (EPP). Server calculates error flags each time
 * policy is sent to client, so every check reloads the policy after saving and looks at the flags of the freshly loaded rule.
 *
 * Server refuses to delete an action or alarm category that is still referenced by a rule, and a deleted object stays in
 * the object index until the next object synchronization cycle, so these references are broken by pointing the rule at an
 * identifier that never existed. Event templates can be deleted while referenced, so that case uses a real deletion.
 */
public class EppRuleErrorsTest extends AbstractSessionTest
{
   private static final String EVENT_NAME = "Event for EPP rule errors test";
   private static final String DELETED_EVENT_NAME = "Event to be deleted for EPP rule errors test";
   private static final String ACTION_NAME = "Action for EPP rule errors test";
   private static final String ALARM_CATEGORY_NAME = "Alarm category for EPP rule errors test";
   private static final String RULE_COMMENT = "Rule for testing EPP rule error detection";

   // Identifiers that do not exist on server
   private static final long BOGUS_OBJECT_ID = 0x7FFFFFF0L;
   private static final int BOGUS_EVENT_CODE = 0x7FFFFFF0;
   private static final long BOGUS_ACTION_ID = 0x7FFFFFF0L;
   private static final long BOGUS_ALARM_CATEGORY_ID = 0x7FFFFFF0L;

   private static final String VALID_SCRIPT = "return true;";
   private static final String INVALID_SCRIPT = "return (1 +;";

   private NXCSession session;
   private EventProcessingPolicyChain policy;
   private EventProcessingPolicyRule rule;
   private UUID ruleGuid;
   private long createdActionId = 0;
   private long createdAlarmCategoryId = 0;
   private int createdEventCode = 0;

   /**
    * Create a valid rule referencing the management server node and a dedicated event template.
    */
   @BeforeEach
   void createTestRule() throws Exception
   {
      session = connectAndLogin();
      session.syncObjects();

      Node node = TestHelper.findManagementServer(session);
      EventTemplate eventTemplate = TestHelperForEpp.findOrCreateEvent(session, EVENT_NAME);

      policy = session.getEventProcessingPolicyChain(0);
      rule = TestHelperForEpp.findOrCreateRule(session, policy, RULE_COMMENT, eventTemplate, node);
      // Reset every reference in case a previous failed run left the rule behind
      rule.setSources(Collections.singletonList(node.getObjectId()));
      rule.setSourceExclusions(new ArrayList<Long>());
      rule.setEvents(Collections.singletonList(eventTemplate.getCode()));
      rule.setActions(new ArrayList<ActionExecutionConfiguration>());
      rule.setAlarmCategories(new ArrayList<Long>());
      rule.setFilterScript("");
      rule.setActionScript("");
      session.saveEventProcessingPolicy(policy);
      ruleGuid = rule.getGuid();

      rule = reloadRule();
      assertEquals(0, rule.getErrors(), "freshly created rule with valid references must not report errors");
      assertFalse(rule.hasErrors());
   }

   /**
    * Remove test rule and any server-side entities created by the test.
    */
   @AfterEach
   void deleteTestRule() throws Exception
   {
      if (session == null)
         return;

      // Drop references from the rule before deleting referenced entities, then drop the rule itself
      EventProcessingPolicyChain currentPolicy = session.getEventProcessingPolicyChain(0);
      EventProcessingPolicyRule currentRule = findRule(currentPolicy);
      if (currentRule != null)
      {
         currentPolicy.deleteRule(currentRule);
         session.saveEventProcessingPolicy(currentPolicy);
      }

      if (createdActionId != 0)
         session.deleteAction(createdActionId);
      if (createdAlarmCategoryId != 0)
         session.deleteAlarmCategory(createdAlarmCategoryId);
      if (createdEventCode != 0)
         session.deleteEventTemplate(createdEventCode);
   }

   /**
    * Find test rule in given policy by GUID.
    */
   private EventProcessingPolicyRule findRule(EventProcessingPolicyChain p)
   {
      for(EventProcessingPolicyRule r : p.getRules())
      {
         if (r.getGuid().equals(ruleGuid))
            return r;
      }
      return null;
   }

   /**
    * Reload policy from server and return test rule from it. Error flags are only calculated by server when policy is sent,
    * so this is the only way to observe them.
    */
   private EventProcessingPolicyRule reloadRule() throws Exception
   {
      policy = session.getEventProcessingPolicyChain(0);
      EventProcessingPolicyRule r = findRule(policy);
      assertNotNull(r, "test rule not found in reloaded policy");
      return r;
   }

   /**
    * Save current policy, reload it, and return test rule with fresh error flags.
    */
   private EventProcessingPolicyRule saveAndReload() throws Exception
   {
      session.saveEventProcessingPolicy(policy);
      return reloadRule();
   }

   @Test
   public void testMissingSourceObject() throws Exception
   {
      rule.setSources(Collections.singletonList(BOGUS_OBJECT_ID));
      rule = saveAndReload();
      assertEquals(EventProcessingPolicyRule.ERROR_MISSING_SOURCE_OBJECT, rule.getErrors());
      assertTrue(rule.hasErrors());

      // A missing exclusion is reported under the same flag
      Node node = TestHelper.findManagementServer(session);
      rule.setSources(Collections.singletonList(node.getObjectId()));
      rule.setSourceExclusions(Collections.singletonList(BOGUS_OBJECT_ID));
      rule = saveAndReload();
      assertEquals(EventProcessingPolicyRule.ERROR_MISSING_SOURCE_OBJECT, rule.getErrors());

      // Both lists valid again - error must clear
      rule.setSourceExclusions(new ArrayList<Long>());
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());
   }

   @Test
   public void testMissingEvent() throws Exception
   {
      // Reference to an event that never existed
      rule.setEvents(Collections.singletonList(BOGUS_EVENT_CODE));
      rule = saveAndReload();
      assertEquals(EventProcessingPolicyRule.ERROR_MISSING_EVENT, rule.getErrors());

      // Reference to a real event that is deleted afterwards
      EventTemplate deletedEvent = TestHelperForEpp.findOrCreateEvent(session, DELETED_EVENT_NAME);
      createdEventCode = deletedEvent.getCode();
      rule.setEvents(Collections.singletonList(deletedEvent.getCode()));
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());

      session.deleteEventTemplate(deletedEvent.getCode());
      createdEventCode = 0;
      rule = reloadRule();
      assertEquals(EventProcessingPolicyRule.ERROR_MISSING_EVENT, rule.getErrors());

      // Restore valid reference - error must clear
      EventTemplate validEvent = TestHelperForEpp.findOrCreateEvent(session, EVENT_NAME);
      rule.setEvents(Collections.singletonList(validEvent.getCode()));
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());
   }

   @Test
   public void testMissingAction() throws Exception
   {
      // Real action is accepted
      createdActionId = session.createAction(ACTION_NAME);
      ServerAction action = new ServerAction(createdActionId);
      action.setName(ACTION_NAME);
      action.setType(ServerActionType.NXSL_SCRIPT);
      action.setData("");
      session.modifyAction(action);

      rule.setActions(Collections.singletonList(new ActionExecutionConfiguration(createdActionId, null, null, null, null)));
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());

      // One missing action among valid ones is enough to raise the flag
      rule.setActions(Arrays.asList(new ActionExecutionConfiguration(createdActionId, null, null, null, null),
            new ActionExecutionConfiguration(BOGUS_ACTION_ID, null, null, null, null)));
      rule = saveAndReload();
      assertEquals(EventProcessingPolicyRule.ERROR_MISSING_ACTION, rule.getErrors());

      rule.setActions(new ArrayList<ActionExecutionConfiguration>());
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());
   }

   @Test
   public void testMissingAlarmCategory() throws Exception
   {
      // Real alarm category is accepted
      AlarmCategory category = new AlarmCategory();
      category.setName(ALARM_CATEGORY_NAME);
      category.setDescription("Created by EppRuleErrorsTest");
      createdAlarmCategoryId = session.modifyAlarmCategory(category);
      assertTrue(createdAlarmCategoryId > 0);

      rule.setAlarmCategories(Collections.singletonList(createdAlarmCategoryId));
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());

      rule.setAlarmCategories(Arrays.asList(createdAlarmCategoryId, BOGUS_ALARM_CATEGORY_ID));
      rule = saveAndReload();
      assertEquals(EventProcessingPolicyRule.ERROR_MISSING_ALARM_CATEGORY, rule.getErrors());

      rule.setAlarmCategories(new ArrayList<Long>());
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());
   }

   @Test
   public void testFilterScriptCompilationError() throws Exception
   {
      rule.setFilterScript(VALID_SCRIPT);
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());

      rule.setFilterScript(INVALID_SCRIPT);
      rule = saveAndReload();
      assertEquals(EventProcessingPolicyRule.ERROR_FILTER_SCRIPT, rule.getErrors());

      rule.setFilterScript("");
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());
   }

   @Test
   public void testActionScriptCompilationError() throws Exception
   {
      rule.setActionScript(VALID_SCRIPT);
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());

      rule.setActionScript(INVALID_SCRIPT);
      rule = saveAndReload();
      assertEquals(EventProcessingPolicyRule.ERROR_ACTION_SCRIPT, rule.getErrors());

      rule.setActionScript("");
      rule = saveAndReload();
      assertEquals(0, rule.getErrors());
   }

   @Test
   public void testMultipleErrorsCombined() throws Exception
   {
      rule.setSources(Collections.singletonList(BOGUS_OBJECT_ID));
      rule.setEvents(Collections.singletonList(BOGUS_EVENT_CODE));
      rule.setActions(Collections.singletonList(new ActionExecutionConfiguration(BOGUS_ACTION_ID, null, null, null, null)));
      rule.setAlarmCategories(Collections.singletonList(BOGUS_ALARM_CATEGORY_ID));
      rule.setFilterScript(INVALID_SCRIPT);
      rule.setActionScript(INVALID_SCRIPT);
      rule = saveAndReload();

      int expected = EventProcessingPolicyRule.ERROR_MISSING_SOURCE_OBJECT | EventProcessingPolicyRule.ERROR_MISSING_EVENT |
            EventProcessingPolicyRule.ERROR_MISSING_ACTION | EventProcessingPolicyRule.ERROR_MISSING_ALARM_CATEGORY |
            EventProcessingPolicyRule.ERROR_FILTER_SCRIPT | EventProcessingPolicyRule.ERROR_ACTION_SCRIPT;
      assertEquals(expected, rule.getErrors());
      assertTrue(rule.hasErrors());

      // Errors are carried over into a copy of the rule
      EventProcessingPolicyRule copy = new EventProcessingPolicyRule(rule);
      assertEquals(expected, copy.getErrors());

      // New rule starts without errors
      assertEquals(0, new EventProcessingPolicyRule().getErrors());
   }
}
