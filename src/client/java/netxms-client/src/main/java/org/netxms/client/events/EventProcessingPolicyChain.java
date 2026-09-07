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
package org.netxms.client.events;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.client.AccessListElement;

/**
 * Event processing policy rule chain. Chain 0 is the main chain (entry point of event processing); it cannot be renamed, deleted,
 * or given an access control list. Registry data (name, description, version, rights, counters) comes with the policy; rules are
 * loaded separately per chain and are null until loaded.
 */
public class EventProcessingPolicyChain
{
   public static final int ACCESS_READ = 0x0001;
   public static final int ACCESS_EDIT = 0x0002;

   /**
    * Rule deleted locally but not yet saved (its last known version is sent to the server for conflict detection)
    */
   public static class DeletedRuleInfo
   {
      private UUID guid;
      private int version;

      /**
       * @param guid rule GUID
       * @param version rule version at the time of deletion
       */
      public DeletedRuleInfo(UUID guid, int version)
      {
         this.guid = guid;
         this.version = version;
      }

      /**
       * @return rule GUID
       */
      public UUID getGuid()
      {
         return guid;
      }

      /**
       * @return rule version at the time of deletion
       */
      public int getVersion()
      {
         return version;
      }
   }

   private int id;
   private UUID guid;
   private String name;
   private String description;
   private int version;
   private int effectiveRights;
   private int ruleCount;
   private int callerCount;
   private List<AccessListElement> accessList = new ArrayList<>();
   private List<EventProcessingPolicyRule> rules = null;
   private List<DeletedRuleInfo> deletedRules = new ArrayList<>();

   /**
    * Create chain object from registry entry in NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID of the registry entry
    */
   public EventProcessingPolicyChain(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      guid = msg.getFieldAsUUID(baseId + 1);
      name = msg.getFieldAsString(baseId + 2);
      description = msg.getFieldAsString(baseId + 3);
      version = msg.getFieldAsInt32(baseId + 4);
      effectiveRights = msg.getFieldAsInt32(baseId + 5);
      ruleCount = msg.getFieldAsInt32(baseId + 6);
      callerCount = msg.getFieldAsInt32(baseId + 7);
   }

   /**
    * Create chain object for a newly created (empty) chain.
    *
    * @param id chain ID assigned by server
    * @param guid chain GUID assigned by server
    * @param name chain name
    * @param description chain description
    */
   public EventProcessingPolicyChain(int id, UUID guid, String name, String description)
   {
      this.id = id;
      this.guid = guid;
      this.name = name;
      this.description = description;
      version = 1;
      effectiveRights = ACCESS_READ | ACCESS_EDIT;
      rules = new ArrayList<>();
   }

   /**
    * @return chain ID (0 for the main chain)
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return true if this is the main chain
    */
   public boolean isMain()
   {
      return id == 0;
   }

   /**
    * @return chain GUID
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @return chain name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name chain name
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return chain description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description chain description
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @return chain version (optimistic concurrency base for saving this chain's rules)
    */
   public int getVersion()
   {
      return version;
   }

   /**
    * @param version chain version
    */
   public void setVersion(int version)
   {
      this.version = version;
   }

   /**
    * @return effective rights of current user on this chain (bitmask of ACCESS_READ, ACCESS_EDIT)
    */
   public int getEffectiveRights()
   {
      return effectiveRights;
   }

   /**
    * @return true if current user can edit rules of this chain
    */
   public boolean isEditable()
   {
      return (effectiveRights & ACCESS_EDIT) != 0;
   }

   /**
    * @return number of rules in this chain as reported by server (available before rules are loaded)
    */
   public int getRuleCount()
   {
      return (rules != null) ? rules.size() : ruleCount;
   }

   /**
    * @return number of rules (in any chain) calling this chain as reported by server
    */
   public int getCallerCount()
   {
      return callerCount;
   }

   /**
    * @return chain access control list (available only to users with the global EPP right)
    */
   public List<AccessListElement> getAccessList()
   {
      return accessList;
   }

   /**
    * @param accessList chain access control list
    */
   public void setAccessList(List<AccessListElement> accessList)
   {
      this.accessList = accessList;
   }

   /**
    * @return true if rules of this chain were loaded
    */
   public boolean isLoaded()
   {
      return rules != null;
   }

   /**
    * @return rules in evaluation order, or null if not loaded
    */
   public List<EventProcessingPolicyRule> getRules()
   {
      return rules;
   }

   /**
    * Replace rules and version with data loaded from server. Pending deletions are discarded.
    *
    * @param rules rules in evaluation order
    * @param version chain version
    */
   public void setRules(List<EventProcessingPolicyRule> rules, int version)
   {
      this.rules = rules;
      this.version = version;
      deletedRules.clear();
   }

   /**
    * Add rule at the end of the chain
    *
    * @param rule rule to add
    */
   public void addRule(EventProcessingPolicyRule rule)
   {
      rules.add(rule);
   }

   /**
    * Insert rule at given position
    *
    * @param rule rule to insert
    * @param index position (0-based)
    */
   public void insertRule(EventProcessingPolicyRule rule, int index)
   {
      rules.add(index, rule);
   }

   /**
    * Delete rule; its GUID and version are remembered until the chain is saved so the server can detect concurrent changes.
    *
    * @param rule rule to delete
    */
   public void deleteRule(EventProcessingPolicyRule rule)
   {
      if (rules.remove(rule) && (rule.getVersion() > 0))
         deletedRules.add(new DeletedRuleInfo(rule.getGuid(), rule.getVersion()));
   }

   /**
    * @return rules deleted since last save
    */
   public List<DeletedRuleInfo> getDeletedRules()
   {
      return deletedRules;
   }

   /**
    * Forget deleted rules (after successful save)
    */
   public void clearDeletedRules()
   {
      deletedRules.clear();
   }
}
