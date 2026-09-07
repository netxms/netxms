/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.netxms.utilities.TestHelper.assertRcc;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EPPChainCaller;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

/**
 * Tests for event processing policy rule chains: traversal, stop inside a chain, diamond reuse, loop detection, and chain
 * management. Each test builds its chains from scratch, so tests are independent of each other.
 */
public class EppChainTest extends AbstractSessionTest
{
   /**
    * Build action script appending given tag to the trace kept in persistent storage
    */
   private static String traceScript(String traceKey, String tag)
   {
      return "trace = ReadPersistentStorage(\"" + traceKey + "\");\n" +
             "if (trace == null)\n" +
             "   trace = \"\";\n" +
             "trace = trace .. \"" + tag + ";\";\n" +
             "WritePersistentStorage(\"" + traceKey + "\", trace);";
   }

   /**
    * Create chain with one rule matching given event and appending given tag to the trace
    */
   private static EventProcessingPolicyChain createChainWithRule(NXCSession session, String chainName, EventTemplate eventTemplate,
         String traceKey, String tag, int extraFlags) throws Exception
   {
      EventProcessingPolicyChain chain = session.createEppChain(chainName, "EppChainTest chain", null);
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      rule.setChainId(chain.getId());
      rule.setComments("EppChainTest rule in " + chainName);
      rule.setEvents(Arrays.asList(eventTemplate.getCode()));
      rule.setActionScript(traceScript(traceKey, tag));
      rule.setFlags(rule.getFlags() | extraFlags);
      chain.addRule(rule);
      return chain;
   }

   /**
    * Find or create main chain rule matching given event, appending given tag to the trace and calling given chains
    */
   private static EventProcessingPolicyRule setupMainRule(NXCSession session, EventProcessingPolicyChain mainChain, String comment,
         EventTemplate eventTemplate, Node node, String traceKey, String tag, List<EventProcessingPolicyChain> chainCalls) throws Exception
   {
      EventProcessingPolicyRule rule = TestHelperForEpp.findOrCreateRule(session, mainChain, comment, eventTemplate, node);
      rule.setEvents(Arrays.asList(eventTemplate.getCode()));
      rule.setActionScript(traceScript(traceKey, tag));
      List<Integer> calledChainIds = new ArrayList<>();
      for(EventProcessingPolicyChain chain : chainCalls)
         calledChainIds.add(chain.getId());
      rule.setChainCalls(calledChainIds);
      return rule;
   }

   /**
    * Get trace from persistent storage
    */
   private static String getTrace(NXCSession session, String traceKey) throws Exception
   {
      return TestHelperForEpp.findPsValueByKey(session, traceKey);
   }

   /**
    * Save given chains (each chain is saved by a separate request)
    */
   private static void saveChains(NXCSession session, EventProcessingPolicyChain... chains) throws Exception
   {
      for(EventProcessingPolicyChain chain : chains)
         assertTrue(session.saveEventProcessingPolicy(chain).isSuccess());
   }

   /**
    * Wait until a session notification with given code and subcode arrives (notifications are delivered asynchronously)
    */
   private static void waitForNotification(List<SessionNotification> notifications, int code, long subCode) throws Exception
   {
      for(int i = 0; i < 50; i++)
      {
         synchronized(notifications)
         {
            for(SessionNotification n : notifications)
               if ((n.getCode() == code) && (n.getSubCode() == subCode))
                  return;
         }
         Thread.sleep(100);
      }
      throw new AssertionError("Notification " + code + " with subcode " + subCode + " not received");
   }

   /**
    * A rule calling a chain: the chain's rules run after the caller's own actions, then processing returns to the rule after
    * the caller.
    */
   @Test
   public void testTraversalAndReturn() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      final String traceKey = "epp-chain-trace-traversal";
      Node node = TestHelper.findManagementServer(session);
      EventTemplate event = TestHelperForEpp.findOrCreateEvent(session, "TestEppChainTraversal");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Traversal-A");

