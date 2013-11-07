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
package org.netxms.ui.eclipse.scripting.views;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.eclipse.ui.console.IOConsole;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.console.TextConsoleViewer;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.base.NXCommon;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.scripting.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.python.core.Py;
import org.python.core.PyException;
import org.python.core.PySystemState;
import org.python.util.InteractiveInterpreter;

/**
 * Results of local command execution
 */
public class ScriptingConsole extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.scripting.views.ScriptingConsole"; //$NON-NLS-1$

	private InteractiveInterpreter python;
	private TextConsoleViewer viewer;
	private IOConsole console;
	private boolean running = false;
	private Object mutex = new Object();
	private Action actionClear;
	private Action actionScrollLock;
	private Action actionTerminate;
	private Action actionCopy;
	private Action actionSelectAll;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		console = new IOConsole("Console", Activator.getImageDescriptor("icons/console.png"));
		viewer = new TextConsoleViewer(parent, console);
		viewer.setEditable(true);
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
		
		createInterpreter();
		runInterpreter();
	}

	/**
	 * Create Python interpreter
	 */
	private void createInterpreter()
	{
		PySystemState systemState = new PySystemState();
		python = new InteractiveInterpreter(null, systemState);
		Py.getSystemState().__setattr__("_jy_interpreter", Py.java2py(python));
		//imp.load("site");

		python.exec("from org.netxms.client import NXCSession");
		python.exec("from org.netxms.api.client import Session");

		python.set("__name__", "__nxshell__");
		
		python.set("session", ConsoleSharedData.getSession());
		python.set("page", getSite().getPage());
		
		python.setIn(console.getInputStream());
	}
	
	/**
	 * 
	 */
	private void runInterpreter()
	{
		final IOConsoleOutputStream out = console.newOutputStream();
		python.setOut(out);
		//python.setErr(out);
		ConsoleJob job = new ConsoleJob("Python interpreter", this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute external command";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				try
				{
					out.write("NetXMS " + NXCommon.VERSION + " Interactive Shell\n");
					BufferedReader reader = new BufferedReader(new InputStreamReader(console.getInputStream()));
					while(true)
					{
						String line = reader.readLine();
						try
						{
							python.exec(line);
						}
						catch(PyException e)
						{
							StringBuilder sb = new StringBuilder("Traceback (most recent call last):\n");
							e.traceback.dumpStack(sb);
							sb.append(e.value.toString());
							sb.append('\n');
							out.write(sb.toString());
						}
					}
				}
				catch(Exception e)
				{
					e.printStackTrace();
				}
				finally
				{
					out.close();
				}
			}

			@Override
			protected void jobFinalize()
			{
				synchronized(mutex)
				{
					running = false;
					mutex.notifyAll();
				}

				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						synchronized(mutex)
						{
							actionTerminate.setEnabled(running);
						}
					}
				});
			}
		};
		job.setUser(false);
		job.setSystem(true);
		job.start();
	}
	
	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.scripting.context.ScriptingConsole");
		}
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
		
		actionClear = new Action("C&lear console", SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				console.clearConsole();
			}
		};

		actionScrollLock = new Action("&Scroll lock", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
			}
		};
		actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
		actionScrollLock.setChecked(false);
		
		actionTerminate = new Action("&Terminate", SharedIcons.TERMINATE) {
			@Override
			public void run()
			{
				synchronized(mutex)
				{
					if (running)
					{
					}
				}
			}
		};
		actionTerminate.setEnabled(false);
      actionTerminate.setActionDefinitionId("org.netxms.ui.eclipse.scripting.commands.terminate_process");
		handlerService.activateHandler(actionTerminate.getActionDefinitionId(), new ActionHandler(actionTerminate));
		
		actionCopy = new Action("&Copy") {
			@Override
			public void run()
			{
				if (viewer.canDoOperation(TextConsoleViewer.COPY))
					viewer.doOperation(TextConsoleViewer.COPY);
			}
		};
		actionCopy.setEnabled(false);
      actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.scripting.commands.copy");
		handlerService.activateHandler(actionCopy.getActionDefinitionId(), new ActionHandler(actionCopy));
		
		actionSelectAll = new Action("Select &all") {
			@Override
			public void run()
			{
				if (viewer.canDoOperation(TextConsoleViewer.SELECT_ALL))
					viewer.doOperation(TextConsoleViewer.SELECT_ALL);
			}
		};
      actionSelectAll.setActionDefinitionId("org.netxms.ui.eclipse.scripting.commands.select_all");
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
			}
		}
		super.dispose();
	}
}
