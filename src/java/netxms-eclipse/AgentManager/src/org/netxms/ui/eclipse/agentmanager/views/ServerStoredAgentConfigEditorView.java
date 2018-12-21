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
package org.netxms.ui.eclipse.agentmanager.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
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
import org.netxms.client.agent.config.ConfigContent;
import org.netxms.client.agent.config.ConfigListElement;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.agentmanager.dialogs.SaveConfigDialog;
import org.netxms.ui.eclipse.agentmanager.dialogs.SaveStoredConfigDialog;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.AgentConfigEditor;

/**
 * Sored on server agent's configuration editor
 */
public class ServerStoredAgentConfigEditorView extends ViewPart implements ISaveablePart2
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.ServerStoredAgentConfigEditorView"; //$NON-NLS-1$

   private NXCSession session;
   private boolean modified = false;
   private boolean reselection = false;

   private ScrolledForm form;
   private AgentConfigEditor configEditor;
   private ScriptEditor filterEditor;
   private Text nameField;
   private TableViewer configList;
   private RefreshAction actionRefresh;
   private Action actionSave;
   private Action actionCreate;
   private Action actionDelete;
   private Action actionMoveUp;
   private Action actionMoveDown;
   private List<ConfigListElement> elements;
   private ConfigContent content;
   private IStructuredSelection previousSelection;

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = (NXCSession)ConsoleSharedData.getSession();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
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

      configList = new TableViewer(listContainer, SWT.FULL_SELECTION | SWT.SINGLE);
      configList.setContentProvider(new ArrayContentProvider());
      configList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            onSelectionChange(event);
         }
      });
      configList.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      new Label(listContainer, SWT.SEPARATOR | SWT.VERTICAL).setLayoutData(new GridData(SWT.RIGHT, SWT.FILL, false, true));

      /**** editor section ****/
      FormToolkit toolkit = new FormToolkit(getSite().getShell().getDisplay());

      Composite formContainer = new Composite(splitter, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      layout.horizontalSpacing = 0;
      formContainer.setLayout(layout);      

      new Label(formContainer, SWT.SEPARATOR | SWT.VERTICAL).setLayoutData(new GridData(SWT.RIGHT, SWT.FILL, false, true));
      
      form = toolkit.createScrolledForm(formContainer);
      form.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      form.setText(Messages.get().ServerStoredAgentConfigEditorView_Noname);
      
      layout = new GridLayout();
      form.getBody().setLayout(layout);
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      form.getBody().setLayoutData(gridData);   

      splitter.setWeights(new int[] { 20, 80 });       
      
      Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      section.setText(Messages.get().ServerStoredAgentConfigEditorView_Name);
      layout = new GridLayout();
      section.setLayout(layout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      section.setLayoutData(gridData);
      
      /**** Name ****/
      nameField = new Text(section, SWT.BORDER);
      section.setClient(nameField);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      nameField.setLayoutData(gridData);
      nameField.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });

      /**** Filter ****/
      section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      section.setText(Messages.get().ServerStoredAgentConfigEditorView_Filter);
      section.setLayout(layout);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      section.setLayoutData(gridData);
      
      filterEditor = new ScriptEditor(section, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true, 
            "Variables:\r\n\t$1\tIP address\r\n\t$2\tplatform name\r\n\t$3\tmajor agent version number\r\n\t$4\tminor agent version number\r\n\t$5\trelease number\r\n\r\nReturn value: true if this configuration should be sent to agent");
      section.setClient(filterEditor);
      filterEditor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      filterEditor.setLayoutData(gridData);

      /**** Config ****/
      section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      section.setText(Messages.get().ServerStoredAgentConfigEditorView_ConfigFile);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      section.setLayoutData(gridData);
      
      configEditor = new AgentConfigEditor(section, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, 0);
      section.setClient(configEditor);
      configEditor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      configEditor.setLayoutData(gridData);

      createActions();
      contributeToActionBars();
      createPopupMenu();
      
      actionSave.setEnabled(false);
      getConfigList();
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
         SaveStoredConfigDialog dlg = new SaveStoredConfigDialog(getSite().getShell());
         int rc = dlg.open();
         if (rc == SaveStoredConfigDialog.SAVE_ID)
         {
            intermediateSave();
            modified = false;
            firePropertyChange(PROP_DIRTY);
            actionSave.setEnabled(false);
            return;
         }
         if (rc == SaveStoredConfigDialog.CANCEL)
         {
            reselection = true;
            configList.setSelection(previousSelection);
            return;
         }
      }

      IStructuredSelection selection = (IStructuredSelection)event.getSelection();
      previousSelection = selection;
      if (selection == null)
         return;

      final ConfigListElement element = (ConfigListElement)selection.getFirstElement();

      new ConsoleJob(Messages.get().ServerStoredAgentConfigEditorView_JobTitle_GetContent, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            if (element != null)
            {
               content = session.getConfigContent(element.getId());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     updateContent();
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ServerStoredAgentConfigEditorView_JobError_GetContent;
         }
      }.start();
   }

   /**
    * 
    */
   private void onTextModify()
   {
      if (!modified)
      {
         modified = true;
         firePropertyChange(PROP_DIRTY);
         actionSave.setEnabled(true);
      }
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
            mgr.add(actionMoveUp);
            mgr.add(actionMoveDown);
            mgr.add(new Separator());
            mgr.add(actionDelete);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(configList.getControl());
      configList.getControl().setMenu(menu);

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, configList);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      nameField.setFocus();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            getConfigList();
         }
      };

      actionSave = new Action(Messages.get().AgentConfigEditorView_Save, SharedIcons.SAVE) {
         @Override
         public void run()
         {
            intermediateSave();
            actionSave.setEnabled(false);
            modified = false;
         }
      };

      actionCreate = new Action(Messages.get().ServerStoredAgentConfigEditorView_JobTitle_CreateNew, SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewConfig();
         }
      };

      actionDelete = new Action(Messages.get().ServerStoredAgentConfigEditorView_Delete, SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteConfig();
         }
      };

      actionMoveUp = new Action(Messages.get().ServerStoredAgentConfigEditorView_MoveUp, SharedIcons.UP) {
         @Override
         public void run()
         {
            moveUp();
         }
      };

      actionMoveDown = new Action(Messages.get().ServerStoredAgentConfigEditorView_MoveDown, SharedIcons.DOWN) {
         @Override
         public void run()
         {
            moveDown();
         }
      };
   }

   /**
    * Move selected item up
    */
   private void moveUp()
   {
      IStructuredSelection selection = (IStructuredSelection)configList.getSelection();
      if (selection == null)
         return;

      final ConfigListElement element1 = (ConfigListElement)selection.getFirstElement();

      int index = elements.indexOf(element1);
      if (index <= 0)
         return;

      final ConfigListElement element2 = elements.get(index - 1);

      new ConsoleJob(Messages.get().ServerStoredAgentConfigEditorView_JobMoveUp, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.swapAgentConfigs(element1.getId(), element2.getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  getConfigList();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ServerStoredAgentConfigEditorView_JobError_MoveUp;
         }
      }.start();
   }

   /**
    * Move selected item down
    */
   private void moveDown()
   {
      IStructuredSelection selection = (IStructuredSelection)configList.getSelection();
      if (selection == null)
         return;

      final ConfigListElement element1 = (ConfigListElement)selection.getFirstElement();

      int index = elements.indexOf(element1);
      if (index >= (elements.size() - 1))
         return;

      final ConfigListElement elemen2 = elements.get(index + 1);

      new ConsoleJob(Messages.get().ServerStoredAgentConfigEditorView_JobMoveDown, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.swapAgentConfigs(element1.getId(), elemen2.getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  getConfigList();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ServerStoredAgentConfigEditorView_JobError_MoveDown;
         }
      }.start();
   }

   /**
    * Delete selected configuration
    */
   private void deleteConfig()
   {
      IStructuredSelection selection = (IStructuredSelection)configList.getSelection();
      if (selection == null)
         return;

      final ConfigListElement element = (ConfigListElement)selection.getFirstElement();

      new ConsoleJob(Messages.get().ServerStoredAgentConfigEditorView_JobDelete, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.deleteAgentConfig(element.getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  elements.remove(element);
                  configList.setInput(elements.toArray(new ConfigListElement[elements.size()]));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ServerStoredAgentConfigEditorView_JobError_Delete;
         }
      }.start();
   }

   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Creates new config that is sent to server on save
    */
   private void createNewConfig()
   {
      modified = true;
      firePropertyChange(PROP_DIRTY);
      actionSave.setEnabled(true);

      ConfigListElement newElement = new ConfigListElement();
      elements.add(newElement);
      configList.setInput(elements.toArray(new ConfigListElement[elements.size()]));

      reselection = true;
      StructuredSelection selection = new StructuredSelection(newElement);
      previousSelection = selection;
      configList.setSelection(selection);
      content = new ConfigContent();
      updateContent();
   }

   /**
    * Update editor content
    */
   private void updateContent()
   {
      form.setText(content.getName());
      nameField.setText(content.getName());
      configEditor.setText(content.getConfig());
      filterEditor.setText(content.getFilter());
      modified = false;
      firePropertyChange(PROP_DIRTY);
      actionSave.setEnabled(false);
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionSave);
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      try
      {
         content.setConfig(configEditor.getText());
         content.setFilter(filterEditor.getText());
         content.setName(nameField.getText());
         session.saveAgentConfig(content);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getViewSite().getShell(), Messages.get().AgentConfigEditorView_Error,
               Messages.get().AgentConfigEditorView_SaveError + e.getMessage());
      }
   }

   /**
    * 
    */
   public void intermediateSave()
   {
      if (content == null)
         return;
      content.setConfig(configEditor.getText());
      content.setFilter(filterEditor.getText());
      content.setName(nameField.getText());

      new ConsoleJob(Messages.get().ServerStoredAgentConfigEditorView_JobSave, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.saveAgentConfig(content);

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  getConfigList();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ServerStoredAgentConfigEditorView_JobError_Save;
         }
      }.start();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return modified;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
   @Override
   public boolean isSaveOnCloseNeeded()
   {
      return modified;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart2#promptToSaveOnClose()
    */
   @Override
   public int promptToSaveOnClose()
   {
      SaveStoredConfigDialog dlg = new SaveStoredConfigDialog(getSite().getShell());
      int rc = dlg.open();
      modified = (rc != SaveStoredConfigDialog.SAVE_ID);
      return (rc == IDialogConstants.CANCEL_ID) ? CANCEL : ((rc == SaveConfigDialog.DISCARD_ID) ? NO : YES);
   }

   /**
    * Gets config list from server and sets editable fields to nothing
    */
   public void getConfigList()
   {
      IStructuredSelection selection = (IStructuredSelection)configList.getSelection();
      final ConfigListElement element = selection == null ? null : (ConfigListElement)selection.getFirstElement();

      new ConsoleJob(Messages.get().PackageManager_OpenDatabase, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            elements = session.getConfigList();

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  configList.setInput(elements.toArray(new ConfigListElement[elements.size()]));

                  if (element == null && elements.size() > 0)
                  {
                     StructuredSelection selection = new StructuredSelection(elements.get(0));
                     configList.setSelection(selection);
                  }

                  if (element != null)
                  {
                     long id = element.getId();
                     if (id == 0)
                        id = elements.get(elements.size() - 1).getId();
                     for(int i = 0; i < elements.size(); i++)
                     {
                        if (elements.get(i).getId() == id)
                        {
                           StructuredSelection selection = new StructuredSelection(elements.get(i));
                           configList.setSelection(selection);
                        }
                     }
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ServerStoredAgentConfigEditorView_JobError_GetList;
         }
      }.start();
   }
}