      EventProcessingPolicyChain mainChain = session.getEventProcessingPolicyChain(0);
      EventProcessingPolicyChain chainA = createChainWithRule(session, "EppChainTest-Traversal-A", event, traceKey, "A", 0);
      setupMainRule(session, mainChain, "EppChainTest traversal caller", event, node, traceKey, "M1", Arrays.asList(chainA));
      EventProcessingPolicyRule afterRule = TestHelperForEpp.findOrCreateRule(session, mainChain, "EppChainTest traversal after", event, node);
      afterRule.setEvents(Arrays.asList(event.getCode()));
      afterRule.setActionScript(traceScript(traceKey, "M2"));
      saveChains(session, mainChain, chainA);

      session.setPersistentStorageValue(traceKey, "");
      session.sendEvent(0, event.getName(), node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(2000);
      assertEquals("M1;A;M2;", getTrace(session, traceKey));
   }

   /**
    * Stop processing inside a called chain halts processing of the event entirely
    */
   @Test
   public void testStopInSubChain() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      final String traceKey = "epp-chain-trace-stop";
      Node node = TestHelper.findManagementServer(session);
      EventTemplate event = TestHelperForEpp.findOrCreateEvent(session, "TestEppChainStop");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Stop-A");

      EventProcessingPolicyChain mainChain = session.getEventProcessingPolicyChain(0);
      EventProcessingPolicyChain chainA = createChainWithRule(session, "EppChainTest-Stop-A", event, traceKey, "A",
            EventProcessingPolicyRule.STOP_PROCESSING);
      setupMainRule(session, mainChain, "EppChainTest stop caller", event, node, traceKey, "M1", Arrays.asList(chainA));
      EventProcessingPolicyRule afterRule = TestHelperForEpp.findOrCreateRule(session, mainChain, "EppChainTest stop after", event, node);
      afterRule.setEvents(Arrays.asList(event.getCode()));
      afterRule.setActionScript(traceScript(traceKey, "M2"));
      saveChains(session, mainChain, chainA);

      session.setPersistentStorageValue(traceKey, "");
      session.sendEvent(0, event.getName(), node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(2000);
      assertEquals("M1;A;", getTrace(session, traceKey));
   }

