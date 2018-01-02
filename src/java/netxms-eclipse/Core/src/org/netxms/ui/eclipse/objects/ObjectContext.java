package org.netxms.ui.eclipse.objects;

import java.util.Map;
import org.netxms.base.NXCommon;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Class to hold information about selected node
 */
public class ObjectContext
{
   public AbstractNode object;
   public Alarm alarm;
   
   public ObjectContext(AbstractNode object, Alarm alarm)
   {
      this.object = object;
      this.alarm = alarm;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + ((alarm == null) ? 0 : alarm.hashCode());
      result = prime * result + ((object == null) ? 0 : object.hashCode());
      return result;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      if (this == obj)
         return true;
      if (obj == null)
         return false;
      if (getClass() != obj.getClass())
         return false;
      ObjectContext other = (ObjectContext)obj;
      if ((other.object == null) || (this.object == null))
         return (other.object == null) && (this.object == null);
      return other.object.getObjectId() == this.object.getObjectId();
   }
   
   /**
    * Substitute macros in string
    * 
    * @param s
    * @param node
    * @param inputValues 
    * @return
    */
   public String substituteMacros(String s, Map<String, String> inputValues)
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
                  sb.append((object != null) ? object.getPrimaryIP().getHostAddress() : Messages.get().ObjectContext_MultipleNodes);
                  break;
               case 'A':   // alarm message
                  if (alarm != null)
                     sb.append(alarm.getMessage());
                  break;
               case 'c':
                  if (alarm != null)
                     sb.append(alarm.getSourceEventCode());
                  break;
               case 'g':
                  sb.append((object != null) ? object.getGuid().toString() : Messages.get().ObjectContext_MultipleNodes);
                  break;
               case 'i':
                  sb.append((object != null) ? String.format("0x%08X", object.getObjectId()) : Messages.get().ObjectContext_MultipleNodes); //$NON-NLS-1$
                  break;
               case 'I':
                  sb.append((object != null) ? Long.toString(object.getObjectId()) : Messages.get().ObjectContext_MultipleNodes);
                  break;
               case 'm':   // alarm message
                  if (alarm != null)
                     sb.append(alarm.getMessage());
                  break;
               case 'n':
                  sb.append((object != null) ? object.getObjectName() : Messages.get().ObjectContext_MultipleNodes);
                  break;
               case 'N':
                  if (alarm != null)
                     sb.append(ConsoleSharedData.getSession().getEventName(alarm.getSourceEventCode()));
                  break;
               case 's':
                  if (alarm != null)
                     sb.append(alarm.getCurrentSeverity());
                  break;
               case 'S':
                  if (alarm != null)
                     sb.append(StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity()));
                  break;
               case 'U':
                  sb.append(ConsoleSharedData.getSession().getUserName());
                  break;
               case 'v':
                  sb.append(NXCommon.VERSION);
                  break;
               case 'y':   // alarm state
                  if (alarm != null)
                     sb.append(alarm.getState());
                  break;
               case 'Y':   // alarm ID
                  if (alarm != null)
                     sb.append(alarm.getId());
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
}
