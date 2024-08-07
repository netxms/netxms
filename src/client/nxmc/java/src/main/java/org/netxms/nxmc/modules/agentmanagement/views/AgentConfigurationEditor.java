/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.dialogs.SaveConfigDialog;
import org.netxms.nxmc.modules.agentmanagement.widgets.AgentConfigEditor;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Agent's master config editor
 */
public class AgentConfigurationEditor extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(AgentConfigurationEditor.class);

	private AgentConfigEditor editor;
	private boolean modified = false;
	private String textSavedVeriosn = null;
	private Action actionSave;
   private Action actionSaveAndApply;

   /**
    * Create agent configuration editor for given node.
    *
    * @param node node object
    */
   public AgentConfigurationEditor(Node node, long contextId)
   {
      super(LocalizationHelper.getI18n(AgentConfigurationEditor.class).tr("Agent Configuration"), ResourceManager.getImageDescriptor("icons/object-views/agent-config.png"), "AgentConfigurationEditor", node.getObjectId(), contextId, false);
   }

   /**
    * Create agent configuration editor for cloning.
    */
   protected AgentConfigurationEditor()
   {
      super(LocalizationHelper.getI18n(AgentConfigurationEditor.class).tr("Agent Configuration"), ResourceManager.getImageDescriptor("icons/object-views/agent-config.png"), "AgentConfigurationEditor", 0, 0, false); 
   }

   /**
    * Post clone action
    */
   protected void postClone(View origin)
   {    
      super.postClone(origin);
      AgentConfigurationEditor view = (AgentConfigurationEditor)origin;
      editor.setText(view.editor.getText());
      modified = view.modified;
      actionSave.setEnabled(view.actionSave.isEnabled());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(Composite parent)
	{
		editor = new AgentConfigEditor(parent, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL);
		editor.getTextWidget().addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
            if (!modified)
				{
					modified = true;
					actionSave.setEnabled(true);
				}
			}
		});

		createActions();
      actionSave.setEnabled(false);
	}

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      if (textSavedVeriosn == null)
         refresh();
      else
         editor.setText(textSavedVeriosn);
         
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
		editor.setFocus();
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
            saveConfig(false);
			}
		};
      addKeyBinding("M1+S", actionSave);

      actionSaveAndApply = new Action(i18n.tr("Save && &apply")) {
         @Override
         public void run()
         {
            saveConfig(true);
         }
      };
      addKeyBinding("M1+M2+S", actionSaveAndApply);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionSave);
      manager.add(actionSaveAndApply);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
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
         if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Reload Content"),
               i18n.tr("You have unsaved changes to configuration file. These changes will be lost after reload. Do you want to continue?")))
            return;
      }

      clearMessages();
      new Job(i18n.tr("Loading agent configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String config = session.readAgentConfigurationFile(getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  editor.setText(config);
                  modified = false;
                  actionSave.setEnabled(false);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load agent configuration file");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#beforeClose()
    */
   @Override
   public boolean beforeClose()
   {
      if (!modified)
         return true;

      SaveConfigDialog dlg = new SaveConfigDialog(getWindow().getShell());
      int rc = dlg.open();
      if (rc == IDialogConstants.CANCEL_ID)
         return false;

      if (rc == SaveConfigDialog.DISCARD_ID)
         return true;

      saveConfig(rc == SaveConfigDialog.SAVE_AND_APPLY_ID);
      return true;
   }

   /**
    * Save configuration,
    *
    * @param saveAndApply true to apply configuration
    */
   private void saveConfig(final boolean saveAndApply)
   {
      if (!modified)
         return;

      clearMessages();
      final String content = editor.getText();
      final long nodeId = getObjectId();
      new Job(i18n.tr("Saving agent configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.writeAgentConfigurationFile(nodeId, content, saveAndApply);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  modified = false;
                  actionSave.setEnabled(false);
                  addMessage(MessageArea.SUCCESS, i18n.tr("Agent configuration saved"));
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

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);     
      if (modified)
         memento.set("config", editor.getText());  
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      textSavedVeriosn = memento.getAsString("config", null);     
   }
}
