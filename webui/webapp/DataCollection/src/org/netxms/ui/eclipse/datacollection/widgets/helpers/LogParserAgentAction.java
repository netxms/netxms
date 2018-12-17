/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Log parser agent action
 */
@Root(name="agentAction", strict=false)
public class LogParserAgentAction
{
   @Attribute(required=false)
   private String action = null;
   
   @Text(required=false)
   private String arguments = "";
   
   /**
    * Protected constructor for XML parser
    */
   protected LogParserAgentAction()
   {
   }
   
   /**
    * @param action
    */
   public LogParserAgentAction(String action)
   {
      setAction(action);
   }
   
   /**
    * @return the action
    */
   public String getAction()
   {
      if (action == null)
         return "";
      
      StringBuilder sb = new StringBuilder(action);
      sb.append(" " + arguments);
      
      return sb.toString();
   }
   
   /**
    * Set action
    * 
    * @param action to set
    */
   public void setAction(String action)
   {
      int index = action.indexOf(" ");
      if (index > 0)
      {
         this.action = action.substring(0, index);
         arguments = action.substring(index + 1, action.length());
      }
      else
         this.action = action;
   }
}
