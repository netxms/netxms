/**
 * 
 */
package org.netxms.ui.eclipse.tools;

import org.eclipse.jface.action.IAction;
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
}
