/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.client.events;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * Event processing policy: the registry of rule chains readable by the current user. The main chain has ID 0. Rules of each
 * chain are loaded separately.
 */
public class EventProcessingPolicy
{
   private List<EventProcessingPolicyChain> chains = new ArrayList<>();

   /**
    * Add chain to registry
    *
    * @param chain chain to add
    */
   public void addChain(EventProcessingPolicyChain chain)
   {
      chains.add(chain);
   }

   /**
    * Replace registry entry of a chain with given object (same chain ID), or add it if not present
    *
    * @param chain chain object
    */
   public void putChain(EventProcessingPolicyChain chain)
   {
      chains.removeIf(c -> c.getId() == chain.getId());
      chains.add(chain);
   }

   /**
    * Remove chain from registry
    *
    * @param chainId chain ID
    */
   public void removeChain(int chainId)
   {
      chains.removeIf(c -> c.getId() == chainId);
   }

   /**
    * @return all chains readable by current user
    */
   public List<EventProcessingPolicyChain> getChains()
   {
      return chains;
   }

   /**
    * @return the main chain, or null if it is not readable by current user
    */
   public EventProcessingPolicyChain getMainChain()
   {
      return findChain(0);
   }

   /**
    * Find chain by ID
    *
    * @param chainId chain ID
    * @return chain or null
    */
   public EventProcessingPolicyChain findChain(int chainId)
   {
      for(EventProcessingPolicyChain chain : chains)
         if (chain.getId() == chainId)
            return chain;
      return null;
   }

   /**
    * Find chain by GUID
    *
    * @param guid chain GUID
    * @return chain or null
    */
   public EventProcessingPolicyChain findChain(UUID guid)
   {
      for(EventProcessingPolicyChain chain : chains)
         if (chain.getGuid().equals(guid))
            return chain;
      return null;
   }

   /**
    * Get rules of all loaded chains (main chain first)
    *
    * @return rules of all loaded chains
    */
   public List<EventProcessingPolicyRule> getLoadedRules()
   {
      List<EventProcessingPolicyRule> rules = new ArrayList<>();
      EventProcessingPolicyChain mainChain = getMainChain();
      if ((mainChain != null) && mainChain.isLoaded())
         rules.addAll(mainChain.getRules());
      for(EventProcessingPolicyChain chain : chains)
         if (!chain.isMain() && chain.isLoaded())
            rules.addAll(chain.getRules());
      return rules;
   }
}
