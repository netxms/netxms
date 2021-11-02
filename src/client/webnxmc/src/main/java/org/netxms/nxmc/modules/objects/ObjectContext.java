/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import java.util.Map;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectContextBase;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Class to hold information about selected node
 */
public class ObjectContext extends ObjectContextBase
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectContext.class);

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
   public String substituteMacrosForMultipleNodes(String s, Map<String, String> inputValues)
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
                  sb.append(i18n.tr("<multiple nodes>"));
                  break;
               case 'g':
                  sb.append(i18n.tr("<multiple nodes>"));
                  break;
               case 'i':
                  sb.append(i18n.tr("<multiple nodes>"));
                  break;
               case 'I':
                  sb.append(i18n.tr("<multiple nodes>"));
                  break;
               case 'n':
                  sb.append(i18n.tr("<multiple nodes>"));
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
                     String value = object.getCustomAttributeValue(attr.toString());
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
