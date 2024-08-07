/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.views;

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
import org.netxms.base.NXCommon;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.LocalChangeListener;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.SavePolicyDialog;
import org.netxms.nxmc.modules.datacollection.widgets.AbstractPolicyEditor;
import org.netxms.nxmc.modules.datacollection.widgets.AgentConfigPolicyEditor;
import org.netxms.nxmc.modules.datacollection.widgets.FileDeliveryPolicyEditor;
import org.netxms.nxmc.modules.datacollection.widgets.GenericPolicyEditor;
import org.netxms.nxmc.modules.datacollection.widgets.LogParserPolicyEditor;
import org.netxms.nxmc.modules.datacollection.widgets.SupportAppPolicyEditor;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.PolicyModifyListener;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Agent policy editor
 */
public class PolicyEditorView extends AdHocObjectView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(PolicyEditorView.class);
   private static final Logger logger = LoggerFactory.getLogger(PolicyEditorView.class);
   
   private AbstractPolicyEditor editor = null;
   private NXCSession session;
   private long templateId;
   private UUID policyGUID = NXCommon.EMPTY_GUID;
   private AgentPolicy policy;
   private boolean throwExceptionOnSave;
   private Exception saveException;
   private Display display;
   private boolean modified = false;
   private boolean modifiedByOtherUser = false;
   private boolean saveInProgress = false;
   private Composite content;
   private Action actionSave;
   private LocalChangeListener localChangeListener;
   
   private String savedPolicy = null;

   /**
    * Constructor
    */
   public PolicyEditorView(UUID policyGUID, long templateId, LocalChangeListener localChangeListener) 
   {
      super(LocalizationHelper.getI18n(PolicyEditorView.class).tr("Policy Editor"), ResourceManager.getImageDescriptor("icons/object-views/policy.gif"),
            "objects.policy-editor." + policyGUID.toString(), templateId, templateId, false);
      session = Registry.getSession();

      this.templateId = templateId;
      this.policyGUID = policyGUID;
      this.localChangeListener = localChangeListener;
   }

   /**
    * Pop up Constructor
    */
   protected PolicyEditorView() 
   {
      super(LocalizationHelper.getI18n(PolicyEditorView.class).tr("Policy Editor"), ResourceManager.getImageDescriptor("icons/object-views/policy.gif"), null, 0, 0, false); 
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.AdHocObjectView#cloneView()
    */
   @Override
   public View cloneView()
   {
      PolicyEditorView view =  (PolicyEditorView)super.cloneView();
      view.templateId = templateId;
      view.policyGUID = policyGUID;
      view.localChangeListener = localChangeListener;      
      return view;
   }  

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      PolicyEditorView origin = (PolicyEditorView)view;
      origin.editor.updatePolicyFromControl();
      policy = origin.policy;
      modified = origin.modified;
      modifiedByOtherUser = origin.modifiedByOtherUser = false;
      actionSave.setEnabled(modified);   
      updateFields();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      display = parent.getDisplay();
      
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FillLayout());
      
      createActions();    

      session.addListener(this);
   }
   
   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      modified = false;
      actionSave.setEnabled(false);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      modified = false;
      localChangeListener = null;
      super.dispose();
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionSave = new Action("&Save", SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
      actionSave.setEnabled(false);
      addKeyBinding("M1+S", actionSave);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      if (editor != null)
      {
         editor.fillLocalPullDown(manager);
      }
      manager.add(new Separator());
      manager.add(actionSave);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      if (editor != null)
      {
         editor.fillLocalToolBar(manager);
      }
      manager.add(new Separator());
      manager.add(actionSave);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getWindow().getShell(), "Refresh policy", "This will discard all unsaved changes. Do you really want to continue?"))
            return;

         modified = false;
         actionSave.setEnabled(false);         
      }      
      modifiedByOtherUser = false;

      Job job = new Job("Get agent policy", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final AgentPolicy tmp = session.getAgentPolicy(templateId, policyGUID);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  policy = tmp;
                  if (savedPolicy != null)
                  {
                     policy.setContent(savedPolicy);
                     modified = true;
                     actionSave.setEnabled(true); 
                  }
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
      setName(policy.getName());
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

      updateToolBar();
      updateMenu();
      
      editor.addModifyListener(new PolicyModifyListener() {
         @Override
         public void modifyParser()
         {
            setModified();
         }
      });       

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
      actionSave.setEnabled(true);
   }
   
   /**
    * Enable/disable save action
    * 
    * @param enable true to enable
    */
   public void enableSaveAction(boolean enable)
   {
      actionSave.setEnabled(enable && modified);
   }

   /**
    * Save policy
    */
   public void doSave()
   {
      try
      {
         session.savePolicy(templateId, policy, false);
      }
      catch(Exception e)
      {
         logger.error("Exception during policy save", e);
         saveException = e;
         if (!throwExceptionOnSave)
         {
            MessageDialogHelper.openError(getWindow().getShell(), "Error", String.format("Cannot save policy object: %s", e.getLocalizedMessage()));
         }
      }
   }
   
   /**
    * Save policy
    */
   private void save()
   {
      if (modifiedByOtherUser && modified)
      {
         if (!MessageDialogHelper.openQuestion(getWindow().getShell(), "Save policy",
               "This policy already modified by other users. Do you really want to continue and overwrite other users changes?\n"))
            return;
         clearMessages();     
      }

      saveInProgress = true;
      modifiedByOtherUser = false;      
      throwExceptionOnSave = true;
      editor.updatePolicyFromControl();
      editor.onSave();
      new Job("Saving agent policy", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            saveException = null;
            doSave();
            if (saveException != null) 
               throw saveException;

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  modified = false;
                  actionSave.setEnabled(false);
                  saveInProgress = false;
                  if (localChangeListener != null)
                     localChangeListener.onObjectChange();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot save agent policy";
         }

         @Override
         protected void jobFinalize()
         {
            throwExceptionOnSave = false;
            saveInProgress = false;
         }
      }.start();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      if (editor != null)
         editor.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#beforeClose()
    */
   @Override
   public boolean beforeClose()
   {
      if (!modified)
         return true;

      String reason = editor.isSaveAllowed();
      if (reason != null)
      {
         MessageDialogHelper.openWarning(getWindow().getShell(), i18n.tr("Save Not Allowed"), reason);
         return false;
      }

      SavePolicyDialog dlg = new SavePolicyDialog(getWindow().getShell());
      int rc = dlg.open();
      if (rc == SavePolicyDialog.SAVE_ID)
      {
         save();
         return true;
      }
      if (rc == SavePolicyDialog.CANCEL)
      {
         return false;
      }
      editor.onDiscard();
      return true;
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
                     addMessage(MessageArea.WARNING,
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
                  getPerspective().removeMainView(policy.getGuid().toString() + "#" + Long.toString(templateId));
               }
            });
            break;
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);      
      memento.set("templateId", templateId);  
      memento.set("policyGUID", policyGUID);
      if (modified)
      {
         editor.updatePolicyFromControl();
         editor.onSave();
         memento.set("content", policy.getContent());
      }
         
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      templateId = memento.getAsLong("templateId", 0);
      policyGUID = memento.getAsUUID("policyGUID");
      savedPolicy = memento.getAsString("content");
   }
}
