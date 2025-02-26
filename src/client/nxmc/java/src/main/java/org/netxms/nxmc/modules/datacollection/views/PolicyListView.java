/**
 * NetXMS - open source network management system
 * Copyright (C) 2019-2024 Raden Solutions
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

import java.util.HashMap;
import java.util.Iterator;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.LocalChangeListener;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.CreatePolicyDialog;
import org.netxms.nxmc.modules.datacollection.views.helpers.PolicyComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.PolicyFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.PolicyLabelProvider;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Editor for the policies
 */
public class PolicyListView extends ObjectView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(PolicyListView.class);
   public static final String JOB_FAMILY = "PolicyEditorJob"; 

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_GUID = 2;
   
   private Display display;
   private NXCSession session;
   private HashMap<UUID, AgentPolicy> policies = null;
   private PolicyFilter filter;
   private SortableTableViewer policyList;

   private Action actionCreate;
   private Action actionDelete;
   private Action actionEdit;
   private Action actionRename;
   private Action actionCopy;
   private Action actionMove;
   private Action actionDuplicate;
   private Action actionForceDeploy;

   /**
    * Constructor
    */
   public PolicyListView() 
   {
      super(LocalizationHelper.getI18n(PolicyListView.class).tr("Agent Policies"), ResourceManager.getImageDescriptor("icons/object-views/policy.gif"), "AgentPolicies", true); 
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {      
      display = getWindow().getShell().getDisplay();
      
      final String[] names = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("GUID") };
      final int[] widths = { 250, 200, 100 };
      
      policyList = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      policyList.setContentProvider(new ArrayContentProvider());
      policyList.setLabelProvider(new PolicyLabelProvider());
      policyList.setComparator(new PolicyComparator());
      filter = new PolicyFilter();
      policyList.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(policyList, "PolicyEditorView"); //$NON-NLS-1$
      policyList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {

            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionRename.setEnabled(selection.size() == 1);
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
               actionCopy.setEnabled(selection.size() > 0);
               actionMove.setEnabled(selection.size() > 0);
               actionDuplicate.setEnabled(selection.size() > 0);
            }
         }
      });
      policyList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });
      policyList.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(policyList, "DataCollectionEditor"); //$NON-NLS-1$
         }
      });

      createActions();
      createPopupMenu();  
      
      session.addListener(this);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      applyPolicy();
      refresh();
   } 

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {      
      if (!isActive())
         return;

      Job job = new Job("Save agent policy", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final HashMap<UUID, AgentPolicy> serverPolicies = session.getAgentPolicyList(getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  policies = serverPolicies;
                  policyList.setInput(policies.values().toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Can't load policy list");
         }
         
      };
      job.start();
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionCreate = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewPolicy();
         }
      }; 

      actionRename = new Action(i18n.tr("&Rename")) {
         @Override
         public void run()
         {
            renamePolicy();
         }
      };

      actionEdit = new Action(i18n.tr("&Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editPolicy();
         }
      };
      actionEdit.setImageDescriptor(ResourceManager.getImageDescriptor("icons/edit.png")); //$NON-NLS-1$
      actionEdit.setEnabled(false);

      actionCopy = new Action(i18n.tr("&Copy to another template(s)...")) {
         @Override
         public void run()
         {
            copyPolicies(false);
         }
      };
      actionCopy.setEnabled(false);

      actionMove = new Action(i18n.tr("&Move to another template(s)...")) {
         @Override
         public void run()
         {
            copyPolicies(true);
         }
      };
      actionMove.setEnabled(false);

      actionDuplicate = new Action(i18n.tr("D&uplicate")) {
         @Override
         public void run()
         {
            duplicatePolicies();
         }
      };
      actionDuplicate.setEnabled(false);
      
      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deletePolicy();
         }
      };
      actionDelete.setEnabled(false);
      
      actionForceDeploy = new Action(i18n.tr("&Force deploy"), ResourceManager.getImageDescriptor("icons/push.png")) {
         @Override
         public void run()
         {
            forceDeploy();
         }
      };
      actionForceDeploy.setEnabled(false);
   }

   /**
    * Rename selected policy
    */
   protected void renamePolicy()
   {
      IStructuredSelection selection = policyList.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      AgentPolicy policy = (AgentPolicy)selection.getFirstElement();
      CreatePolicyDialog dlg = new CreatePolicyDialog(getWindow().getShell(), policy);  
      if (dlg.open() != Window.OK)
         return;

      final AgentPolicy newPolicy = dlg.getPolicy();
      new Job(i18n.tr("Rename policy"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.savePolicy(getObjectId(), newPolicy, false);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot rename policy");
         }
      }.start();
   }

   /**
    * Copy/move policies
    *
    * @param doMove true to move instead of copy
    */
   protected void copyPolicies(boolean doMove)
   {
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), ObjectSelectionDialog.createTemplateSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      if (dlg.getSelectedObjects(Template.class).length <= 0)
      {
         MessageDialogHelper.openError(getWindow().getShell(), i18n.tr("Error"), i18n.tr("Please select at least one destination template."));
         return;
      }

      IStructuredSelection selection = policyList.getStructuredSelection();
      Iterator<?> it = selection.iterator();
      final AgentPolicy[] policyList = new AgentPolicy[selection.size()];
      for(int i = 0; (i < policyList.length) && it.hasNext(); i++)
         policyList[i] = (AgentPolicy)it.next();
      
      new Job(i18n.tr("Copy policy of template: {0}", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            boolean sameObject = false;
            for(AbstractObject o : dlg.getSelectedObjects(Template.class))
            {
               if (o.getObjectId() != getObjectId() || !doMove)
                  for(AgentPolicy p : policyList)
                     session.savePolicy(o.getObjectId(), new AgentPolicy(p), true);
               if (o.getObjectId() == getObjectId())
                  sameObject = true;
            }
            if (doMove)
            {
               for(AgentPolicy p : policyList)
                  session.deletePolicy(getObjectId(), ((AgentPolicy)p).getGuid()); 
            }
            
            if (doMove || sameObject)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     showInformationMessage();
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error on policy item manipulcation for template: {0}", getObjectId());
         }
      }.start();      
   }

   /**
    * Duplicate policies
    */
   protected void duplicatePolicies()
   {
      IStructuredSelection selection = policyList.getStructuredSelection();
      Iterator<?> it = selection.iterator();
      final AgentPolicy[] policyList = new AgentPolicy[selection.size()];
      for(int i = 0; (i < policyList.length) && it.hasNext(); i++)
         policyList[i] = new AgentPolicy((AgentPolicy)it.next());
      
      new Job(i18n.tr("Duplicate policy items for template: {0}", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AgentPolicy p : policyList)
               session.savePolicy(getObjectId(), p, true);
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  showInformationMessage();
               }
            });
            
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error duplicationg policies on template: ", getObjectName());
         }
      }.start();
      
   }

   /**
    * Edit selected policy
    */
   private void editPolicy()
   {
      IStructuredSelection selection = policyList.getStructuredSelection();
      AgentPolicy policy = (AgentPolicy)selection.getFirstElement();
      openView(new PolicyEditorView(policy.getGuid(), getObjectId(), new LocalChangeListener() {
         
         @Override
         public void onObjectChange()
         {
            showInformationMessage();
         }
      }));
      
   }
   
   /**
    * Delete policy
    */
   protected void deletePolicy()
   {      
      IStructuredSelection selection = policyList.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Delete policy"), i18n.tr("Do you really want to delete selected policies?")))
         return;

      new Job(i18n.tr("DeletePolicy"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object policy : selection.toList())
            {
               session.deletePolicy(getObjectId(), ((AgentPolicy)policy).getGuid()); 
            }
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  showInformationMessage();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Can not delete policy");
         }
      }.start();
   }

   /**
    * Create new policy
    */
   protected void createNewPolicy()
   {
      CreatePolicyDialog dlg = new CreatePolicyDialog(getWindow().getShell(), null);  
      if (dlg.open() != Window.OK)
         return;

      final AgentPolicy newPolicy = dlg.getPolicy();
      new Job(i18n.tr("Save policy"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final UUID newObjectGuid = session.savePolicy(getObjectId(), newPolicy, false);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  openView(new PolicyEditorView(newObjectGuid, getObjectId(), new LocalChangeListener() {
                     
                     @Override
                     public void onObjectChange()
                     {
                        showInformationMessage();
                     }
                  }));
                  showInformationMessage();
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update policy");
         }
      }.start();
   }
   
   /**
    * Force deploy policies
    */
   private void forceDeploy()
   {     
      final NXCSession session = Registry.getSession();     
      new Job(i18n.tr("Running forced agent policy deployment for template \"{0}\"", getObjectName()), null, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.forcePolicyInstallation(getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  clearMessages();
                  addMessage(MessageArea.SUCCESS, i18n.tr("Policies from template \"{0}\" successfully installed", getObjectName()));
               }
            });               
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Unable to force install policies from template \"{0}\"", getObjectName());
         }
      }.start();
   }

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            mgr.add(actionEdit);
            mgr.add(actionCreate);
            mgr.add(actionDelete);
            mgr.add(actionRename);
            mgr.add(new Separator());
            mgr.add(actionMove);
            mgr.add(actionCopy);
            mgr.add(actionDuplicate);
            mgr.add(new Separator());
            mgr.add(actionForceDeploy);  
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(policyList.getControl());
      policyList.getControl().setMenu(menu);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionForceDeploy); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(actionForceDeploy);      
      manager.add(new Separator());
   }

   /* (non-Javadoc)
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.POLICY_MODIFIED: 
            if (n.getSubCode() != getObjectId())
               return;

            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {     
                  if (policyList.getControl().isDisposed())
                     return;

                  AgentPolicy policy = policies.get(((AgentPolicy)n.getObject()).getGuid());
                  if (policy != null)
                  {
                     policy.update((AgentPolicy)n.getObject());
                     policyList.update(policy, null);
                  }
                  else
                  {
                     policies.put(((AgentPolicy)n.getObject()).getGuid(), (AgentPolicy)n.getObject());
                     policyList.setInput(policies.values().toArray());
                  }
               }
            });
            break;
         case SessionNotification.POLICY_DELETED:
            if (n.getSubCode() != getObjectId())
               return;

            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  if (policyList.getControl().isDisposed())
                     return;

                  policies.remove((UUID)n.getObject());
                  policyList.setInput(policies.values().toArray());
               }
            });
            break;
      }
   }
   
   /**
    * Apply policy on view dispose or on selected object change
    */
   private void applyPolicy()
   {
      if (getObjectId() == 0)
         return;
      
      Job job = new Job(i18n.tr("Update policy on editor close"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.onPolicyEditorClose(getObjectId());
            runInUIThread(new Runnable() {               
               @Override
               public void run()
               {
                  clearMessages();
                  actionForceDeploy.setEnabled(false);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update policy on editor close");
         }
      };
      job.setUser(false);
      job.start();      
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      applyPolicy();
      super.dispose();
   }
   
   /**
    * Display message with information about policy deploy
    */
   public void showInformationMessage()
   {
      addMessage(MessageArea.INFORMATION, i18n.tr("Changes in policies have been made. Please deploy them to nodes"), false, "Deploy policies", new Runnable() {
         
         @Override
         public void run()
         {
            forceDeploy();            
         }
      });  
      actionForceDeploy.setEnabled(true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Template); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 40;
   }
}
