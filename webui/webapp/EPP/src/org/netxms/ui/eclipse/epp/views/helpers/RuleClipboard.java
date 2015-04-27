package org.netxms.ui.eclipse.epp.views.helpers;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Rule clipboard
 */
public class RuleClipboard
{
   private List<EventProcessingPolicyRule> content = new ArrayList<EventProcessingPolicyRule>(0);
   private boolean cloneOnPaste = false;
   
   /**
    * @return
    */
   public boolean isEmpty()
   {
      return content.isEmpty();
   }

   /**
    * Clear clipboard
    */
   public void clear()
   {
      content.clear();
      cloneOnPaste = false;
   }

   /**
    * Add rule to clipboard
    * 
    * @param rule
    */
   public void add(EventProcessingPolicyRule rule)
   {
      content.add(rule);
   }
   
   /**
    * Do paste - returns content (cloned as necessary) ready for pasting
    * 
    * @return
    */
   public Collection<EventProcessingPolicyRule> paste()
   {
      List<EventProcessingPolicyRule> list = new ArrayList<EventProcessingPolicyRule>(content.size());
      if (cloneOnPaste)
      {
         for(EventProcessingPolicyRule r : content)
            list.add(new EventProcessingPolicyRule(r));
      }
      else
      {
         list.addAll(content);
      }
      cloneOnPaste = true;  // next paste of same content should be cloned
      return list;
   }
}
