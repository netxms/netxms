/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
 * This class represents NetXMS event processing policy.
 */
public class EventProcessingPolicy
{
   /**
    * Information about a deleted rule for conflict detection
    */
   public static class DeletedRuleInfo
   {
      private UUID guid;
      private int version;

      /**
       * Create deleted rule info.
       *
       * @param guid rule GUID
       * @param version rule version when deleted
       */
      public DeletedRuleInfo(UUID guid, int version)
      {
         this.guid = guid;
         this.version = version;
      }

      /**
       * Get rule GUID.
       *
       * @return rule GUID
       */
      public UUID getGuid()
      {
         return guid;
      }

      /**
       * Get rule version when it was deleted.
       *
       * @return rule version
       */
      public int getVersion()
      {
         return version;
      }
   }

   private List<EventProcessingPolicyRule> rules;
   private int version;
   private List<DeletedRuleInfo> deletedRules;

   /**
    * Create new policy object.
    *
    * @param numRules Expected number of rules
    */
   public EventProcessingPolicy(int numRules)
   {
      rules = new ArrayList<EventProcessingPolicyRule>(numRules);
      version = 0;
      deletedRules = new ArrayList<>();
   }

   /**
    * Create new policy object with version.
    *
    * @param numRules Expected number of rules
    * @param version Policy version from server
    */
   public EventProcessingPolicy(int numRules, int version)
   {
      rules = new ArrayList<EventProcessingPolicyRule>(numRules);
      this.version = version;
      deletedRules = new ArrayList<>();
   }
	
	/**
	 * Add new rule.
	 * 
	 * @param rule Rule to add
	 */
	public void addRule(EventProcessingPolicyRule rule)
	{
		rules.add(rule);
	}
	
	/**
	 * Insert rule before rule at given position
	 * 
	 * @param rule rule to insert
	 * @param index position to insert at
	 */
	public void insertRule(EventProcessingPolicyRule rule, int index)
	{
		rules.add(index, rule);
	}

   /**
    * Delete rule.
    *
    * @param rule rule object to be remove
    */
   public void deleteRule(EventProcessingPolicyRule rule)
   {
      if (rules.remove(rule))
      {
         // Track deletion for conflict detection (only if rule has a server version)
         if (rule.getVersion() > 0)
         {
            deletedRules.add(new DeletedRuleInfo(rule.getGuid(), rule.getVersion()));
         }
      }
   }

   /**
    * @return the rules
    */
   public List<EventProcessingPolicyRule> getRules()
   {
      return rules;
   }

   /**
    * Get policy version for optimistic concurrency control.
    *
    * @return policy version
    */
   public int getVersion()
   {
      return version;
   }

   /**
    * Set policy version.
    *
    * @param version new version
    */
   public void setVersion(int version)
   {
      this.version = version;
   }

   /**
    * Get list of deleted rules.
    *
    * @return list of deleted rules
    */
   public List<DeletedRuleInfo> getDeletedRules()
   {
      return deletedRules;
   }

   /**
    * Clear deleted rules list.
    */
   public void clearDeletedRules()
   {
      deletedRules.clear();
   }
}
