/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.agent.config.AgentConfiguration;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.widgets.AgentConfigEditor;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Editor for agent configuration stored on server
 */
public class StoredAgentConfigurationEditor extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(StoredAgentConfigurationEditor.class);

   private NXCSession session;
   private long configurationId;
   private String configurationName;
   private boolean modified = false;
   private Composite editorContent;
   private AgentConfigEditor contentEditor;
   private ScriptEditor filterEditor;
   private Action actionSave;

   /**
    * Create server stored agent configuration  view
    */
   public StoredAgentConfigurationEditor(long configurationId, String configurationName)
   {
      super(configurationName, ResourceManager.getImageDescriptor("icons/object-views/agent-config.png"), "StoredAgentConfigurationEditor." + Long.toString(configurationId), false);
      this.configurationId = configurationId;
      this.configurationName = configurationName;
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      editorContent = new Composite(parent, SWT.NONE);
      editorContent.setLayout(new GridLayout());

      /**** Filter ****/
      // Filtering script
      Label label = new Label(editorContent, SWT.NONE);
      label.setText(i18n.tr("Filter"));
      GridData gridData = new GridData();
      gridData.verticalIndent = WidgetHelper.DIALOG_SPACING;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      label.setLayoutData(gridData);

      filterEditor = new ScriptEditor(editorContent, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL,  
            "Variables:\n\t$1\tIP address\n\t$2\tplatform name\n\t$3\tmajor agent version number\n\t$4\tminor agent version number\n\t$5\trelease number\n\nReturn value: true if this configuration should be sent to agent");
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

      /**** Configuration file ****/
      // Filtering script
      label = new Label(editorContent, SWT.NONE);
      label.setText(i18n.tr("Configuration file"));
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
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * Called when any of text fields is modified
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
    * Create actions
    */
   private void createActions()
   {
      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
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
      new Job(i18n.tr("Loading stored agent configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            AgentConfiguration configuration = session.getAgentConfiguration(configurationId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  contentEditor.setText(configuration.getContent());
                  filterEditor.setText(configuration.getFilter());
                  modified = false;
                  actionSave.setEnabled(false);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load stored agent configuration");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return modified;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
      final AgentConfiguration configuration = new AgentConfiguration();
      configuration.setId(configurationId);
      configuration.setName(configurationName);
      configuration.setFilter(filterEditor.getText());
      configuration.setContent(contentEditor.getText());

      new Job(i18n.tr("Saving agent configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.saveAgentConfig(configuration);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  modified = false;
                  actionSave.setEnabled(false);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save agent configuration");
         }
      }.start();
   }
}
