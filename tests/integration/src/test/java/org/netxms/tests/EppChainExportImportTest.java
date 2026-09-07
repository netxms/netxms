/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.UUID;
import org.apache.commons.io.FileUtils;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.utilities.TestHelperForEpp;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

/**
 * Configuration export and import of event processing policy rules that belong to or call sub-chains. Chains are
 * referenced by GUID in export files and mapped back to local chain IDs on import.
 */
public class EppChainExportImportTest extends AbstractSessionTest
{
   private static final String CHAIN_CALLED = "EppChainExportImportTest-Called";
   private static final String CHAIN_CALLER = "EppChainExportImportTest-Caller";
   private static final String RULE_CALLED = "EppChainExportImportTest rule in called chain";
   private static final String RULE_CALLER = "EppChainExportImportTest calling rule";

   private NXCSession session;
   private EventProcessingPolicyChain called;
   private EventProcessingPolicyChain caller;
   private UUID calledRuleGuid;
   private UUID callerRuleGuid;

   @BeforeEach
   void setup() throws Exception
   {
      session = connectAndLogin();
      removeTestObjects();

      called = session.createEppChain(CHAIN_CALLED, "called chain", null);
      EventProcessingPolicyRule calledRule = new EventProcessingPolicyRule();
      calledRule.setChainId(called.getId());
      calledRule.setComments(RULE_CALLED);
      called.addRule(calledRule);
      assertTrue(session.saveEventProcessingPolicy(called).isSuccess());
      calledRuleGuid = calledRule.getGuid();

      caller = session.createEppChain(CHAIN_CALLER, "caller chain", null);
      EventProcessingPolicyRule callerRule = new EventProcessingPolicyRule();
      callerRule.setChainId(caller.getId());
      callerRule.setComments(RULE_CALLER);
      callerRule.setChainCalls(Arrays.asList(called.getId()));
      caller.addRule(callerRule);
      assertTrue(session.saveEventProcessingPolicy(caller).isSuccess());
      callerRuleGuid = callerRule.getGuid();
   }

   @AfterEach
   void teardown() throws Exception
   {
      if (session != null)
         removeTestObjects();
   }

   /**
    * Remove test chains and any test rule that ended up in the main chain
    */
   private void removeTestObjects() throws Exception
   {
      TestHelperForEpp.deleteChains(session, "EppChainExportImportTest-");
      TestHelperForEpp.deleteRules(session, "EppChainExportImportTest");
   }

   /**
    * Export both test rules and parse the resulting file
    */
   private JsonObject export() throws Exception
   {
      File file = session.exportConfiguration("EppChainExportImportTest", new long[0], new long[0], new long[0],
            new UUID[] { callerRuleGuid, calledRuleGuid }, new long[0], new long[0], new long[0], new long[0], new long[0], new long[0],
            new String[0], new UUID[0], new UUID[0], new UUID[0], new long[0]);
      String content = FileUtils.readFileToString(file, StandardCharsets.UTF_8);
      file.delete();
      return JsonParser.parseString(content).getAsJsonObject();
   }

   private static JsonObject findByGuid(JsonArray array, UUID guid)
   {
      for(JsonElement e : array)
         if (e.getAsJsonObject().get("guid").getAsString().equals(guid.toString()))
            return e.getAsJsonObject();
      return null;
   }

   private static EventProcessingPolicyRule findRule(EventProcessingPolicyChain chain, UUID guid)
   {
      for(EventProcessingPolicyRule rule : chain.getRules())
         if (rule.getGuid().equals(guid))
            return rule;
      return null;
   }

   /**
    * Export records reference chains by GUID (owning chain in "chain", called chains in "chainCalls") and the file
    * carries a registry of every referenced chain.
    */
   @Test
   public void testExportFormat() throws Exception
   {
      JsonObject root = export();

      JsonArray rules = root.getAsJsonArray("rules");
      JsonObject callerJson = findByGuid(rules, callerRuleGuid);
      assertNotNull(callerJson);
      assertEquals(caller.getGuid().toString(), callerJson.get("chain").getAsString());
      JsonArray calls = callerJson.getAsJsonArray("chainCalls");
      assertEquals(1, calls.size());
      assertEquals(called.getGuid().toString(), calls.get(0).getAsString());
      assertFalse(callerJson.has("errors"));

      JsonObject calledJson = findByGuid(rules, calledRuleGuid);
      assertNotNull(calledJson);
      assertEquals(called.getGuid().toString(), calledJson.get("chain").getAsString());
      assertEquals(0, calledJson.getAsJsonArray("chainCalls").size());

      JsonArray chains = root.getAsJsonArray("chains");
      assertEquals(2, chains.size());
      JsonObject calledChain = findByGuid(chains, called.getGuid());
      assertNotNull(calledChain);
      assertEquals(CHAIN_CALLED, calledChain.get("name").getAsString());
      assertEquals("called chain", calledChain.get("description").getAsString());
      assertNotNull(findByGuid(chains, caller.getGuid()));

      // Main chain rules carry no "chain" field
      EventProcessingPolicyChain main = session.getEventProcessingPolicyChain(0);
      if (!main.getRules().isEmpty())
      {
         UUID mainRuleGuid = main.getRules().get(0).getGuid();
         File file = session.exportConfiguration("EppChainExportImportTest", new long[0], new long[0], new long[0],
               new UUID[] { mainRuleGuid }, new long[0], new long[0], new long[0], new long[0], new long[0], new long[0],
               new String[0], new UUID[0], new UUID[0], new UUID[0], new long[0]);
         JsonObject mainRoot = JsonParser.parseString(FileUtils.readFileToString(file, StandardCharsets.UTF_8)).getAsJsonObject();
         file.delete();
         assertFalse(findByGuid(mainRoot.getAsJsonArray("rules"), mainRuleGuid).has("chain"));
      }
   }

