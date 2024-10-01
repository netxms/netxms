package org.netxms.utilities;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
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
    * @param policy
    * @param commentForSearching
    * @param eventTemplate
    * @param node
    * @return EventProcessingPolicyRule
    * @throws Exception
    */
   public static EventProcessingPolicyRule findOrCreateRule(final NXCSession session, EventProcessingPolicy policy, String commentForSearching, EventTemplate eventTemplate, AbstractObject node)
         throws Exception
   {
      EventProcessingPolicyRule testRule = null;

      for(EventProcessingPolicyRule rule : policy.getRules())
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
         testRule.setRuleNumber(policy.getRules().size());
         testRule.setComments(commentForSearching);
         List<Integer> evnts = new ArrayList<>();
         evnts.add(eventTemplate.getCode());
         testRule.setEvents(evnts);
         policy.insertRule(testRule, testRule.getRuleNumber());
         session.saveEventProcessingPolicy(policy);
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

}
