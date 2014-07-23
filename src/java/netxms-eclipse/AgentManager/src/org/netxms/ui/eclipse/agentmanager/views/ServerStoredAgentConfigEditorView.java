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
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.agent.config.ConfigContent;
import org.netxms.client.agent.config.ConfigListElement;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.agentmanager.dialogs.SaveConfigDialog;
import org.netxms.ui.eclipse.agentmanager.dialogs.SaveStoredConfigDialog;
import org.netxms.ui.eclipse.agentmanager.widgets.AgentConfigEditor;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Sored on server agent's configuration editor
 */
public class ServerStoredAgentConfigEditorView extends ViewPart implements ISaveablePart2
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.ServerStoredAgentConfigEditorView"; //$NON-NLS-1$

   private NXCSession session;
   private boolean modified = false;
   private boolean reselection = false;

   private AgentConfigEditor configEditor;
   private ScriptEditor filterEditor;
   private Label nameLabel;
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
      GridLayout gridLayout = new GridLayout();
      parent.setLayout(gridLayout);

      SashForm splitter = new SashForm(parent, SWT.HORIZONTAL);
      splitter.setLayout(new FillLayout());
      splitter.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      configList = new TableViewer(splitter, SWT.FULL_SELECTION | SWT.SINGLE);
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      configList.getTable().setLayoutData(gridData);
      configList.setContentProvider(new ArrayContentProvider());

      configList.addSelectionChangedListener(new ISelectionChangedListener() {

         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            onSelectionChange(event);
         }
      });

      createPopupMenu();

      // edit part
      final Composite editGroup = new Composite(splitter, SWT.NONE);
      GridLayout layout = new GridLayout();
      editGroup.setLayout(layout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      editGroup.setLayoutData(gridData);

      final Composite nameGroup = new Composite(editGroup, SWT.BORDER);
      layout = new GridLayout();
      nameGroup.setLayout(layout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      nameGroup.setLayoutData(gridData);

      nameLabel = new Label(nameGroup, SWT.NONE);
      nameLabel.setText("Name");
      nameField = new Text(nameGroup, SWT.NONE);
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

      splitter = new SashForm(editGroup, SWT.VERTICAL);
      splitter.setLayout(new FillLayout());
      splitter.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      final WidgetFactory filterFactory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new ScriptEditor(parent, style, SWT.H_SCROLL | SWT.V_SCROLL);
         }
      };

      gridData = new GridData();
      gridData.horizontalSpan = 2;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      filterEditor = (ScriptEditor)WidgetHelper.createLabeledControl(splitter, SWT.BORDER, filterFactory, "Filter", gridData);
      filterEditor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });
      gridData = new GridData();
      gridData.horizontalSpan = 2;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      filterEditor.setLayoutData(gridData);

      final WidgetFactory configFactory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new AgentConfigEditor(parent, style, SWT.H_SCROLL | SWT.V_SCROLL);
         }
      };

      gridData = new GridData();
      gridData.horizontalSpan = 2;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      configEditor = (AgentConfigEditor)WidgetHelper.createLabeledControl(splitter, SWT.BORDER, configFactory, "Config", gridData);
      configEditor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });
      gridData = new GridData();
      gridData.horizontalSpan = 2;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      configEditor.setLayoutData(gridData);

      createActions();
      contributeToActionBars();
      actionSave.setEnabled(false);
      getConfigList();
   }

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
            intermidiateSave();
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

      new ConsoleJob(Messages.get().PackageManager_OpenDatabase, this, Activator.PLUGIN_ID, null) {
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
            return "Not possible to get config content";
         }
      }.start();
   }

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
            mgr.add(actionDelete);
            mgr.add(actionMoveUp);
            mgr.add(actionMoveDown);
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

      actionSave = new Action() {
         @Override
         public void run()
         {
            intermidiateSave();
            actionSave.setEnabled(false);
            modified = false;
         }
      };
      actionSave.setText(Messages.get().AgentConfigEditorView_Save);
      actionSave.setImageDescriptor(SharedIcons.SAVE);

      actionCreate = new Action() {
         @Override
         public void run()
         {
            createNewConfig();
         }
      };
      actionCreate.setText("Create new config");
      actionCreate.setImageDescriptor(SharedIcons.ADD_OBJECT);

      actionDelete = new Action() {
         @Override
         public void run()
         {
            deleteConfig();
         }
      };
      actionDelete.setText("Delete config");
      actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);

      actionMoveUp = new Action() {
         @Override
         public void run()
         {
            moveUp();
         }
      };
      actionMoveUp.setText("Move up");
      actionMoveUp.setImageDescriptor(SharedIcons.DELETE_OBJECT);

      actionMoveDown = new Action() {
         @Override
         public void run()
         {
            moveDown();
         }
      };
      actionMoveDown.setText("Move down");
      actionMoveDown.setImageDescriptor(SharedIcons.DELETE_OBJECT);
   }

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

      new ConsoleJob(Messages.get().PackageManager_OpenDatabase, this, Activator.PLUGIN_ID, null) {
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
            return "Not possible to move element up";
         }
      }.start();
   }

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

      new ConsoleJob(Messages.get().PackageManager_OpenDatabase, this, Activator.PLUGIN_ID, null) {
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
            return "Not possible to move element down";
         }
      }.start();
   }

   private void deleteConfig()
   {
      IStructuredSelection selection = (IStructuredSelection)configList.getSelection();
      if (selection == null)
         return;

      final ConfigListElement element = (ConfigListElement)selection.getFirstElement();

      new ConsoleJob(Messages.get().PackageManager_OpenDatabase, this, Activator.PLUGIN_ID, null) {
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
            return "Not possible to delete config list";
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

   private void updateContent()
   {
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

   public void intermidiateSave()
   {
      if (content == null)
         return;
      content.setConfig(configEditor.getText());
      content.setFilter(filterEditor.getText());
      content.setName(nameField.getText());

      new ConsoleJob(Messages.get().PackageManager_OpenDatabase, this, Activator.PLUGIN_ID, null) {
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
            return "Not possible to save config";
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
            return "Not possible to get config list";
         }
      }.start();
   }
}
