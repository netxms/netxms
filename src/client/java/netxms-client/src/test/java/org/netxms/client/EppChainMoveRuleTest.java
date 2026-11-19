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
package org.netxms.client;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.UUID;
import org.junit.jupiter.api.Test;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Tests for EventProcessingPolicyChain.moveRule
 */
public class EppChainMoveRuleTest
{
   private static EventProcessingPolicyRule savedRule(EventProcessingPolicyChain chain, int version)
   {
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      rule.setChainId(chain.getId());
      rule.setVersion(version);
      rule.setModified(false);
      chain.addRule(rule);
      return rule;
   }

   @Test
   public void testMoveKeepsGuidAndResetsVersion()
   {
      EventProcessingPolicyChain a = new EventProcessingPolicyChain(1, UUID.randomUUID(), "A", "");
      EventProcessingPolicyChain b = new EventProcessingPolicyChain(2, UUID.randomUUID(), "B", "");
      EventProcessingPolicyRule existing = savedRule(b, 2);
      EventProcessingPolicyRule other = savedRule(a, 5);
      EventProcessingPolicyRule rule = savedRule(a, 3);
      UUID guid = rule.getGuid();

      a.moveRule(rule, b);

      assertEquals(1, a.getRules().size());
      assertTrue(a.getRules().get(0) == other);
      assertEquals(1, a.getDeletedRules().size());
      assertEquals(guid, a.getDeletedRules().get(0).getGuid());
      assertEquals(3, a.getDeletedRules().get(0).getVersion());

      assertEquals(2, b.getRules().size());
      assertTrue(b.getRules().get(0) == existing);
      assertTrue(b.getRules().get(1) == rule);
      assertEquals(guid, rule.getGuid());
      assertEquals(0, rule.getVersion());
      assertTrue(rule.isModified());
      assertEquals(2, rule.getChainId());
      assertTrue(b.getDeletedRules().isEmpty());
   }

   @Test
   public void testMoveBackRestoresVersion()
   {
      EventProcessingPolicyChain a = new EventProcessingPolicyChain(1, UUID.randomUUID(), "A", "");
      EventProcessingPolicyChain b = new EventProcessingPolicyChain(2, UUID.randomUUID(), "B", "");
      EventProcessingPolicyRule unrelated = savedRule(a, 4);
      a.deleteRule(unrelated);
      EventProcessingPolicyRule rule = savedRule(a, 3);

      a.moveRule(rule, b);
      b.moveRule(rule, a);

      assertEquals(1, a.getRules().size());
      assertTrue(a.getRules().get(0) == rule);
      assertEquals(3, rule.getVersion());
      assertEquals(1, rule.getChainId());
      assertEquals(1, a.getDeletedRules().size());
      assertEquals(unrelated.getGuid(), a.getDeletedRules().get(0).getGuid());

      assertTrue(b.getRules().isEmpty());
      assertTrue(b.getDeletedRules().isEmpty());
   }

   @Test
   public void testMoveUnsavedRule()
   {
      EventProcessingPolicyChain a = new EventProcessingPolicyChain(1, UUID.randomUUID(), "A", "");
      EventProcessingPolicyChain b = new EventProcessingPolicyChain(2, UUID.randomUUID(), "B", "");
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      a.addRule(rule);

      a.moveRule(rule, b);

      assertTrue(a.getRules().isEmpty());
      assertTrue(a.getDeletedRules().isEmpty());
      assertEquals(1, b.getRules().size());
      assertEquals(0, rule.getVersion());
   }
}
