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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.events.EPPConflict;
import org.netxms.client.events.EPPSaveResult;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.utilities.TestHelperForEpp;

/**
 * Tests for optimistic concurrency control of event processing policy chains: two sessions edit the same chain and
 * the server either merges their changes or reports conflicts. Every test runs on a dedicated chain seeded with three
 * rules R1, R2, R3 so the main chain is never touched.
 */
public class EppConflictTest extends AbstractSessionTest
{
   private static final String CHAIN_NAME = "EppConflictTest";
   private static final String[] SEED_COMMENTS = { "R1", "R2", "R3" };

   private NXCSession sessionA;
   private NXCSession sessionB;
   private int chainId;
   private UUID[] seedGuids = new UUID[SEED_COMMENTS.length];

   /**
    * Create the test chain with rules R1, R2, R3 (chain version 1 after save).
    */
   @BeforeEach
   void createTestChain() throws Exception
   {
      sessionA = connectAndLogin();
      // Second session is created directly: connect() of the base class closes the previous one
      sessionB = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
      sessionB.connect(new int[] { ProtocolVersion.INDEX_FULL });
      sessionB.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);

      TestHelperForEpp.deleteChains(sessionA, CHAIN_NAME);

      EventProcessingPolicyChain chain = sessionA.createEppChain(CHAIN_NAME, "conflict test chain", null);
      chainId = chain.getId();
      for(int i = 0; i < SEED_COMMENTS.length; i++)
      {
         EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
         rule.setChainId(chainId);
         rule.setComments(SEED_COMMENTS[i]);
         chain.addRule(rule);
         seedGuids[i] = rule.getGuid();
      }
      assertTrue(sessionA.saveEventProcessingPolicy(chain).isSuccess());
   }

   @AfterEach
   void deleteTestChain() throws Exception
   {
      if (sessionA != null)
         TestHelperForEpp.deleteChains(sessionA, CHAIN_NAME);
      if (sessionB != null)
         sessionB.disconnect();
   }

   private EventProcessingPolicyChain load(NXCSession session) throws Exception
   {
      return session.getEventProcessingPolicyChain(chainId);
   }

   private static EventProcessingPolicyRule findRule(EventProcessingPolicyChain chain, UUID guid)
   {
      for(EventProcessingPolicyRule rule : chain.getRules())
         if (rule.getGuid().equals(guid))
            return rule;
      return null;
   }

   private static EventProcessingPolicyRule newRule(int chainId, String comment)
   {
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      rule.setChainId(chainId);
      rule.setComments(comment);
      return rule;
   }

   private static List<String> comments(EventProcessingPolicyChain chain)
   {
      List<String> comments = new ArrayList<>();
      for(EventProcessingPolicyRule rule : chain.getRules())
         comments.add(rule.getComments());
      return comments;
   }

   private static void assertSingleConflict(EPPSaveResult result, EPPConflict.Type type, UUID ruleGuid, boolean hasClientRule, boolean hasServerRule)
   {
      assertFalse(result.isSuccess());
      assertEquals(1, result.getConflicts().size());
      EPPConflict conflict = result.getConflicts().get(0);
      assertEquals(type, conflict.getType());
      assertEquals(ruleGuid, conflict.getRuleGuid());
      assertEquals(hasClientRule, conflict.hasClientRule());
      assertEquals(hasServerRule, conflict.hasServerRule());
   }

   /**
    * Save on top of the current chain version: rule and chain versions advance, modification info is recorded.
    */
   @Test
   public void testSaveWithoutConcurrentChanges() throws Exception
   {
      EventProcessingPolicyChain chain = load(sessionA);
      int chainVersion = chain.getVersion();
      EventProcessingPolicyRule r1 = findRule(chain, seedGuids[0]);
      int ruleVersion = r1.getVersion();
      assertTrue(ruleVersion > 0);
      assertFalse(r1.isModified());

      r1.setComments("R1 changed by A");
      assertTrue(r1.isModified());
      EPPSaveResult result = sessionA.saveEventProcessingPolicy(chain);
      assertTrue(result.isSuccess());
      assertTrue(result.getConflicts().isEmpty());
      assertEquals(chainVersion + 1, result.getNewVersion());
      assertEquals(chainVersion + 1, chain.getVersion());
      assertEquals(ruleVersion + 1, r1.getVersion());
      assertFalse(r1.isModified());

      EventProcessingPolicyChain reloaded = load(sessionB);
      assertEquals(chainVersion + 1, reloaded.getVersion());
      EventProcessingPolicyRule reloadedR1 = findRule(reloaded, seedGuids[0]);
      assertEquals("R1 changed by A", reloadedR1.getComments());
      assertEquals(ruleVersion + 1, reloadedR1.getVersion());
      assertEquals(sessionA.getUserName(), reloadedR1.getModifiedByName());
      // Untouched rules keep their versions
      assertEquals(ruleVersion, findRule(reloaded, seedGuids[1]).getVersion());
   }

   /**
    * Two sessions modify different rules: the later save is merged and both changes survive.
    */
   @Test
   public void testMergeDisjointModifications() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);
      int baseVersion = chainA.getVersion();
      assertEquals(baseVersion, chainB.getVersion());

      findRule(chainA, seedGuids[0]).setComments("R1 changed by A");
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());

      findRule(chainB, seedGuids[1]).setComments("R2 changed by B");
      EPPSaveResult result = sessionB.saveEventProcessingPolicy(chainB);
      assertTrue(result.isSuccess());
      assertEquals(baseVersion + 2, result.getNewVersion());

      EventProcessingPolicyChain reloaded = load(sessionA);
      assertEquals(Arrays.asList("R1 changed by A", "R2 changed by B", "R3"), comments(reloaded));
   }

   /**
    * Stale unmodified copy of a rule does not revert the change made by another session.
    */
   @Test
   public void testStaleUnmodifiedCopyKeepsServerVersion() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      findRule(chainA, seedGuids[0]).setComments("R1 changed by A");
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());
      int r1Version = findRule(chainA, seedGuids[0]).getVersion();

      // B saves its stale copy without touching R1
      findRule(chainB, seedGuids[2]).setComments("R3 changed by B");
      assertTrue(sessionB.saveEventProcessingPolicy(chainB).isSuccess());

      EventProcessingPolicyChain reloaded = load(sessionA);
      assertEquals(Arrays.asList("R1 changed by A", "R2", "R3 changed by B"), comments(reloaded));
      assertEquals(r1Version, findRule(reloaded, seedGuids[0]).getVersion());
   }

   /**
    * Same rule modified by both sessions: conflict reported, nothing from the losing save is applied.
    */
   @Test
   public void testModifyConflict() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      findRule(chainA, seedGuids[0]).setComments("R1 changed by A");
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());
      int serverVersion = chainA.getVersion();

      findRule(chainB, seedGuids[0]).setComments("R1 changed by B");
      findRule(chainB, seedGuids[1]).setComments("R2 changed by B");
      EPPSaveResult result = sessionB.saveEventProcessingPolicy(chainB);
      assertSingleConflict(result, EPPConflict.Type.MODIFY, seedGuids[0], true, true);
      assertEquals(serverVersion, result.getNewVersion());
      assertEquals(sessionA.getUserName(), result.getConflicts().get(0).getServerModifiedByName());
      assertTrue(result.getConflicts().get(0).getServerModificationTime() > 0);

      // Losing save is rejected as a whole: R2 change is not applied and chain version is unchanged
      EventProcessingPolicyChain reloaded = load(sessionA);
      assertEquals(serverVersion, reloaded.getVersion());
      assertEquals(Arrays.asList("R1 changed by A", "R2", "R3"), comments(reloaded));

      // Local state of B is left as is, so the user can resolve the conflict and retry
      assertTrue(findRule(chainB, seedGuids[0]).isModified());
      assertTrue(findRule(chainB, seedGuids[1]).isModified());

      // After reload the change goes through
      chainB = load(sessionB);
      findRule(chainB, seedGuids[0]).setComments("R1 changed by B after reload");
      assertTrue(sessionB.saveEventProcessingPolicy(chainB).isSuccess());
      assertEquals("R1 changed by B after reload", findRule(load(sessionA), seedGuids[0]).getComments());
   }

   /**
    * Rule deleted by one session after being modified by another: delete conflict with the server copy attached.
    */
   @Test
   public void testDeleteConflictOnServerModifiedRule() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      findRule(chainA, seedGuids[0]).setComments("R1 changed by A");
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());

      chainB.deleteRule(findRule(chainB, seedGuids[0]));
      assertEquals(1, chainB.getDeletedRules().size());
      EPPSaveResult result = sessionB.saveEventProcessingPolicy(chainB);
      assertSingleConflict(result, EPPConflict.Type.DELETE, seedGuids[0], false, true);
      assertEquals(sessionA.getUserName(), result.getConflicts().get(0).getServerModifiedByName());

      // Rule is still there and deletion request is kept locally for retry
      assertEquals("R1 changed by A", findRule(load(sessionA), seedGuids[0]).getComments());
      assertEquals(1, chainB.getDeletedRules().size());
   }

   /**
    * Rule modified by one session after being deleted by another: delete conflict with the client copy attached.
    */
   @Test
   public void testDeleteConflictOnServerDeletedRule() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      chainA.deleteRule(findRule(chainA, seedGuids[0]));
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());
      assertTrue(chainA.getDeletedRules().isEmpty());

      findRule(chainB, seedGuids[0]).setComments("R1 changed by B");
      EPPSaveResult result = sessionB.saveEventProcessingPolicy(chainB);
      assertSingleConflict(result, EPPConflict.Type.DELETE, seedGuids[0], true, false);

      assertNull(findRule(load(sessionA), seedGuids[0]));
   }

   /**
    * Deletion of a rule nobody else touched is merged; unmodified copy of a rule deleted elsewhere is dropped silently.
    */
   @Test
   public void testMergeDeletions() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      chainA.deleteRule(findRule(chainA, seedGuids[0]));
      findRule(chainA, seedGuids[1]).setComments("R2 changed by A");
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());

      // B still has R1 (unmodified) and deletes R3
      chainB.deleteRule(findRule(chainB, seedGuids[2]));
      EPPSaveResult result = sessionB.saveEventProcessingPolicy(chainB);
      assertTrue(result.isSuccess());
      assertTrue(chainB.getDeletedRules().isEmpty());

      EventProcessingPolicyChain reloaded = load(sessionA);
      assertEquals(Arrays.asList("R2 changed by A"), comments(reloaded));
   }

   /**
    * Both sessions delete the same rule: second deletion is a no-op, not a conflict.
    */
   @Test
   public void testConcurrentDeleteOfSameRule() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      chainA.deleteRule(findRule(chainA, seedGuids[0]));
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());

      chainB.deleteRule(findRule(chainB, seedGuids[0]));
      assertTrue(sessionB.saveEventProcessingPolicy(chainB).isSuccess());

      assertEquals(Arrays.asList("R2", "R3"), comments(load(sessionA)));
   }

   /**
    * New rules saved on top of a changed chain are inserted after their predecessors in server order; a new rule
    * without predecessor goes first, several new rules after the same predecessor keep their relative order.
    */
   @Test
   public void testMergeInsertedRules() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      findRule(chainA, seedGuids[2]).setComments("R3 changed by A");
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());

      chainB.insertRule(newRule(chainId, "N0"), 0);
      chainB.insertRule(newRule(chainId, "N1"), 2);
      chainB.insertRule(newRule(chainId, "N2"), 3);
      assertEquals(Arrays.asList("N0", "R1", "N1", "N2", "R2", "R3"), comments(chainB));
      EPPSaveResult result = sessionB.saveEventProcessingPolicy(chainB);
      assertTrue(result.isSuccess());
      for(EventProcessingPolicyRule rule : chainB.getRules())
      {
         assertTrue(rule.getVersion() > 0);
         assertFalse(rule.isModified());
      }

      EventProcessingPolicyChain reloaded = load(sessionA);
      assertEquals(Arrays.asList("N0", "R1", "N1", "N2", "R2", "R3 changed by A"), comments(reloaded));
   }

   /**
    * Rule added by another session is kept when a stale chain is saved; rule order follows the server.
    */
   @Test
   public void testMergeKeepsRuleAddedOnServer() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      chainA.insertRule(newRule(chainId, "A1"), 1);
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());

      findRule(chainB, seedGuids[0]).setComments("R1 changed by B");
      chainB.addRule(newRule(chainId, "B1"));
      assertTrue(sessionB.saveEventProcessingPolicy(chainB).isSuccess());

      EventProcessingPolicyChain reloaded = load(sessionA);
      assertEquals(Arrays.asList("R1 changed by B", "A1", "R2", "R3", "B1"), comments(reloaded));
   }

   /**
    * Several conflicts are reported together, one per rule.
    */
   @Test
   public void testMultipleConflicts() throws Exception
   {
      EventProcessingPolicyChain chainA = load(sessionA);
      EventProcessingPolicyChain chainB = load(sessionB);

      findRule(chainA, seedGuids[0]).setComments("R1 changed by A");
      chainA.deleteRule(findRule(chainA, seedGuids[1]));
      assertTrue(sessionA.saveEventProcessingPolicy(chainA).isSuccess());

      findRule(chainB, seedGuids[0]).setComments("R1 changed by B");
      findRule(chainB, seedGuids[1]).setComments("R2 changed by B");
      findRule(chainB, seedGuids[2]).setComments("R3 changed by B");
      EPPSaveResult result = sessionB.saveEventProcessingPolicy(chainB);
      assertFalse(result.isSuccess());
      assertEquals(2, result.getConflicts().size());

      EPPConflict modifyConflict = null, deleteConflict = null;
      for(EPPConflict c : result.getConflicts())
      {
         if (c.getRuleGuid().equals(seedGuids[0]))
            modifyConflict = c;
         else if (c.getRuleGuid().equals(seedGuids[1]))
            deleteConflict = c;
      }
      assertNotNull(modifyConflict);
      assertEquals(EPPConflict.Type.MODIFY, modifyConflict.getType());
      assertNotNull(deleteConflict);
      assertEquals(EPPConflict.Type.DELETE, deleteConflict.getType());
      assertTrue(deleteConflict.hasClientRule());
      assertFalse(deleteConflict.hasServerRule());

      // Non-conflicting R3 change was not applied either
      assertEquals(Arrays.asList("R1 changed by A", "R3"), comments(load(sessionA)));
   }
}
