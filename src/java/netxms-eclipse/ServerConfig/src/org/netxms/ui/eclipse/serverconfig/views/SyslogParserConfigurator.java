/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.widgets.LogParserEditor;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParserModifyListener;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Syslog parser configurator
 */
public class SyslogParserConfigurator extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.SyslogParserConfigurator"; //$NON-NLS-1$
	
	private NXCSession session;
	private LogParserEditor editor;
	private boolean modified = false;
	private String content;
	private Action actionRefresh;
	private Action actionSave;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		editor = new LogParserEditor(parent, SWT.NONE, true);
		editor.addModifyListener(new LogParserModifyListener() {
			@Override
			public void modifyParser()
			{
				setModified(true);
			}
		});
		
		createActions();
		contributeToActionBars();
		
		refresh();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refresh();
			}
		};
		
		actionSave = new Action(Messages.get().SyslogParserConfigurator_Save, SharedIcons.SAVE) {
			@Override
			public void run()
			{
				save();
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		editor.setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		editor.getDisplay().syncExec(new Runnable() {
			@Override
			public void run()
			{
				content = editor.getParserXml();
			}
		});
		try
		{
			session.setServerConfigClob("SyslogParser", content); //$NON-NLS-1$
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(getSite().getShell(), Messages.get().SyslogParserConfigurator_Error, 
			      String.format(Messages.get().SyslogParserConfigurator_ErrorSaveConfig, e.getLocalizedMessage()));
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

	/**
	 * Refresh viewer
	 */
	private void refresh()
	{
		if (modified)
		{
			if (!MessageDialogHelper.openQuestion(getSite().getShell(), Messages.get().SyslogParserConfigurator_ConfirmRefresh, Messages.get().SyslogParserConfigurator_ConfirmRefreshText))
				return;
		}
		
		actionSave.setEnabled(false);
		new ConsoleJob(Messages.get().SyslogParserConfigurator_LoadJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				try
				{
					content = session.getServerConfigClob("SyslogParser"); //$NON-NLS-1$
				}
				catch(NXCException e)
				{
					// If syslog parser is not configured, server will return
					// UNKNOWN_VARIABLE error
					if (e.getErrorCode() != RCC.UNKNOWN_VARIABLE)
						throw e;
					content = "<parser>\n</parser>\n"; //$NON-NLS-1$
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
				return Messages.get().SyslogParserConfigurator_LoadJobError;
			}
		}.start();
	}
	
	/**
	 * Save config
	 */
	private void save()
	{
		final String xml = editor.getParserXml();
		actionSave.setEnabled(false);
		new ConsoleJob(Messages.get().SyslogParserConfigurator_SaveJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.setServerConfigClob("SyslogParser", xml); //$NON-NLS-1$
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
				return Messages.get().SyslogParserConfigurator_SaveJobError;
			}
		}.start();
	}
	
	/**
	 * Mark view as modified
	 */
	private void setModified(boolean b)
	{
		if (b != modified)
		{
			modified = b;
			firePropertyChange(PROP_DIRTY);
			actionSave.setEnabled(modified);
		}
	}
}
