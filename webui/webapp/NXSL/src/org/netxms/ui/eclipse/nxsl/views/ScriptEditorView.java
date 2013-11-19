/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.nxsl.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.scripts.Script;
import org.netxms.api.client.scripts.ScriptLibraryManager;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.Activator;
import org.netxms.ui.eclipse.nxsl.Messages;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Script editor view
 *
 */
public class ScriptEditorView extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.nxsl.views.ScriptEditorView"; //$NON-NLS-1$
	
	private ScriptLibraryManager scriptLibraryManager;
	private ScriptEditor editor;
	private long scriptId;
	private String scriptName;
	private RefreshAction actionRefresh;
	private Action actionSave;
	private boolean modified = false;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		scriptLibraryManager = (ScriptLibraryManager)ConsoleSharedData.getSession();
		scriptId = Long.parseLong(site.getSecondaryId());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		parent.setLayout(new FillLayout());
		
		editor = new ScriptEditor(parent, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL);
		editor.getTextWidget().addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				if (!modified)
				{
					modified = true;
					firePropertyChange(PROP_DIRTY);
					actionSave.setEnabled(true);
				}
			}
		});
		
		createActions();
		contributeToActionBars();
		//createPopupMenu();

		reloadScript();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
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
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				reloadScript();
			}
		};
		
		actionSave = new Action(Messages.get().ScriptEditorView_Save, SharedIcons.SAVE) {
			@Override
			public void run()
			{
				saveScript();
			}
		};
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
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
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
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionSave);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Reload script from server
	 */
	private void reloadScript()
	{
		new ConsoleJob(String.format(Messages.get().ScriptEditorView_LoadJobTitle, scriptId), this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().ScriptEditorView_LoadJobError, scriptId);
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Script script = scriptLibraryManager.getScript(scriptId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						scriptName = script.getName();
						setPartName(String.format(Messages.get().ScriptEditorView_PartName, scriptName));
						editor.setText(script.getSource());
						actionSave.setEnabled(false);
					}
				});
			}
		}.start();
	}
	
	/**
	 * Save script
	 */
	private void saveScript()
	{
		final String source = editor.getText();
		editor.getTextWidget().setEditable(false);
		new ConsoleJob(Messages.get().ScriptEditorView_SaveJobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ScriptEditorView_SaveJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				doScriptSave(source, monitor);
			}
		}.start();
	}
	
	/**
	 * Do actual script save
	 * 
	 * @param source
	 * @param monitor
	 * @throws Exception
	 */
	private void doScriptSave(String source, IProgressMonitor monitor) throws Exception
	{
		scriptLibraryManager.modifyScript(scriptId, scriptName, source);
		editor.getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            if (editor.isDisposed())
               return;
            
            editor.getTextWidget().setEditable(true);
            actionSave.setEnabled(false);
            modified = false;
            firePropertyChange(PROP_DIRTY);
         }
      });
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#getAdapter(java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Class adapter)
	{
		Object object = super.getAdapter(adapter);
		if (object != null)
		{
			return object;
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		final String source = editor.getText();
		editor.getTextWidget().setEditable(false);
		try
		{
			doScriptSave(source, monitor);
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(getViewSite().getShell(), Messages.get().ScriptEditorView_Error, String.format(Messages.get().ScriptEditorView_SaveErrorMessage, e.getMessage()));
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSaveAs()
	 */
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
}
