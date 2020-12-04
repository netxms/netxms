/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.CreatePolicyDialog;
import org.netxms.ui.eclipse.datacollection.views.helpers.PolicyComparator;
import org.netxms.ui.eclipse.datacollection.views.helpers.PolicyFilter;
import org.netxms.ui.eclipse.datacollection.views.helpers.PolicyLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Editor for the policies
 */
public class PolicyListView extends ViewPart implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.datacollection.views.policy_list"; //$NON-NLS-1$
   public static final String JOB_FAMILY = "PolicyEditorJob"; //$NON-NLS-1$

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_GUID = 2;
   
   private Display display;
   private NXCSession session;
   private long templateId;
   private String templateName;
   private HashMap<UUID, AgentPolicy> policies = null;
   private PolicyFilter filter;
   private FilterText filterText;
   private IDialogSettings settings;
   private SortableTableViewer policyList;

   private Action actionRefresh;
   private Action actionCreate;
   private Action actionDelete;
   private Action actionEdit;
   private Action actionRename;
   private Action actionCopy;
   private Action actionMove;
   private Action actionDuplicate;
   private Action actionShowFilter;
   private Composite content;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
      settings = Activator.getDefault().getDialogSettings();
      AbstractObject obj = session.findObjectById(Long.parseLong(site.getSecondaryId()));
      if(obj != null)
      {
         templateId = obj.getObjectId();
         templateName = obj.getObjectName();
         setPartName(String.format("Agent Policies - %s", templateName));
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      display = parent.getDisplay();
      
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FormLayout());
      
      // Create filter area
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      
      final String[] names = { "Name", "Type", "GUID" };
      final int[] widths = { 250, 200, 100 };
      
      policyList = new SortableTableViewer(content, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      policyList.setContentProvider(new ArrayContentProvider());
      policyList.setLabelProvider(new PolicyLabelProvider());
      policyList.setComparator(new PolicyComparator());
      filter = new PolicyFilter();
      policyList.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(policyList, settings, "PolicyEditorView"); //$NON-NLS-1$
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
            WidgetHelper.saveTableViewerSettings(policyList, settings, "DataCollectionEditor"); //$NON-NLS-1$
         }
      });
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      policyList.getTable().setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createActions();
      contributeToActionBars();
      createPopupMenu();  

      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });

      // Set initial focus to filter input line
      if (actionShowFilter.isChecked())
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      session.addListener(this);
      refresh();
   }

   /**
    * Full refresh
    */
   private void refresh()
   {      
      ConsoleJob job = new ConsoleJob("Save agent policy", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final HashMap<UUID, AgentPolicy> serverPolicies = session.getAgentPolicyList(templateId);
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
            return "Can not load policy list";
         }
         
      };
      job.start();
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };

      actionCreate = new Action("Create new", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewPolicy();
         }
      }; 

      actionRename = new Action("Rename policy") {
         @Override
         public void run()
         {
            renamePolicy();
         }
      };

      actionEdit = new Action("Edit", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editPolicy();
         }
      };
      actionEdit.setImageDescriptor(Activator.getImageDescriptor("icons/edit.png")); //$NON-NLS-1$
      actionEdit.setEnabled(false);

      actionCopy = new Action("Copy to another template(s)...") {
         @Override
         public void run()
         {
            copyItems(false);
         }
      };
      actionCopy.setEnabled(false);

      actionMove = new Action("Move to another template(s)...") {
         @Override
         public void run()
         {
            copyItems(true);
         }
      };
      actionMove.setEnabled(false);

      actionDuplicate = new Action("Duplicate") {
         @Override
         public void run()
         {
            duplicateItems();
         }
      };
      actionDuplicate.setEnabled(false);
      
      actionDelete = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deletePolicy();
         }
      };
      actionDelete.setEnabled(false);
      
      actionShowFilter = new Action(Messages.get().DataCollectionEditor_ShowFilter, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(getBooleanFromSettings("DataCollectionEditor.showFilter", true));
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
   }
  
   /**
    * Rename selected policy
    */
   protected void renamePolicy()
   {
      IStructuredSelection selection = (IStructuredSelection)policyList.getSelection();
      if (selection.size() != 1)
         return;
      
      AgentPolicy policy = (AgentPolicy)selection.getFirstElement();

      CreatePolicyDialog dlg = new CreatePolicyDialog(getViewSite().getShell(), policy);  
      if (dlg.open() != Window.OK)
         return;

      final AgentPolicy newPolicy = dlg.getPolicy();
      
      new ConsoleJob("Rename policy", this, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.savePolicy(templateId, newPolicy, false);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot rename policy";
         }
      }.start();
      
   }

   protected void copyItems(boolean doMove)
   {
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), ObjectSelectionDialog.createTemplateSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      IStructuredSelection selection = (IStructuredSelection)policyList.getSelection();
      Iterator<?> it = selection.iterator();
      final AgentPolicy[] policyList = new AgentPolicy[selection.size()];
      for(int i = 0; (i < policyList.length) && it.hasNext(); i++)
         policyList[i] = (AgentPolicy)it.next();
      
      new ConsoleJob("Copy policy of template: " + templateName, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(AbstractObject o : dlg.getSelectedObjects(Template.class))
            {
               for(AgentPolicy p : policyList)
                  session.savePolicy(o.getObjectId(), new AgentPolicy(p), true);
            }
            if (doMove)
            {
               for(AgentPolicy p : policyList)
                  session.deletePolicy(templateId, ((AgentPolicy)p).getGuid()); 
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Error on policy item manipulcation for template: " + templateName;
         }
      }.start();      
   }

   protected void duplicateItems()
   {
      IStructuredSelection selection = (IStructuredSelection)policyList.getSelection();
      Iterator<?> it = selection.iterator();
      final AgentPolicy[] policyList = new AgentPolicy[selection.size()];
      for(int i = 0; (i < policyList.length) && it.hasNext(); i++)
         policyList[i] = new AgentPolicy((AgentPolicy)it.next());
      
      new ConsoleJob("Duplicate policy items for template: " + templateName, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(AgentPolicy p : policyList)
               session.savePolicy(templateId, p, true);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Error duplicationg policies on template: " + templateName;
         }
      }.start();
      
   }

   private void editPolicy()
   {
      IStructuredSelection selection = (IStructuredSelection)policyList.getSelection();
      AgentPolicy policy = (AgentPolicy)selection.getFirstElement();
      try
      {
         PolicyEditorView view = (PolicyEditorView)getSite().getPage().showView(PolicyEditorView.ID, policy.getGuid().toString() + Long.toString(templateId), IWorkbenchPage.VIEW_ACTIVATE);
         if (view != null)
            view.setPolicy(policy.getGuid(), templateId);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(getSite().getWorkbenchWindow().getShell(), "Error editing policy", String.format("Error on policy edit: %s", e.getMessage()));
      }
      
   }
   
   /**
    * Delete policy
    */
   protected void deletePolicy()
   {      

      final IStructuredSelection selection = (IStructuredSelection)policyList.getSelection();
      if (selection.size() <= 0)
         return;
      
      if (!MessageDialogHelper.openConfirm(getSite().getShell(), "Delete policy",
                                     "Do you really want to delete selected policies?"))
         return;
      
      new ConsoleJob("DeletePolicy", this, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Object policy : selection.toList())
            {
               session.deletePolicy(templateId, ((AgentPolicy)policy).getGuid()); 
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Can not delete policy";
         }
      }.start();
   }

   /**
    * Create new policy
    */
   protected void createNewPolicy()
   {
      CreatePolicyDialog dlg = new CreatePolicyDialog(getViewSite().getShell(), null);  
      if (dlg.open() != Window.OK)
         return;
      
      final AgentPolicy newPolicy = dlg.getPolicy();
      new ConsoleJob("Save policy", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final UUID newObjectGuid = session.savePolicy(templateId, newPolicy, false);
            runInUIThread(new Runnable() {
               
               @Override
               public void run()
               {
                  try
                  {
                     PolicyEditorView view = (PolicyEditorView)getSite().getPage().showView(PolicyEditorView.ID, newObjectGuid.toString() + Long.toString(templateId), IWorkbenchPage.VIEW_ACTIVATE);
                     if (view != null)
                        view.setPolicy(newObjectGuid, templateId);
                  }
                  catch(PartInitException e)
                  {
                     MessageDialogHelper.openError(getSite().getWorkbenchWindow().getShell(), "Error editing policy", String.format("Error on policy edit: %s", e.getMessage()));
                  }
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot update policy";
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
            mgr.add(actionMove);
            mgr.add(actionCopy);
            mgr.add(actionDuplicate);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(policyList.getControl());
      policyList.getControl().setMenu(menu);

      // Register menu for extension.
      getSite().setSelectionProvider(policyList);
      getSite().registerContextMenu(menuMgr, policyList);
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
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionShowFilter);
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
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionRefresh);
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
            if (n.getSubCode() != templateId)
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
            if (n.getSubCode() != templateId)
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

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      ConsoleJob job = new ConsoleJob("Update policy on editor close", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.onPolicyEditorClose(templateId);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot update policy on editor close";
         }
      };
      job.setUser(false);
      job.start();
      super.dispose();
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterText.setVisible(enable);
      FormData fd = (FormData)policyList.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
      settings.put("DataCollectionEditor.showFilter", enable);
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      policyList.refresh(false);
   }

   /**
    * @param b
    * @param defval
    * @return
    */
   private boolean getBooleanFromSettings(String name, boolean defval)
   {
      String v = settings.get(name);
      return (v != null) ? Boolean.valueOf(v) : defval;
   }

   @Override
   public void setFocus()
   {
   }
}
