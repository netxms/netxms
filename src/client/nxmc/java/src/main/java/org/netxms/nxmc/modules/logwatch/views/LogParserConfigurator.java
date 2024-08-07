/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logwatch.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logwatch.widgets.LogParserEditor;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserModifyListener;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserType;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Parser configurator for global log parsers
 */
public class LogParserConfigurator extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(LogParserConfigurator.class);

   private LogParserType parserType;
   private String configName;
   private String displayName;
	private NXCSession session;
	private LogParserEditor editor;
	private boolean modified = false;
	private String content;
	private Action actionSave;
   private String savedEditorState = null;

   /**
    * Create global log parser configuration view
    */
   public LogParserConfigurator(LogParserType parserType, String configName, String displayName)
   {
      super(String.format(LocalizationHelper.getI18n(LogParserConfigurator.class).tr("%s Parser"), displayName), ResourceManager.getImageDescriptor("icons/config-views/log-parser.png"), configName, false);
      this.parserType = parserType;
      this.configName = configName;
      this.displayName = displayName;
      session = Registry.getSession();
   }

   /**
    * Create global log parser configuration view
    */
   protected LogParserConfigurator()
   {
      super(null, ResourceManager.getImageDescriptor("icons/config-views/log-parser.png"), null, false);
      session = Registry.getSession();
   }  

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      LogParserConfigurator view = (LogParserConfigurator)super.cloneView();
      view.parserType = parserType;
      view.configName = configName;
      view.displayName = displayName;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
	{
      editor = new LogParserEditor(parent, SWT.NONE, parserType);
		editor.addModifyListener(new LogParserModifyListener() {
			@Override
			public void modifyParser()
			{
				setModified(true);
			}
		});

		createActions();
	}

	/**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      
      if (savedEditorState != null)
      {
         editor.setParserXml(savedEditorState);
         setModified(true);
         savedEditorState = null;
      }
      else
         refresh();
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
      addKeyBinding("M1+S", actionSave);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionSave);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionSave);
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
		editor.setFocus();
	}

	/**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
		if (modified)
		{
         if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Refresh"), i18n.tr("This will destroy all unsaved changes. Are you sure?")))
				return;
		}

		actionSave.setEnabled(false);
      new Job(String.format(i18n.tr("Loading %s parser configuration"), displayName), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				try
				{
               content = session.getServerConfigClob(configName);
				}
				catch(NXCException e)
				{
               // If parser is not configured, server will return
					// UNKNOWN_VARIABLE error
					if (e.getErrorCode() != RCC.UNKNOWN_VARIABLE)
						throw e;
               content = "<parser>\n</parser>\n";
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						editor.setParserXml(content);
						setModified(false);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot load %s parser configuration"), displayName);
			}
		}.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
	{
		final String xml = editor.getParserXml();
		actionSave.setEnabled(false);
      new Job(String.format(i18n.tr("Saving %s parser configuraton"), displayName), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            session.setServerConfigClob(configName, xml);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						setModified(false);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot save %s parser configuration"), displayName);
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
    * Mark view as modified
    */
	private void setModified(boolean b)
	{
		if (b != modified)
		{
			modified = b;
			actionSave.setEnabled(modified);
		}
	}

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      if (modified)
      {
         memento.set("editorContent", editor.getParserXml()); 
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
      savedEditorState = memento.getAsString("editorContent", null);
      setName(String.format(LocalizationHelper.getI18n(LogParserConfigurator.class).tr("%s Parser"), displayName));
   }      
}
