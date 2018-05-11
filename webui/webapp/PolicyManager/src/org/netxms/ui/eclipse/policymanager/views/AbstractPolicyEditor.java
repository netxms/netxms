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
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.policymanager.dialogs.SavePolicyOnCloseDialog;
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

   private Display display;
   private boolean modified = false;
   private boolean throwExceptionOnSave = false;
   private Exception saveException = null;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      display = Display.getCurrent();
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
   
   /**
    * Internal save implementation
    * 
    * @param monitor
    */
   protected abstract void doSaveInternal(IProgressMonitor monitor) throws Exception;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public final void doSave(IProgressMonitor monitor)
   {
      try
      {
         doSaveInternal(monitor);
         display.asyncExec(new Runnable() {
            @Override
            public void run()
            {
               if (MessageDialogHelper.openQuestion(getSite().getShell(), "Install Policy", "Policy successfully saved. Install new version on all nodes where this policy is already installed?"))
                  deployPolicyOnExistingNodes();
            }
         });
      }
      catch(Exception e)
      {
         saveException = e;
         if (!throwExceptionOnSave)
         {
            MessageDialogHelper.openError(getViewSite().getShell(), "Error", String.format("Cannot save policy object: %s", e.getLocalizedMessage()));
         }
      }
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
      SavePolicyOnCloseDialog dlg = new SavePolicyOnCloseDialog(getSite().getShell());
      int rc = dlg.open();
      if (rc == SavePolicyOnCloseDialog.SAVE_ID)
      {
         modified = false;
         return YES;
      }
      if (rc == SavePolicyOnCloseDialog.CANCEL)
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
      throwExceptionOnSave = true;
      new ConsoleJob("Save agent policy", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            saveException = null;
            doSave(monitor);
            if (saveException != null)
               throw saveException;
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

         /* (non-Javadoc)
          * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
          */
         @Override
         protected void jobFinalize()
         {
            throwExceptionOnSave = false;
         }
      }.start();
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
      modified = false;
      firePropertyChange(PROP_DIRTY);
      actionSave.setEnabled(false);
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
      actionSave.setEnabled(false);
   }
   
   /**
    * Actual implementation for view refresh
    */
   protected abstract void doRefresh();
   
   /**
    * Deploy policy on nodes where it is already installed
    */
   private void deployPolicyOnExistingNodes()
   {
      ConsoleJob job = new ConsoleJob("Deploy policy", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(long nodeId : policy.getChildIdList())
            {
               try
               {
                  session.deployAgentPolicy(policy.getObjectId(), nodeId);
               }
               catch(Exception e)
               {
                  Activator.logError("Exception in deployPolicyOnExistingNodes", e);
               }
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setSystem(true);
      job.setUser(false);
      job.start();
   }
}
