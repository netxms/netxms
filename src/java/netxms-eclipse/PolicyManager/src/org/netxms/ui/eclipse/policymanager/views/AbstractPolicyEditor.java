/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2018 Raden Solutions
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
package org.netxms.ui.eclipse.policymanager.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.dialogs.SaveStoredConfigDialog;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Base class for all policy editors
 */
public abstract class AbstractPolicyEditor extends ViewPart implements ISaveablePart2
{
   protected NXCSession session;
   protected AgentPolicy policy;
   protected Action actionRefresh;
   protected Action actionSave;

   private boolean modified = false;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      createActions();
      contributeToActionBars();
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionSave = new Action("Save", SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
   }
   
   /**
    * Contribute actions to action bar
    */
   protected void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    *           Menu manager for pull-down menu
    */
   protected void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    *           Menu manager for local toolbar
    */
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return modified;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
   @Override
   public boolean isSaveOnCloseNeeded()
   {
      return modified;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart2#promptToSaveOnClose()
    */
   @Override
   public int promptToSaveOnClose()
   {
      SaveStoredConfigDialog dlg = new SaveStoredConfigDialog(getSite().getShell());
      int rc = dlg.open();
      if (rc == SaveStoredConfigDialog.SAVE_ID)
      {
         modified = false;
         return YES;
      }
      if (rc == SaveStoredConfigDialog.CANCEL)
      {
         return CANCEL;
      }
      return NO;
   }
   
   /**
    * Initiate save operation. Calls doSave on background thread. 
    */
   protected void save()
   {
      new ConsoleJob("Save agent policy", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            doSave(monitor);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  modified = false;
                  firePropertyChange(PROP_DIRTY);
                  actionSave.setEnabled(false);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot save agent policy";
         }
      };
   }
   
   /**
    * Mark content as modified
    */
   protected void setModified()
   {
      if (modified)
         return;
      
      modified = true;
      firePropertyChange(PROP_DIRTY);
      actionSave.setEnabled(true);
   }

   /**
    * @return the modified
    */
   public boolean isModified()
   {
      return modified;
   }

   /**
    * Set policy object to edit
    * 
    * @param policy policy object
    */
   public void setPolicy(AgentPolicy policy)
   {
      this.policy = policy;
      setPartName("Edit Policy \""+ policy.getObjectName() +"\"");
      doRefresh();
   }
   
   /**
    * Refresh editor
    */
   private void refresh()
   {
      if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Confirm Refresh", "This will destroy all unsaved changes. Are you sure? "))
            return;
      }

      policy = session.findObjectById(policy.getObjectId(), AgentPolicy.class);
      actionSave.setEnabled(false);
      doRefresh();
      modified = false;
      firePropertyChange(PROP_DIRTY);
   }
   
   /**
    * Actual implementation for view refresh
    */
   protected abstract void doRefresh();
}
