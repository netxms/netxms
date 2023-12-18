package org.netxms.utilities;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;

public class TestHelperForEpp
{
   public static EventTemplate findOrCreateEvent(final NXCSession session, String templateName) throws Exception
   {
      EventTemplate eventTemplate = null;

      for(EventTemplate template : session.getEventTemplates())
      {
         if (template.getName().equals(templateName))// Checking if an event template for the alarm generation rule with the
                                                     // specified name exists
         {
            eventTemplate = template;
            break;
         }
      }
      if (eventTemplate == null)// If not exists, creating a new one
      {
         EventTemplate template = new EventTemplate(0);
         template.setName(templateName);
         session.modifyEventObject(template);
         eventTemplate = template;
      }

      return eventTemplate;

   }

   public static EventProcessingPolicyRule findOrCreateRule(final NXCSession session, EventProcessingPolicy policy, String commentForSearching, EventTemplate eventTemplate, AbstractObject node)
         throws Exception
   {
      EventProcessingPolicyRule testRule = null;

      for(EventProcessingPolicyRule rule : policy.getRules())// Searching for test rule based on the specified comment;
      {
         if (rule.getComments().equals(commentForSearching))
         {
            testRule = rule;
            break;
         }
      }
      if (testRule == null) // if not found, creates a new one;
      {
         testRule = new EventProcessingPolicyRule();
         testRule.setRuleNumber(policy.getRules().size());
         testRule.setComments(commentForSearching);
         List<Long> evnts = new ArrayList<Long>();
         evnts.add(eventTemplate.getCode());
         testRule.setEvents(evnts);
         session.saveEventProcessingPolicy(policy);
         policy.insertRule(testRule, testRule.getRuleNumber());
      }
      return testRule;
   }

}
