/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.AgentPolicy;
import org.netxms.nxmc.modules.datacollection.views.PolicyListView;

/**
 * Label provider for policy list
 */
public class PolicyLabelProvider extends LabelProvider implements ITableLabelProvider
{   
   private Map<String, String> policyTypeDisplayNames;

   /**
    * Default constructor
    */
   public PolicyLabelProvider()
   {
      super();
      policyTypeDisplayNames = new HashMap<String, String>();
      policyTypeDisplayNames.put(AgentPolicy.AGENT_CONFIG, "Agent Configuration");
      policyTypeDisplayNames.put(AgentPolicy.FILE_DELIVERY, "File Delivery");
      policyTypeDisplayNames.put(AgentPolicy.LOG_PARSER, "Log Parser");
      policyTypeDisplayNames.put(AgentPolicy.SUPPORT_APPLICATION, "User Support Application");
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AgentPolicy policy = (AgentPolicy)element;
      switch(columnIndex)
      {
         case PolicyListView.COLUMN_NAME:
            return policy.getName();
         case PolicyListView.COLUMN_TYPE:
            return getPolicyTypeDisplayName(policy.getPolicyType());
         case PolicyListView.COLUMN_GUID:
            return policy.getGuid().toString();
      }
      return null;
   }
   
   /**
    * Get display name for given policy type
    * 
    * @param type policy type
    * @return policy type display name
    */
   public String getPolicyTypeDisplayName(String type)
   {
      return policyTypeDisplayNames.getOrDefault(type, type);
   }
}
