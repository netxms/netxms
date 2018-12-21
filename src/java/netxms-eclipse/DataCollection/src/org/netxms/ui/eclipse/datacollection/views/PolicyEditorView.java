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
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.dialogs.CreatePolicyDialog;
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
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Editor for the policies
 */
public class PolicyEditorView  extends ViewPart implements ISaveablePart2, SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.datacollection.views.policy_editor"; //$NON-NLS-1$
   
   protected NXCSession session;
   protected long templateId;
   protected HashMap<UUID, AgentPolicy> policies = null;
   protected boolean modified = false;
   protected boolean reselection = false;
   protected AgentPolicy currentlySelectedElement;
   private AgentPolicy savedObject;   
   protected IStructuredSelection previousSelection;
   protected boolean throwExceptionOnSave;
   protected Exception saveException;
   
   protected Action actionRefresh;
   protected Action actionSave;
   protected Action actionShowTechnicalInformation;
   protected Action actionCreate; 
   protected Action actionDelete; 
   protected TableViewer policyList;
   protected AbstractPolicyEditor editor = null;
   protected Text name;
   protected LabeledText guid;
   private FormToolkit toolkit;
   protected ScrolledForm form;
   private Section editorSection;
   private Composite editorArea;
   private Composite clientArea;
   
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
   public void setPolicy(Template templateObj)
   {
      templateId = templateObj.getObjectId();
      setPartName("Edit Policies for \""+ templateObj.getObjectName() +"\" template");
      refresh();
      modified = false;
      firePropertyChange(PROP_DIRTY);
      actionSave.setEnabled(false);
   }
   
   /**
    * Refresh on selection change
    */
   private void refreshOnSelectionChange()
   {
      if(currentlySelectedElement != null)
      {
         if(actionShowTechnicalInformation.isChecked())
         {
            if(guid == null)
            {
               guid = new LabeledText(clientArea, SWT.NONE);
               guid.setEditable(false);
               guid.setLabel("GUID");
               guid.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
            }
            guid.setText(currentlySelectedElement.getGuid().toString());
         }
         else if(guid != null)
         {
            guid.dispose();
         }
         
         name.setText(currentlySelectedElement.getName());
         
         if (editor != null)
         {
            editor.dispose();
            editor = null;
         }
         
         if(currentlySelectedElement.getPolicyType().equals(AgentPolicy.agent))
         {
            editor = new AgentConfigPolicyEditor(editorArea, SWT.NONE, currentlySelectedElement);            
         }
         else if (currentlySelectedElement.getPolicyType().equals(AgentPolicy.logParser))
         {
            editor = new LogParserPolicyEditor(editorArea, SWT.NONE, currentlySelectedElement);               
         }
         else if (currentlySelectedElement.getPolicyType().equals(AgentPolicy.supportApplication))
         {
            editor = new SupportAppPolicyEditor(editorArea, SWT.NONE, currentlySelectedElement);              
         }
         else
         {
            editor = new GenericPolicyEditor(editorSection, SWT.NONE, currentlySelectedElement);
         }
         toolkit.adapt(editor);
         editor.addModifyListener(new PolicyModifyListener() {
            @Override
            public void modifyParser()
            {
               setModified();
            }
         });
         form.layout(true, true);
         editor.setFocus();
      }
   }

   /**
    * Full refresh
    */
   private void refresh()
   {      
      ConsoleJob job = new ConsoleJob("Save agent policy", this, Activator.PLUGIN_ID, null) 
      {

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            policies = session.getAgentPolicyList(templateId);
            runInUIThread(new Runnable() {
               
               @Override
               public void run()
               {
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
      
      refreshOnSelectionChange();
   }

   @Override
   public void createPartControl(Composite parent)
   {
      parent.setLayout(new FillLayout());

      SashForm splitter = new SashForm(parent, SWT.HORIZONTAL);
      splitter.setSashWidth(3);
      
      Composite listContainer = new Composite(splitter, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      layout.horizontalSpacing = 0;
      listContainer.setLayout(layout);

      policyList = new TableViewer(listContainer, SWT.FULL_SELECTION | SWT.SINGLE);
      policyList.setContentProvider(new ArrayContentProvider());
      policyList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            onSelectionChange(event);
         }
      });
      policyList.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      new Label(listContainer, SWT.SEPARATOR | SWT.VERTICAL).setLayoutData(new GridData(SWT.RIGHT, SWT.FILL, false, true));

      toolkit = new FormToolkit(getSite().getShell().getDisplay());

      form = toolkit.createScrolledForm(splitter);
      form.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      layout = new GridLayout();
      form.getBody().setLayout(layout);
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      form.getBody().setLayoutData(gridData);   

      splitter.setWeights(new int[] { 20, 80 });       
      
      Section detailsSection = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      detailsSection.setText("Name");
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      detailsSection.setLayoutData(gridData);     

      clientArea = toolkit.createComposite(detailsSection);
      layout = new GridLayout();
      layout.makeColumnsEqualWidth = true;
      clientArea.setLayout(layout);
      detailsSection.setClient(clientArea);
      
      name = new Text(clientArea, SWT.BORDER);
      name.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      name.addKeyListener(new KeyListener() {
         
         @Override
         public void keyReleased(KeyEvent e)
         {
         }
         
         @Override
         public void keyPressed(KeyEvent e)
         {
            setModified();
         }
      });
      
      editorSection = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      editorSection.setText("Content");
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.grabExcessVerticalSpace = true;
      editorSection.setLayoutData(gridData);
      
      editorArea = toolkit.createComposite(editorSection);
      editorArea.setLayout(new FillLayout());
      editorSection.setClient(editorArea);
      
      createActions();
      contributeToActionBars();
      createPopupMenu(); 
      
      session.addListener(this);
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
      
      actionShowTechnicalInformation = new Action("Show technical details", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            refreshOnSelectionChange();
         }
      };
      actionShowTechnicalInformation.setChecked(false);

      actionCreate = new Action("Create new", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewPolicy();
         }
      };

      actionDelete = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deletePolicy();
         }
      };
   }
   
   protected void deletePolicy()
   {
      new ConsoleJob("DeletePolicy", this, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.deletePolicy(templateId, currentlySelectedElement.getGuid()); 
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Can not delete policy";
         }
      }.start();
   }

   protected void createNewPolicy()
   {
      CreatePolicyDialog dlg = new CreatePolicyDialog(getViewSite().getShell());  
      int rc = dlg.open();
      if(rc != dlg.OK)
         return;
      
      final AgentPolicy newPolicy = dlg.getPolicy();
      
      new ConsoleJob("Save policy", this, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final UUID newObjectGuid = session.savePolicy(templateId, newPolicy);  
            newPolicy.setGuid(newObjectGuid);
            
            runInUIThread(new Runnable() {
               
               @Override
               public void run()
               {
                  AgentPolicy policy = policies.get(newObjectGuid);
                  if(policy != null)
                  {
                     currentlySelectedElement = policy; 
                     StructuredSelection selection = new StructuredSelection(policy);
                     policyList.setSelection(selection);
                  }
                  else
                  {
                     policies.put(newObjectGuid, newPolicy);                     
                     policyList.setInput(policies.values().toArray());
                     currentlySelectedElement = newPolicy; 
                     StructuredSelection selection = new StructuredSelection(newPolicy);
                     policyList.setSelection(selection);
                  }
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Can not update policy";
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
            mgr.add(actionDelete);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(policyList.getControl());
      policyList.getControl().setMenu(menu);

      // Register menu for extension.
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
      manager.add(actionShowTechnicalInformation);
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
      manager.add(actionCreate);
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * @param event
    */
   private void onSelectionChange(SelectionChangedEvent event)
   {
      if (reselection)
      {
         reselection = false;
         return;
      }

      if (modified)
      {
         SavePolicyDialog dlg = new SavePolicyDialog(getSite().getShell());
         int rc = dlg.open();
         if (rc == SavePolicyDialog.SAVE_ID || rc == SavePolicyDialog.DISCARD_ID)
         {
            if(rc == SavePolicyDialog.SAVE_ID)
               save();
            modified = false;
            firePropertyChange(PROP_DIRTY);
            actionSave.setEnabled(false);
         }
         if (rc == SavePolicyDialog.CANCEL)
         {
            reselection = true;
            policyList.setSelection(previousSelection);
            return;
         }
      }

      IStructuredSelection selection = (IStructuredSelection)event.getSelection();
      previousSelection = selection;
      if (selection == null)
         return;

      currentlySelectedElement = (AgentPolicy)selection.getFirstElement();

      refreshOnSelectionChange();
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

   private void save()
   {
      throwExceptionOnSave = true;
      currentlySelectedElement = editor.getUpdatedPolicy();
      currentlySelectedElement.setName(name.getText());
      savedObject = currentlySelectedElement;
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

   @Override
   public void setFocus()
   {
      if(editor != null)
         editor.setFocus();
   }

   @Override
   public void doSave(IProgressMonitor monitor)
   {      
      try
      {
         session.savePolicy(templateId, savedObject);
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

   @Override
   public int promptToSaveOnClose()
   {
      SavePolicyDialog dlg = new SavePolicyDialog(getSite().getShell());
      int rc = dlg.open();
      if (rc == SavePolicyDialog.SAVE_ID)
      {
         modified = false;
         return YES;
      }
      if (rc == SavePolicyDialog.CANCEL)
      {
         return CANCEL;
      }
      return NO;
   }

   @Override
   public void notificationHandler(final SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.POLICY_MODIFIED: 
            if(n.getSubCode() != templateId)
               return;
            policyList.getTable().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {     
                  AgentPolicy policy = policies.get(((AgentPolicy)n.getObject()).getGuid());
                  if(policy != null)
                  {
                     policy.update((AgentPolicy)n.getObject());
                     policyList.update(policy, null);
                  }
                  else
                  {
                     policies.put(((AgentPolicy)n.getObject()).getGuid(), (AgentPolicy)n.getObject());
                     policyList.setInput(policies);
                  }
               }
            });
            break;
         case SessionNotification.POLICY_DELETED:
            if(n.getSubCode() != templateId)
               return;
            policyList.getTable().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  policies.remove((UUID)n.getObject());
                  policyList.setInput(policies.values().toArray());
                  refreshOnSelectionChange();
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
   
   
}
