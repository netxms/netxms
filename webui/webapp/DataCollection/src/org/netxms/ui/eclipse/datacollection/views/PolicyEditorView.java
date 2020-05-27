/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.views;

import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.base.NXCommon;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.dialogs.SavePolicyDialog;
import org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.AgentConfigPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.FileDeliveryPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.GenericPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.LogParserPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.SupportAppPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.PolicyModifyListener;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.MessageBar;

/**
 * Agent policy editor
 */
public class PolicyEditorView extends ViewPart implements ISaveablePart2, SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.datacollection.views.policy_editor"; //$NON-NLS-1$
   
   private AbstractPolicyEditor editor = null;
   private NXCSession session;
   private long templateId;
   private UUID policyGUID = NXCommon.EMPTY_GUID;
   private AgentPolicy policy;
   //private FindReplaceAction actionFindReplace; 
   private boolean throwExceptionOnSave;
   private Exception saveException;
   private Display display;
   private boolean modified = false;
   private boolean modifiedByOtherUser = false;
   private boolean saveInProgress = false;
   
   private CompositeWithMessageBar contentWrapper;
   private Composite content;
   private Action actionSave;
   private Action actionRefresh;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
   }

   /**
    * Set policy object to edit
    * 
    * @param policyGUID policy object GUID
    * @param templateId template object ID
    */
   public void setPolicy(UUID policyGUID, long templateId)
   {
      this.templateId = templateId;
      this.policyGUID = policyGUID;
      modified = false;
      firePropertyChange(PROP_DIRTY);
      actionSave.setEnabled(false);
      refresh();
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      display = parent.getDisplay();
      
      contentWrapper = new CompositeWithMessageBar(parent, SWT.NONE);
      content = new Composite(contentWrapper, SWT.NONE);
      content.setLayout(new FillLayout());
      contentWrapper.setContent(content);

      createActions();
      contributeToActionBars();      

      session.addListener(this);
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionSave = new Action("Save", SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
      actionSave.setEnabled(false);
      
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };

      //actionFindReplace = NXFindAndReplaceAction.getFindReplaceAction(this);
   }

   /**
    * Contribute actions to action bar
    */
   protected void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      bars.getMenuManager().removeAll();
      bars.getToolBarManager().removeAll();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
      bars.updateActionBars();
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    *           Menu manager for pull-down menu
    */
   protected void fillLocalPullDown(IMenuManager manager)
   {
      if (editor != null)
      {
         editor.fillLocalPullDown(manager);
      }
      manager.add(new Separator());
      manager.add(actionSave);
      //manager.add(actionFindReplace);
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
      if (editor != null)
      {
         editor.fillLocalToolBar(manager);
      }
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
   
   /**
    * Refresh editor
    */
   private void refresh()
   {
      if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Refresh policy",
               "This will discard all unsaved changes. Do you really want to continue?"))
            return;

         modified = false;
         firePropertyChange(PROP_DIRTY);
         actionSave.setEnabled(false);         
      }      
      modifiedByOtherUser = false;

      ConsoleJob job = new ConsoleJob("Get agent policy", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final AgentPolicy tmp = session.getAgentPolicy(templateId, policyGUID);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  policy = tmp;
                  if (editor == null)
                  {
                     updateFields();
                  }
                  else 
                  {
                     editor.setPolicy(policy);
                     editor.updateControlFromPolicy();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot load policy";
         }
      };
      job.start();
   }

   /**
    * Update fields with new configuration
    */
   private void updateFields()
   {          
      setPartName(String.format("Agent Policy - %s", policy.getName()));
      if (editor != null)
      {
         editor.dispose();
         editor = null;
      }

      switch(policy.getPolicyType())
      {
         case AgentPolicy.AGENT_CONFIG:
            editor = new AgentConfigPolicyEditor(content, SWT.NONE, policy, this);
            break;
         case AgentPolicy.FILE_DELIVERY:
            editor = new FileDeliveryPolicyEditor(content, SWT.NONE, policy, this);
            break;
         case AgentPolicy.LOG_PARSER:
            editor = new LogParserPolicyEditor(content, SWT.NONE, policy, this);
            break;
         case AgentPolicy.SUPPORT_APPLICATION:
            editor = new SupportAppPolicyEditor(content, SWT.NONE, policy, this);
            break;
         default:
            editor = new GenericPolicyEditor(content, SWT.NONE, policy, this);
            break;
      }

      contributeToActionBars();   // Action bars has to be updated after editor update

      editor.addModifyListener(new PolicyModifyListener() {
         @Override
         public void modifyParser()
         {
            setModified();
         }
      });       

      //editor.setFindAndReplaceAction(actionFindReplace);
      //actionFindReplace.setEnabled(editor.isFindReplaceRequired());
      content.layout(true, true);
      editor.setFocus();
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
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      try
      {
         session.savePolicy(templateId, policy, false);
      }
      catch(Exception e)
      {
         Activator.logError("Exception during policy save", e);
         saveException = e;
         if (!throwExceptionOnSave)
         {
            MessageDialogHelper.openError(getViewSite().getShell(), "Error", String.format("Cannot save policy object: %s", e.getLocalizedMessage()));
         }
      }
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
   }
   
   /**
    * Save policy
    */
   private void save()
   {
      if (modifiedByOtherUser && modified)
      {
         if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Refresh policy",
               "This policy already modified by other users. Do you really want to continue and overwrite other users changes?\n"))
            return;
         contentWrapper.hideMessage();       
      }

      saveInProgress = true;
      modifiedByOtherUser = false;      
      throwExceptionOnSave = true;
      policy = editor.updatePolicyFromControl();
      editor.onSave();
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
                  saveInProgress = false;
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
            saveInProgress = false;
         }
      }.start();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      if (editor != null)
         editor.setFocus();
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return modified;
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   /**
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
      SavePolicyDialog dlg = new SavePolicyDialog(getSite().getShell());
      int rc = dlg.open();
      if (rc == SavePolicyDialog.SAVE_ID)
      {
         if (modifiedByOtherUser && modified)
         {
            if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Refresh policy",
                  "This policy already modified by other users. Do you really want to continue and overwrite other users changes?\n"))
               return CANCEL;
         }      
         policy = editor.updatePolicyFromControl();
         editor.onSave();
         return YES;
      }
      if (rc == SavePolicyDialog.CANCEL)
      {
         return CANCEL;
      }
      editor.onDiscard();
      return NO;
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.POLICY_MODIFIED: 
            if ((n.getSubCode() != templateId) || !((AgentPolicy)n.getObject()).getGuid().equals(policyGUID))
               return;

            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  if (content.isDisposed())
                     return;
                  if (!modified)
                  {
                     policy = (AgentPolicy)n.getObject();
                     editor.setPolicy(policy);
                  }
                  else if (!saveInProgress)
                  { 
                     contentWrapper.showMessage(MessageBar.WARNING,
                           "Policy is modified by other users. \"Refresh\" will discard local changes. \"Save\" will overwrite other users changes with local changes.");
                     modifiedByOtherUser = true;
                  }
               }
            });
            break;
         case SessionNotification.POLICY_DELETED:
            if (n.getSubCode() != templateId || !policyGUID.equals(n.getObject()))
               return;

            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  getViewSite().getPage().hideView(PolicyEditorView.this);
               }
            });
            break;
      }
   }
}