   /**
    * Import into a server without the chains recreates them with the exported GUIDs and maps chain calls to the
    * new local IDs.
    */
   @Test
   public void testImportRecreatesChains() throws Exception
   {
      JsonObject root = export();
      session.deleteEppChain(caller.getId());
      session.deleteEppChain(called.getId());
      assertNull(session.getEventProcessingPolicyChains().findChain(called.getGuid()));

      session.importConfiguration(root.toString(), 0);

      EventProcessingPolicy policy = session.getEventProcessingPolicyChains();
      EventProcessingPolicyChain newCalled = policy.findChain(called.getGuid());
      EventProcessingPolicyChain newCaller = policy.findChain(caller.getGuid());
      assertNotNull(newCalled);
      assertNotNull(newCaller);
      assertEquals(CHAIN_CALLED, newCalled.getName());
      assertEquals("called chain", newCalled.getDescription());
      assertEquals(1, newCalled.getRuleCount());
      assertEquals(1, newCalled.getCallerCount());

      EventProcessingPolicyRule callerRule = findRule(session.getEventProcessingPolicyChain(newCaller.getId()), callerRuleGuid);
      assertNotNull(callerRule);
      assertEquals(RULE_CALLER, callerRule.getComments());
      assertEquals(Arrays.asList(newCalled.getId()), callerRule.getChainCalls());
      assertNotNull(findRule(session.getEventProcessingPolicyChain(newCalled.getId()), calledRuleGuid));
   }

   /**
    * Existing chain with the same GUID is reused: without the replace flag local name, description and rules are
    * kept; with it they are overwritten. Neither case produces duplicates.
    */
   @Test
   public void testImportIntoExistingChains() throws Exception
   {
      JsonObject root = export();

      session.modifyEppChain(called.getId(), CHAIN_CALLED, "locally changed description", null);
      EventProcessingPolicyChain c = session.getEventProcessingPolicyChain(called.getId());
      findRule(c, calledRuleGuid).setComments(RULE_CALLED + " locally changed");
      assertTrue(session.saveEventProcessingPolicy(c).isSuccess());
      int versionBefore = c.getVersion();

      session.importConfiguration(root.toString(), 0);
      EventProcessingPolicy policy = session.getEventProcessingPolicyChains();
      assertEquals(2, countChains(policy));
      EventProcessingPolicyChain kept = policy.findChain(called.getGuid());
      assertEquals(called.getId(), kept.getId());
      assertEquals("locally changed description", kept.getDescription());
      c = session.getEventProcessingPolicyChain(called.getId());
      assertEquals(1, c.getRules().size());
      assertEquals(RULE_CALLED + " locally changed", findRule(c, calledRuleGuid).getComments());

      session.importConfiguration(root.toString(), NXCSession.CFG_IMPORT_REPLACE_EPP_RULES);
      policy = session.getEventProcessingPolicyChains();
      assertEquals(2, countChains(policy));
      EventProcessingPolicyChain replaced = policy.findChain(called.getGuid());
      assertEquals(called.getId(), replaced.getId());
      assertEquals("called chain", replaced.getDescription());
      c = session.getEventProcessingPolicyChain(called.getId());
      assertEquals(1, c.getRules().size());
      assertEquals(RULE_CALLED, findRule(c, calledRuleGuid).getComments());
      assertTrue(c.getVersion() > versionBefore);
   }

   /**
    * Rule whose owning chain is unknown goes to the main chain; a call to an unknown chain is dropped.
    */
   @Test
   public void testImportWithUnknownChains() throws Exception
   {
      JsonObject root = export();
      session.deleteEppChain(caller.getId());
      session.deleteEppChain(called.getId());

      // Drop the chain registry and point the call to a chain that never existed
      root.remove("chains");
      JsonObject callerJson = findByGuid(root.getAsJsonArray("rules"), callerRuleGuid);
      JsonArray calls = new JsonArray();
      calls.add(UUID.randomUUID().toString());
      callerJson.add("chainCalls", calls);

      session.importConfiguration(root.toString(), 0);

      EventProcessingPolicy policy = session.getEventProcessingPolicyChains();
      assertNull(policy.findChain(called.getGuid()));
      assertNull(policy.findChain(caller.getGuid()));

      EventProcessingPolicyChain main = session.getEventProcessingPolicyChain(0);
      EventProcessingPolicyRule callerRule = findRule(main, callerRuleGuid);
      assertNotNull(callerRule);
      assertEquals(0, callerRule.getChainId());
      assertTrue(callerRule.getChainCalls().isEmpty());
      assertNotNull(findRule(main, calledRuleGuid));
   }

   private static int countChains(EventProcessingPolicy policy)
   {
      int count = 0;
      for(EventProcessingPolicyChain chain : policy.getChains())
         if (chain.getName().equals(CHAIN_CALLED) || chain.getName().equals(CHAIN_CALLER))
            count++;
      return count;
   }
}