   /**
    * A chain called from two other chains runs each time (a chain is skipped only while it is on the active call stack)
    */
   @Test
   public void testDiamondReuse() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      final String traceKey = "epp-chain-trace-diamond";
      Node node = TestHelper.findManagementServer(session);
      EventTemplate event = TestHelperForEpp.findOrCreateEvent(session, "TestEppChainDiamond");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Diamond-A");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Diamond-B");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Diamond-C");

      EventProcessingPolicyChain mainChain = session.getEventProcessingPolicyChain(0);
      EventProcessingPolicyChain chainC = createChainWithRule(session, "EppChainTest-Diamond-C", event, traceKey, "C", 0);
      EventProcessingPolicyChain chainA = createChainWithRule(session, "EppChainTest-Diamond-A", event, traceKey, "A", 0);
      EventProcessingPolicyChain chainB = createChainWithRule(session, "EppChainTest-Diamond-B", event, traceKey, "B", 0);
      // A and B each call C
      chainA.getRules().get(0).setChainCalls(Arrays.asList(chainC.getId()));
      chainB.getRules().get(0).setChainCalls(Arrays.asList(chainC.getId()));
      setupMainRule(session, mainChain, "EppChainTest diamond caller", event, node, traceKey, "M", Arrays.asList(chainA, chainB));
      saveChains(session, mainChain, chainA, chainB, chainC);

      session.setPersistentStorageValue(traceKey, "");
      session.sendEvent(0, event.getName(), node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(2000);
      assertEquals("M;A;C;B;C;", getTrace(session, traceKey));
   }

   /**
    * A call to a chain already on the call stack is skipped and reported once by SYS_EPP_CHAIN_LOOP
    */
   @Test
   public void testLoopDetection() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      final String traceKey = "epp-chain-trace-loop";
      Node node = TestHelper.findManagementServer(session);
      EventTemplate event = TestHelperForEpp.findOrCreateEvent(session, "TestEppChainLoop");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Loop-A");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Loop-B");

      EventProcessingPolicyChain mainChain = session.getEventProcessingPolicyChain(0);
      EventProcessingPolicyChain chainA = createChainWithRule(session, "EppChainTest-Loop-A", event, traceKey, "A", 0);
      EventProcessingPolicyChain chainB = createChainWithRule(session, "EppChainTest-Loop-B", event, traceKey, "B", 0);
      // A calls B, B calls A - a cycle
      chainA.getRules().get(0).setChainCalls(Arrays.asList(chainB.getId()));
      chainB.getRules().get(0).setChainCalls(Arrays.asList(chainA.getId()));
      setupMainRule(session, mainChain, "EppChainTest loop caller", event, node, traceKey, "M", Arrays.asList(chainA));

      // Alarm on SYS_EPP_CHAIN_LOOP to observe the loop event; fresh key each run so repeat count starts at 1
      EventTemplate loopEvent = TestHelperForEpp.findOrCreateEvent(session, "SYS_EPP_CHAIN_LOOP");
      final String alarmKey = "EppChainTest-loop-alarm-" + new Date().getTime();
      EventProcessingPolicyRule loopRule = TestHelperForEpp.findOrCreateRule(session, mainChain, "EppChainTest loop alarm", loopEvent, node);
      loopRule.setEvents(Arrays.asList(loopEvent.getCode()));
      loopRule.setAlarmKey(alarmKey);
      loopRule.setAlarmMessage("EPP chain loop: %1 -> %2");
      loopRule.setFlags(loopRule.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM);
      saveChains(session, mainChain, chainA, chainB);

      session.setPersistentStorageValue(traceKey, "");
      session.sendEvent(0, event.getName(), node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(3000);   // Loop event is posted asynchronously and processed separately

      // B's call back to A was skipped
      assertEquals("M;A;B;", getTrace(session, traceKey));
      Alarm loopAlarm = null;
      for(Alarm a : session.getAlarms().values())
         if (a.getKey().equals(alarmKey))
            loopAlarm = a;
      assertNotNull(loopAlarm);
      assertEquals(1, loopAlarm.getRepeatCount());

      // Second event within the report interval: the call is skipped again but the loop is not reported again
      session.sendEvent(0, event.getName(), node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(3000);
      assertEquals("M;A;B;M;A;B;", getTrace(session, traceKey));
      loopAlarm = session.getAlarms().get(loopAlarm.getId());
      assertNotNull(loopAlarm);
      assertEquals(1, loopAlarm.getRepeatCount());
      session.terminateAlarm(loopAlarm.getId());
   }

   /**
    * Chain registry: create, load, callers, rename, delete with call removal; every change is announced to sessions
    */
   @Test
   public void testChainManagement() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      TestHelperForEpp.deleteChains(session, "EppChainTest-Mgmt");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Mgmt-Renamed");

      // Every chain registry and rule change is announced to all sessions
      final List<SessionNotification> notifications = Collections.synchronizedList(new ArrayList<SessionNotification>());
      SessionListener listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.EPP_RULES_CHANGED) || (n.getCode() == SessionNotification.EPP_CHAIN_UPDATED) || (n.getCode() == SessionNotification.EPP_CHAIN_DELETED))
               notifications.add(n);
         }
      };
      session.addListener(listener);

      EventProcessingPolicyChain created = session.createEppChain("EppChainTest-Mgmt", "management test chain", null);
      assertTrue(created.getId() > 0);
      assertNotNull(created.getGuid());
      waitForNotification(notifications, SessionNotification.EPP_CHAIN_UPDATED, created.getId());

      // Registry lists the main chain and the new chain without rules
      EventProcessingPolicy policy = session.getEventProcessingPolicyChains();
      assertNotNull(policy.getMainChain());
      assertTrue(policy.getMainChain().isMain());
      EventProcessingPolicyChain loaded = policy.findChain(created.getId());
      assertNotNull(loaded);
      assertEquals("EppChainTest-Mgmt", loaded.getName());
      assertEquals("management test chain", loaded.getDescription());
      assertEquals(created.getGuid(), loaded.getGuid());
      assertTrue(loaded.isEditable());
      assertTrue(!loaded.isLoaded());
      assertEquals(0, loaded.getRuleCount());
      assertEquals(0, loaded.getCallerCount());

      // Main chain rule calling the chain is reported as its caller
      Node node = TestHelper.findManagementServer(session);
      EventTemplate event = TestHelperForEpp.findOrCreateEvent(session, "TestEppChainMgmt");
      EventProcessingPolicyChain mainChain = session.getEventProcessingPolicyChain(0);
      assertTrue(mainChain.isLoaded());
      EventProcessingPolicyRule caller = TestHelperForEpp.findOrCreateRule(session, mainChain, "EppChainTest management caller", event, node);
      caller.setChainCalls(Arrays.asList(created.getId()));
      assertTrue(session.saveEventProcessingPolicy(mainChain).isSuccess());
      waitForNotification(notifications, SessionNotification.EPP_RULES_CHANGED, 0);

      List<EPPChainCaller> callers = session.getEppChainCallers(created.getId());
      assertEquals(1, callers.size());
      assertEquals(caller.getGuid(), callers.get(0).getRuleGuid());
      assertEquals(0, callers.get(0).getChainId());
      assertEquals(1, session.getEventProcessingPolicyChain(created.getId()).getCallerCount());

      notifications.clear();
      session.modifyEppChain(created.getId(), "EppChainTest-Mgmt-Renamed", "renamed", null);
      waitForNotification(notifications, SessionNotification.EPP_CHAIN_UPDATED, created.getId());
      policy = session.getEventProcessingPolicyChains();
      loaded = policy.findChain(created.getId());
      assertNotNull(loaded);
      assertEquals("EppChainTest-Mgmt-Renamed", loaded.getName());

      // Deleting the chain removes the call from the main chain rule and gives the main chain a new version
      notifications.clear();
      Map<Integer, Integer> updatedChains = session.deleteEppChain(created.getId());
      assertTrue(updatedChains.containsKey(0));
      waitForNotification(notifications, SessionNotification.EPP_CHAIN_DELETED, created.getId());
      waitForNotification(notifications, SessionNotification.EPP_RULES_CHANGED, 0);
      policy = session.getEventProcessingPolicyChains();
      assertNull(policy.findChain(created.getId()));
      mainChain = session.getEventProcessingPolicyChain(0);
      assertEquals(updatedChains.get(0).intValue(), mainChain.getVersion());
      EventProcessingPolicyRule reloadedCaller = null;
      for(EventProcessingPolicyRule rule : mainChain.getRules())
         if (rule.getGuid().equals(caller.getGuid()))
            reloadedCaller = rule;
      assertNotNull(reloadedCaller);
      assertTrue(reloadedCaller.getChainCalls().isEmpty());
      session.removeListener(listener);
   }

   /**
    * Full policy load returns every chain with rules; deleting a chain called from several chains removes the
    * call from each of them and reports every chain that got a new version.
    */
   @Test
   public void testFullLoadAndMultiCallerDelete() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      TestHelperForEpp.deleteChains(session, "EppChainTest-Target");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Caller1");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Caller2");

      EventTemplate event = TestHelperForEpp.findOrCreateEvent(session, "TestEppChainMultiCaller");
      EventProcessingPolicyChain target = createChainWithRule(session, "EppChainTest-Target", event, "EppChainTest-multi", "T", 0);
      EventProcessingPolicyChain caller1 = createChainWithRule(session, "EppChainTest-Caller1", event, "EppChainTest-multi", "C1", 0);
      EventProcessingPolicyChain caller2 = createChainWithRule(session, "EppChainTest-Caller2", event, "EppChainTest-multi", "C2", 0);
      caller1.getRules().get(0).setChainCalls(Arrays.asList(target.getId()));
      caller2.getRules().get(0).setChainCalls(Arrays.asList(target.getId()));
      saveChains(session, target, caller1, caller2);

      EventProcessingPolicy policy = session.getEventProcessingPolicy();
      assertTrue(policy.getMainChain().isLoaded());
      for(EventProcessingPolicyChain chain : policy.getChains())
         assertTrue(chain.isLoaded());
      EventProcessingPolicyChain loadedTarget = policy.findChain(target.getGuid());
      assertEquals(target.getId(), loadedTarget.getId());
      assertTrue(loadedTarget.isLoaded());
      assertEquals(1, loadedTarget.getRules().size());
      assertEquals(2, loadedTarget.getCallerCount());
      assertEquals(Arrays.asList(target.getId()), policy.findChain(caller1.getId()).getRules().get(0).getChainCalls());

      List<EPPChainCaller> callers = session.getEppChainCallers(target.getId());
      assertEquals(2, callers.size());

      Map<Integer, Integer> updatedChains = session.deleteEppChain(target.getId());
      assertEquals(2, updatedChains.size());
      assertTrue(updatedChains.containsKey(caller1.getId()));
      assertTrue(updatedChains.containsKey(caller2.getId()));
      for(EventProcessingPolicyChain caller : Arrays.asList(caller1, caller2))
      {
         EventProcessingPolicyChain reloaded = session.getEventProcessingPolicyChain(caller.getId());
         assertEquals(updatedChains.get(caller.getId()).intValue(), reloaded.getVersion());
         assertTrue(reloaded.getRules().get(0).getChainCalls().isEmpty());
      }

      TestHelperForEpp.deleteChains(session, "EppChainTest-Caller1");
      TestHelperForEpp.deleteChains(session, "EppChainTest-Caller2");
   }

   /**
    * Chain management validation: the main chain cannot be renamed or deleted, a chain needs a name, chain names
    * need not be unique, rules cannot be saved into an unknown chain or call an unknown chain.
    */
   @Test
   public void testChainManagementValidation() throws Exception
   {
      final NXCSession session = connectAndLogin();
      TestHelperForEpp.deleteChains(session, "EppChainTest-Validation");

      assertRcc(RCC.INVALID_ARGUMENT, () -> session.createEppChain("", "no name", null));
      assertRcc(RCC.INVALID_ARGUMENT, () -> session.modifyEppChain(0, "Renamed main", "", null));
      assertRcc(RCC.INVALID_ARGUMENT, () -> session.deleteEppChain(0));
      assertRcc(RCC.INVALID_ARGUMENT, () -> session.modifyEppChain(0x7FFFFFF0, "Unknown", "", null));
      assertRcc(RCC.INVALID_ARGUMENT, () -> session.deleteEppChain(0x7FFFFFF0));

      // Names are not unique: two chains with the same name are distinct by ID and GUID
      EventProcessingPolicyChain first = session.createEppChain("EppChainTest-Validation", "first", null);
      EventProcessingPolicyChain second = session.createEppChain("EppChainTest-Validation", "second", null);
      assertTrue(first.getId() != second.getId());
      assertTrue(!first.getGuid().equals(second.getGuid()));
      EventProcessingPolicy policy = session.getEventProcessingPolicyChains();
      assertEquals("first", policy.findChain(first.getId()).getDescription());
      assertEquals("second", policy.findChain(second.getId()).getDescription());

      // Rules cannot be saved into a chain that does not exist
      EventProcessingPolicyChain ghost = new EventProcessingPolicyChain(0x7FFFFFF0, UUID.randomUUID(), "ghost", "");
      ghost.setRules(new ArrayList<EventProcessingPolicyRule>(), 0);
      assertRcc(RCC.INVALID_ARGUMENT, () -> session.saveEventProcessingPolicy(ghost));

      // A rule calling a chain that no longer exists is rejected and nothing is saved
      EventProcessingPolicyChain chain = session.getEventProcessingPolicyChain(first.getId());
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      rule.setChainId(first.getId());
      rule.setComments("EppChainTest rule calling deleted chain");
      rule.setChainCalls(Arrays.asList(second.getId()));
      chain.addRule(rule);
      session.deleteEppChain(second.getId());
      assertRcc(RCC.INVALID_ARGUMENT, () -> session.saveEventProcessingPolicy(chain));
      assertTrue(session.getEventProcessingPolicyChain(first.getId()).getRules().isEmpty());

      session.deleteEppChain(first.getId());
   }

   /**
    * Remove everything the test creates on the server, whether it passed or failed
    */
   @AfterEach
   void removeTestData() throws Exception
   {
      NXCSession session = connectAndLogin();
      TestHelperForEpp.deleteRules(session, "EppChainTest ");
      TestHelperForEpp.deleteChains(session, "EppChainTest-");
      TestHelperForEpp.deletePersistentStorageValues(session, "epp-chain-trace-");
      TestHelperForEpp.deletePersistentStorageValues(session, "EppChainTest-multi");
      TestHelperForEpp.terminateAlarms(session, "EppChainTest-loop-alarm-");
   }
}
