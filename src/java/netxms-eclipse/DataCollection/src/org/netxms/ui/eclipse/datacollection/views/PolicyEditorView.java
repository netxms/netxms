package org.netxms.ui.eclipse.datacollection.views;

import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.text.IFindReplaceTarget;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.texteditor.FindReplaceAction;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.dialogs.SavePolicyDialog;
import org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.AgentConfigPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.GenericPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.LogParserPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.SupportAppPolicyEditor;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.PolicyModifyListener;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.NXFindAndReplaceAction;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;

public class PolicyEditorView extends ViewPart implements ISaveablePart2, SessionListener, IFindReplaceTarget
{
   public static final String ID = "org.netxms.ui.eclipse.datacollection.views.policy_editor"; //$NON-NLS-1$
   
   private AbstractPolicyEditor editor = null;
   private NXCSession session;
   private long templateId;
   private UUID policyGUID;
   private AgentPolicy policy;
   private boolean modified;
   private FindReplaceAction actionFindReplace; 
   private boolean throwExceptionOnSave;
   private Exception saveException;
   private Display display;
   private boolean modifiedBuOtherUser = false;
   private boolean saveInProgress = false;
   
   private CompositeWithMessageBar contentWrapper;
   private Composite content;
   private Action actionSave;
   private Action actionRefresh;


   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
   }

   /**
    * Set template object to edit
    * 
    * @param policy policy object
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

      actionFindReplace = NXFindAndReplaceAction.getFindReplaceAction(this);
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
      manager.add(actionFindReplace);
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
      manager.add(actionFindReplace);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
   
   private void refresh()
   {
      if(modifiedBuOtherUser && modified)
      {
         if (!MessageDialogHelper.openConfirm(getSite().getShell(), "Refresh policy",
                                        "Do you really want to refresh the policy?\n You will loose all your changes."))
            return;
         
         modified = false;
         firePropertyChange(PROP_DIRTY);
         actionSave.setEnabled(false);         
      }      
      modifiedBuOtherUser = false;
      
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
                  updateFields();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Can not load policy";
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
      
      if (policy.getPolicyType().equals(AgentPolicy.agent))
      {
         editor = new AgentConfigPolicyEditor(content, SWT.NONE, policy);            
      }
      else if (policy.getPolicyType().equals(AgentPolicy.logParser))
      {
         editor = new LogParserPolicyEditor(content, SWT.NONE, policy);      
      }
      else if (policy.getPolicyType().equals(AgentPolicy.supportApplication))
      {
         editor = new SupportAppPolicyEditor(content, SWT.NONE, policy);  
      }
      else
      {
         editor = new GenericPolicyEditor(content, SWT.NONE, policy);
      }
      
      editor.addModifyListener(new PolicyModifyListener() {
         @Override
         public void modifyParser()
         {
            setModified();
         }
      });       

      editor.setFindAndReplaceAction(actionFindReplace);
      actionFindReplace.setEnabled(editor.isFindReplaceRequired());
      actionFindReplace.update();
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
   
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      try
      {
         session.savePolicy(templateId, policy);
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

   @Override
   public void doSaveAs()
   {
   }
   
   /**
    * Save policy
    */
   private void save()
   {
      if(modifiedBuOtherUser && modified)
      {
         if (!MessageDialogHelper.openConfirm(getSite().getShell(), "Refresh policy",
                                        "Do you really want to save the policy?\n You will overwrite other user changes."))
            return;
         contentWrapper.hideMessage();       
      }     
      saveInProgress = true;
      modifiedBuOtherUser = false;      
      throwExceptionOnSave = true;
      policy = editor.getUpdatedPolicy();
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
            saveInProgress = false;
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

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      if(editor != null)
         editor.setFocus();
   }

   @Override
   public boolean isDirty()
   {
      return modified;
   }

   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

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
         if (modifiedBuOtherUser && modified)
         {
            if (!MessageDialogHelper.openConfirm(getSite().getShell(), "Refresh policy",
                                           "Do you really want to save the policy?\n You will overwrite other user changes."))
               return CANCEL;
         }      
         policy = editor.getUpdatedPolicy();
         return YES;
      }
      if (rc == SavePolicyDialog.CANCEL)
      {
         return CANCEL;
      }
      return NO;
   }

   @Override
   public void notificationHandler(SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.POLICY_MODIFIED: 
            if (n.getSubCode() != templateId && !((AgentPolicy)n.getObject()).getGuid().equals(policyGUID))
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
                     updateFields();
                  }
                  else if (!saveInProgress)
                  { 
                     contentWrapper.showMessage(0, "Policy is modified by other user");
                     modifiedBuOtherUser = true;
                  }
               }
            });
            break;
         case SessionNotification.POLICY_DELETED:
            if (n.getSubCode() != templateId && !((AgentPolicy)n.getObject()).getGuid().equals(policyGUID))
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

   @Override
   public boolean canPerformFind()
   {
      return editor != null ? editor.canPerformFind() : false;
   }

   @Override
   public int findAndSelect(int widgetOffset, String findString, boolean searchForward, boolean caseSensitive, boolean wholeWord)
   {
      return editor != null ? editor.findAndSelect(widgetOffset, findString, searchForward, caseSensitive, wholeWord) : 0;
   }

   @Override
   public Point getSelection()
   {
      return editor != null ?  editor.getSelection() : null;
   }

   @Override
   public String getSelectionText()
   {
      return editor != null ? editor.getSelectionText() : null;
   }

   @Override
   public boolean isEditable()
   {
      return editor != null ? editor.isEditable() : false;
   }

   @Override
   public void replaceSelection(String text)
   {
      if(editor != null)
         editor.replaceSelection(text);
   }
}
