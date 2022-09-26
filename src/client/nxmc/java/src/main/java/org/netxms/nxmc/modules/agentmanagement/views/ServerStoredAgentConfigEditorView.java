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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.util.List;
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
import org.eclipse.jface.viewers.LabelProvider;
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
import org.netxms.client.NXCSession;
import org.netxms.client.agent.config.AgentConfiguration;
import org.netxms.client.agent.config.AgentConfigurationHandle;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.widgets.AgentConfigEditor;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;
/**
 * Editor for server side agent configurations
 */
public class ServerStoredAgentConfigEditorView extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ServerStoredAgentConfigEditorView.class);
   public static final String ID = "ServerStoredAgentConfigEditorView"; //$NON-NLS-1$

   private NXCSession session;
   private boolean modified = false;
   private boolean reselection = false;

   private Composite editorContent;
   private AgentConfigEditor contentEditor;
   private ScriptEditor filterEditor;
   private LabeledText nameField;
   private TableViewer configList;
   private Action actionSave;
   private Action actionCreate;
   private Action actionDelete;
   private Action actionMoveUp;
   private Action actionMoveDown;
   private List<AgentConfigurationHandle> elements;
   private AgentConfiguration configuration;
   private IStructuredSelection previousSelection;
   private Label configNameLabel;

   /**
    * Create server stored agent configuration  view
    */
   public ServerStoredAgentConfigEditorView()
   {
      super(i18n.tr("Agent Configurations"), ResourceManager.getImageDescriptor("icons/config-views/tunnel_manager.png"), ID, false);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
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
      configList.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((AgentConfigurationHandle)element).getName();
         }
      });
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

      Composite editSection = new Composite(splitter, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      layout.horizontalSpacing = 0;
      editSection.setLayout(layout);      

      new Label(editSection, SWT.SEPARATOR | SWT.VERTICAL).setLayoutData(new GridData(SWT.RIGHT, SWT.FILL, false, true));
      
      editorContent = new Composite(editSection, SWT.NONE);
      editorContent.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));      
      layout = new GridLayout();
      editorContent.setLayout(layout);
      
      configNameLabel = new Label(editorContent, SWT.BOLD);
      configNameLabel.setText(i18n.tr("noname"));
      GridData gridData = new GridData();
      gridData.verticalIndent = WidgetHelper.DIALOG_SPACING;
      configNameLabel.setLayoutData(gridData);
      //editorContent.setText(i18n.tr("noname")); 

      splitter.setWeights(new int[] { 20, 80 });       
      
      /**** Name ****/
      nameField = new LabeledText(editorContent, SWT.BORDER);
      nameField.setLabel(i18n.tr("Name"));
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      nameField.setLayoutData(gridData);
      nameField.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });

      /**** Filter ****/
      // Filtering script
      Label label = new Label(editorContent, SWT.NONE);
      label.setText(i18n.tr("Filter"));
      gridData = new GridData();
      gridData.verticalIndent = WidgetHelper.DIALOG_SPACING;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      label.setLayoutData(gridData);
      
      filterEditor = new ScriptEditor(editorContent, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true, 
            "Variables:\r\n\t$1\tIP address\r\n\t$2\tplatform name\r\n\t$3\tmajor agent version number\r\n\t$4\tminor agent version number\r\n\t$5\trelease number\r\n\r\nReturn value: true if this configuration should be sent to agent");
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
      // Filtering script
      label = new Label(editorContent, SWT.NONE);
      label.setText(i18n.tr("Configuration File"));
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalIndent = WidgetHelper.DIALOG_SPACING;
      label.setLayoutData(gridData);

      contentEditor = new AgentConfigEditor(editorContent, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL);
      contentEditor.getTextWidget().addModifyListener(new ModifyListener() {
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
      contentEditor.setLayoutData(gridData);

      createActions();
      createPopupMenu();
      
      actionSave.setEnabled(false);
      refresh();
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
         int choice = MessageDialogHelper.openQuestionWithCancel(getWindow().getShell(), i18n.tr("Unsaved Changes"), getSaveOnExitPrompt());
         if (choice == MessageDialogHelper.YES)
         {
            intermediateSave();
            modified = false;
            actionSave.setEnabled(false);
            return;
         }
         if (choice == MessageDialogHelper.CANCEL)
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

      final AgentConfigurationHandle element = (AgentConfigurationHandle)selection.getFirstElement();

      new Job(i18n.tr("Get configuration file content"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (element != null)
            {
               configuration = session.getAgentConfiguration(element.getId());
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
            return i18n.tr("Cannot get configuration content");
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
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
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

      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            intermediateSave();
            actionSave.setEnabled(false);
            modified = false;
         }
      };

      actionCreate = new Action(i18n.tr("Create new configuration"), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewConfig();
         }
      };

      actionDelete = new Action(i18n.tr("Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteConfig();
         }
      };

      actionMoveUp = new Action(i18n.tr("Move up"), SharedIcons.UP) {
         @Override
         public void run()
         {
            moveUp();
         }
      };

      actionMoveDown = new Action(i18n.tr("Move down"), SharedIcons.DOWN) {
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

      final AgentConfigurationHandle element1 = (AgentConfigurationHandle)selection.getFirstElement();

      int index = elements.indexOf(element1);
      if (index <= 0)
         return;

      final AgentConfigurationHandle element2 = elements.get(index - 1);

      new Job(i18n.tr("Move configuration up"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.swapAgentConfigs(element1.getId(), element2.getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot move element up");
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

      final AgentConfigurationHandle element1 = (AgentConfigurationHandle)selection.getFirstElement();

      int index = elements.indexOf(element1);
      if (index >= (elements.size() - 1))
         return;

      final AgentConfigurationHandle elemen2 = elements.get(index + 1);

      new Job(i18n.tr("Move configuration down"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.swapAgentConfigs(element1.getId(), elemen2.getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot move element down");
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

      final AgentConfigurationHandle element = (AgentConfigurationHandle)selection.getFirstElement();

      new Job(i18n.tr("Delete configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.deleteAgentConfig(element.getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  elements.remove(element);
                  configList.setInput(elements.toArray(new AgentConfigurationHandle[elements.size()]));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete configuration");
         }
      }.start();
   }

   /**
    * Creates new config that is sent to server on save
    */
   private void createNewConfig()
   {
      modified = true;
      actionSave.setEnabled(true);

      AgentConfigurationHandle newElement = new AgentConfigurationHandle();
      elements.add(newElement);
      configList.setInput(elements.toArray(new AgentConfigurationHandle[elements.size()]));

      reselection = true;
      StructuredSelection selection = new StructuredSelection(newElement);
      previousSelection = selection;
      configList.setSelection(selection);
      configuration = new AgentConfiguration();
      updateContent();
   }

   /**
    * Update editor content
    */
   private void updateContent()
   {
      configNameLabel.setText(configuration.getName());
      nameField.setText(configuration.getName());
      contentEditor.setText(configuration.getContent());
      filterEditor.setText(configuration.getFilter());
      modified = false;
      actionSave.setEnabled(false);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
      manager.add(actionCreate);
      manager.add(new Separator());
   }

   /**
    * 
    */
   public void intermediateSave()
   {
      if (configuration == null)
         return;
      configuration.setContent(contentEditor.getText());
      configuration.setFilter(filterEditor.getText());
      configuration.setName(nameField.getText());

      new Job(i18n.tr("Save configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.saveAgentConfig(configuration);

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save configuration");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#getSaveOnExitPrompt()
    */
   @Override
   public String getSaveOnExitPrompt()
   {
      return i18n.tr("There are unsaved changes to configuration file. Do you want to save them?");
   }

   /**
    * Gets config list from server and sets editable fields to nothing
    */
   @Override
   public void refresh()
   {
      IStructuredSelection selection = (IStructuredSelection)configList.getSelection();
      final AgentConfigurationHandle element = selection == null ? null : (AgentConfigurationHandle)selection.getFirstElement();

      new Job(i18n.tr("Open package database"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            elements = session.getAgentConfigurations();

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  configList.setInput(elements.toArray(new AgentConfigurationHandle[elements.size()]));

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
            return i18n.tr("Cannot get configurations list");
         }
      }.start();
   }

   @Override
   public boolean isModified()
   {
      return modified;
   }

   @Override
   public void save()
   {
      try
      {
         configuration.setContent(contentEditor.getText());
         configuration.setFilter(filterEditor.getText());
         configuration.setName(nameField.getText());
         session.saveAgentConfig(configuration);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getWindow().getShell(), i18n.tr("Error"),
               i18n.tr("Cannot save agent's configuration file: ") + e.getMessage());
      }
   }
}
