package org.netxms.ui.eclipse.objects;

import java.util.Map;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectContextBase;
import org.netxms.ui.eclipse.console.Messages;

/**
 * Class to hold information about selected node
 */
public class ObjectContext extends ObjectContextBase
{   
   /**
    * @param object
    * @param alarm
    */
   public ObjectContext(AbstractNode object, Alarm alarm)
   {
      super(object, alarm);
   }

   /**
    * @param s
    * @param inputValues
    * @return
    */
   public String substituteMacrosForMultiNodes(String s, Map<String, String> inputValues)
   {
      StringBuilder sb = new StringBuilder();
      
      char[] src = s.toCharArray();
      for(int i = 0; i < s.length(); i++)
      {
         if (src[i] == '%')
         {
            i++;
            if (i == s.length())
               break;   // malformed string
            
            switch(src[i])
            {
               case 'a':
                  sb.append(Messages.get().ObjectContext_MultipleNodes);
                  break;
               case 'g':
                  sb.append(Messages.get().ObjectContext_MultipleNodes);
                  break;
               case 'i':
                  sb.append(Messages.get().ObjectContext_MultipleNodes); 
                  break;
               case 'I':
                  sb.append(Messages.get().ObjectContext_MultipleNodes);
                  break;
               case 'n':
                  sb.append(Messages.get().ObjectContext_MultipleNodes);
                  break;
               case '%':
                  sb.append('%');
                  break;
               case '{':   // object's custom attribute
                  StringBuilder attr = new StringBuilder();
                  for(i++; i < s.length(); i++)
                  {
                     if (src[i] == '}')
                        break;
                     attr.append(src[i]);
                  }
                  if ((object != null) && (attr.length() > 0))
                  {
                     String value = object.getCustomAttributes().get(attr.toString());
                     if (value != null)
                        sb.append(value);
                  }
                  break;
               case '(':   // input field
                  StringBuilder name = new StringBuilder();
                  for(i++; i < s.length(); i++)
                  {
                     if (src[i] == ')')
                        break;
                     name.append(src[i]);
                  }
                  if (name.length() > 0)
                  {
                     String value = inputValues.get(name.toString());
                     if (value != null)
                        sb.append(value);
                  }
                  break;
               default:
                  break;
            }
         }
         else
         {
            sb.append(src[i]);
         }
      }
      
      return sb.toString();
   }
   
   /**
    * Returns alarm id or 0 if alarm is not set
    * 
    * @return Context alarm id or 0 if alarm is not set
    */
   public long getAlarmId()
   {
      return alarm != null ? alarm.getId() : 0;
   }
}