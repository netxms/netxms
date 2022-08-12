/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.ui.eclipse.objects;

import java.util.Map;
import org.eclipse.swt.widgets.Display;
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
    * @param source source string
    * @param inputValues input values provided by user
    * @param display current display
    * @return result after macro substitution in source string
    */
   public String substituteMacrosForMultipleNodes(String source, Map<String, String> inputValues, Display display)
   {
      StringBuilder sb = new StringBuilder();

      char[] src = source.toCharArray();
      for(int i = 0; i < source.length(); i++)
      {
         if (src[i] == '%')
         {
            i++;
            if (i == source.length())
               break;   // malformed string

            switch(src[i])
            {
               case 'a':
                  sb.append(Messages.get(display).ObjectContext_MultipleNodes);
                  break;
               case 'g':
                  sb.append(Messages.get(display).ObjectContext_MultipleNodes);
                  break;
               case 'i':
                  sb.append(Messages.get(display).ObjectContext_MultipleNodes); 
                  break;
               case 'I':
                  sb.append(Messages.get(display).ObjectContext_MultipleNodes);
                  break;
               case 'n':
                  sb.append(Messages.get(display).ObjectContext_MultipleNodes);
                  break;
               case '%':
                  sb.append('%');
                  break;
               case '{':   // object's custom attribute
                  StringBuilder attr = new StringBuilder();
                  for(i++; i < source.length(); i++)
                  {
                     if (src[i] == '}')
                        break;
                     attr.append(src[i]);
                  }
                  if ((object != null) && (attr.length() > 0))
                  {
                     String value = object.getCustomAttributeValue(attr.toString());
                     if (value != null)
                        sb.append(value);
                  }
                  break;
               case '(':   // input field
                  StringBuilder name = new StringBuilder();
                  for(i++; i < source.length(); i++)
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
