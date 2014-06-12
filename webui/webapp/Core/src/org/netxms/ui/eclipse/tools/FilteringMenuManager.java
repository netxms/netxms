/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.tools;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.ui.IPluginContribution;
import org.eclipse.ui.activities.WorkbenchActivityHelper;

/**
 * Menu manager which can filter actions using activities
 */
public class FilteringMenuManager extends MenuManager
{
   private String pluginId;
   
   /**
    * @param pluginId
    */
   public FilteringMenuManager(String pluginId)
   {
      super();
      this.pluginId = pluginId;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.action.ContributionManager#add(org.eclipse.jface.action.IAction)
    */
   @Override
   public void add(final IAction action)
   {
      if ((action.getId() == null) || 
            !WorkbenchActivityHelper.filterItem(new IPluginContribution() {
               @Override
               public String getPluginId()
               {
                  return pluginId;
               }
               
               @Override
               public String getLocalId()
               {
                  return action.getId();
               }
            }))
      {
         super.add(action);
      }
   }
   
   /**
    * Static helper method for adding with filtering to existing menu managers
    * 
    * @param manager
    * @param action
    * @param pluginId
    */
   public static void add(IMenuManager manager, final IAction action, final String pluginId)
   {
      if ((action.getId() == null) || 
            !WorkbenchActivityHelper.filterItem(new IPluginContribution() {
               @Override
               public String getPluginId()
               {
                  return pluginId;
               }
               
               @Override
               public String getLocalId()
               {
                  return action.getId();
               }
            }))
      {
         manager.add(action);
      }
   }
}
