package org.netxms.utilities;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;

public class TestHelperForEpp
{
   /**
    * Checking if event template with specified name exists if not create new one.
    *
    * @param session
    * @param templateName
    * @return eventTemplate
    * @throws Exception
    */
   public static EventTemplate findOrCreateEvent(final NXCSession session, String templateName) throws Exception
   {
      EventTemplate eventTemplate = null;

      for(EventTemplate template : session.getEventTemplates())
      {
         if (template.getName().equals(templateName))
         {
            eventTemplate = template;
            break;
         }
      }
      if (eventTemplate == null)
      {
         EventTemplate template = new EventTemplate(0);
         template.setName(templateName);
         session.modifyEventObject(template);
         eventTemplate = template;
      }

      return eventTemplate;

   }

   /**
    * Searching for test rule based on the specified comment, if not found, creates a new one.
    *
    * @param session
    * @param chain chain to search in (must be loaded); a created rule is appended to it and the chain is saved
    * @param commentForSearching
    * @param eventTemplate
    * @param node
    * @return EventProcessingPolicyRule
    * @throws Exception
    */
   public static EventProcessingPolicyRule findOrCreateRule(final NXCSession session, EventProcessingPolicyChain chain, String commentForSearching, EventTemplate eventTemplate, AbstractObject node)
         throws Exception
   {
      EventProcessingPolicyRule testRule = null;

      for(EventProcessingPolicyRule rule : chain.getRules())
      {
         if (rule.getComments().equals(commentForSearching))
         {
            testRule = rule;
            break;
         }
      }
      if (testRule == null)
      {
         testRule = new EventProcessingPolicyRule();
         testRule.setChainId(chain.getId());
         testRule.setRuleNumber(chain.getRules().size());
         testRule.setComments(commentForSearching);
         List<Integer> evnts = new ArrayList<>();
         evnts.add(eventTemplate.getCode());
         testRule.setEvents(evnts);
         chain.insertRule(testRule, testRule.getRuleNumber());
         session.saveEventProcessingPolicy(chain);
      }
      return testRule;
   }

   /**
    * Searching for entry in persistence storage based on the specified key.
    *
    * @param session
    * @param key
    * @return
    * @throws Exception
    */
   public static String findPsValueByKey(NXCSession session, String key) throws Exception
   {
      HashMap<String, String> allPersistentStorageValue = session.getPersistentStorageList();
      return allPersistentStorageValue.get(key);
   }

   /**
    * Delete main chain rules whose comment starts with given prefix
    */
   public static void deleteRules(NXCSession session, String commentPrefix) throws Exception
   {
      EventProcessingPolicyChain mainChain = session.getEventProcessingPolicyChain(0);
      boolean changed = false;
      for(EventProcessingPolicyRule rule : new ArrayList<>(mainChain.getRules()))
      {
         if ((rule.getComments() != null) && rule.getComments().startsWith(commentPrefix))
         {
            mainChain.deleteRule(rule);
            changed = true;
         }
      }
      if (changed)
         session.saveEventProcessingPolicy(mainChain);
   }

   /**
    * Delete chains whose name starts with given prefix
    */
   public static void deleteChains(NXCSession session, String namePrefix) throws Exception
   {
      EventProcessingPolicy policy = session.getEventProcessingPolicyChains();
      for(EventProcessingPolicyChain chain : policy.getChains())
         if (!chain.isMain() && chain.getName().startsWith(namePrefix))
            session.deleteEppChain(chain.getId());
   }

   /**
    * Delete persistent storage values whose key starts with given prefix
    */
   public static void deletePersistentStorageValues(NXCSession session, String keyPrefix) throws Exception
   {
      for(String key : session.getPersistentStorageList().keySet())
         if (key.startsWith(keyPrefix))
            session.deletePersistentStorageValue(key);
   }

   /**
    * Terminate alarms whose key starts with given prefix
    */
   public static void terminateAlarms(NXCSession session, String keyPrefix) throws Exception
   {
      for(Alarm alarm : session.getAlarms().values())
         if (alarm.getKey().startsWith(keyPrefix))
            session.terminateAlarm(alarm.getId());
   }

   /**
    * Delete scheduled tasks with given key
    */
   public static void deleteScheduledTasks(NXCSession session, String key) throws Exception
   {
      for(ScheduledTask task : session.getScheduledTasks())
         if (key.equals(task.getKey()))
            session.deleteScheduledTask(task.getId());
   }
}
