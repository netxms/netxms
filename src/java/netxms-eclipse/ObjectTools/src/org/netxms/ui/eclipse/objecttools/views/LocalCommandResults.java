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
package org.netxms.ui.eclipse.objecttools.views;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.console.IOConsole;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.console.TextConsoleViewer;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.ObjectToolsCache;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Results of local command execution
 */
public class LocalCommandResults extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.LocalCommandResults"; //$NON-NLS-1$

	private long nodeId;
	private long toolId;
	private TextConsoleViewer viewer;
	private IOConsole console;
	private Process process;
	private boolean running = false;
	private String lastCommand = null;
	private Object mutex = new Object();
	private Action actionClear;
	private Action actionScrollLock;
	private Action actionTerminate;
	private Action actionRestart;
	private Action actionCopy;
	private Action actionSelectAll;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		// Secondary ID must be in form toolId&nodeId
		String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
		if (parts.length != 2)
			throw new PartInitException("Internal error"); //$NON-NLS-1$
		
		try
		{
			nodeId = Long.parseLong(parts[0]);
			toolId = Long.parseLong(parts[1]);
			
			NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			AbstractObject object = session.findObjectById(nodeId);
			setPartName(object.getObjectName() + " - " + ObjectToolsCache.getInstance().findTool(toolId).getDisplayName()); //$NON-NLS-1$
		}
		catch(Exception e)
		{
			throw new PartInitException("Unexpected initialization failure", e); //$NON-NLS-1$
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		console = new IOConsole("Console", Activator.getImageDescriptor("icons/console.png")); //$NON-NLS-1$ //$NON-NLS-2$
		viewer = new TextConsoleViewer(parent, console);
		viewer.setEditable(false);
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				actionCopy.setEnabled(viewer.canDoOperation(TextConsoleViewer.COPY));
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();

		activateContext();
	}

	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.objecttools.context.LocalCommandResults"); //$NON-NLS-1$
		}
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
		
		actionClear = new Action(Messages.LocalCommandResults_ClearConsole, SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				console.clearConsole();
			}
		};

		actionScrollLock = new Action(Messages.LocalCommandResults_ScrollLock, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
			}
		};
		actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
		actionScrollLock.setChecked(false);
		
		actionTerminate = new Action(Messages.LocalCommandResults_Terminate, SharedIcons.TERMINATE) {
			@Override
			public void run()
			{
				synchronized(mutex)
				{
					if (running)
					{
						process.destroy();
					}
				}
			}
		};
		actionTerminate.setEnabled(false);
      actionTerminate.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.terminate_process"); //$NON-NLS-1$
		handlerService.activateHandler(actionTerminate.getActionDefinitionId(), new ActionHandler(actionTerminate));
		
		actionRestart = new Action(Messages.LocalCommandResults_Restart, SharedIcons.RESTART) {
			@Override
			public void run()
			{
				runCommand(lastCommand);
			}
		};
		actionRestart.setEnabled(false);

		actionCopy = new Action(Messages.LocalCommandResults_Copy) {
			@Override
			public void run()
			{
				if (viewer.canDoOperation(TextConsoleViewer.COPY))
					viewer.doOperation(TextConsoleViewer.COPY);
			}
		};
		actionCopy.setEnabled(false);
      actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.copy"); //$NON-NLS-1$
		handlerService.activateHandler(actionCopy.getActionDefinitionId(), new ActionHandler(actionCopy));
		
		actionSelectAll = new Action(Messages.LocalCommandResults_SelectAll) {
			@Override
			public void run()
			{
				if (viewer.canDoOperation(TextConsoleViewer.SELECT_ALL))
					viewer.doOperation(TextConsoleViewer.SELECT_ALL);
			}
		};
      actionSelectAll.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.select_all"); //$NON-NLS-1$
		handlerService.activateHandler(actionSelectAll.getActionDefinitionId(), new ActionHandler(actionSelectAll));
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
	 * @param manager Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionTerminate);
		manager.add(actionRestart);
		manager.add(new Separator());
		manager.add(actionClear);
		manager.add(actionScrollLock);
		manager.add(new Separator());
		manager.add(actionSelectAll);
		manager.add(actionCopy);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionTerminate);
		manager.add(actionRestart);
		manager.add(new Separator());
		manager.add(actionClear);
		manager.add(actionScrollLock);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	private void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionTerminate);
		manager.add(actionRestart);
		manager.add(new Separator());
		manager.add(actionClear);
		manager.add(actionScrollLock);
		manager.add(new Separator());
		manager.add(actionSelectAll);
		manager.add(actionCopy);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTextWidget().setFocus();
	}
	
	/**
	 * Run command (called by tool execution action)
	 * 
	 * @param command
	 */
	public void runCommand(final String command)
	{
		synchronized(mutex)
		{
			if (running)
			{
				process.destroy();
				try
				{
					mutex.wait();
				}
				catch(InterruptedException e)
				{
				}
			}
			running = true;
			lastCommand = command;
			actionTerminate.setEnabled(true);
			actionRestart.setEnabled(false);
		}
		
		final IOConsoleOutputStream out = console.newOutputStream();
		ConsoleJob job = new ConsoleJob(Messages.LocalCommandResults_JobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.LocalCommandResults_JobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				process = Runtime.getRuntime().exec(command);
				InputStream in = process.getInputStream();
				try
				{
					byte[] data = new byte[16384];
					boolean isWindows = Platform.getOS().equals(Platform.OS_WIN32);
					while(true)
					{
						int bytes = in.read(data);
						if (bytes == -1)
							break;
						String s = new String(Arrays.copyOf(data, bytes));
						
						// The following is a workaround for issue NX-65
						// Problem is that on Windows XP many system commands
						// (like ping, tracert, etc.) generates output with lines
						// ending in 0x0D 0x0D 0x0A
						if (isWindows)
							out.write(s.replace("\r\r\n", " \r\n")); //$NON-NLS-1$ //$NON-NLS-2$
						else
							out.write(s);
					}
					
					out.write(Messages.LocalCommandResults_Terminated);
				}
				catch(IOException e)
				{
					e.printStackTrace();
				}
				finally
				{
					in.close();
					out.close();
				}
			}

			@Override
			protected void jobFinalize()
			{
				synchronized(mutex)
				{
					running = false;
					process = null;
					mutex.notifyAll();
				}

				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						synchronized(mutex)
						{
							actionTerminate.setEnabled(running);
							actionRestart.setEnabled(!running);
						}
					}
				});
			}
		};
		job.setUser(false);
		job.setSystem(true);
		job.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		synchronized(mutex)
		{
			if (running)
			{
				process.destroy();
			}
		}
		super.dispose();
	}
}
