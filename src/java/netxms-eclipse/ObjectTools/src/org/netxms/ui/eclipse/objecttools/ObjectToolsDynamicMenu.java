/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URLEncoder;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.ContributionItem;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.ui.ISources;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.console.ConsolePlugin;
import org.eclipse.ui.console.IConsole;
import org.eclipse.ui.console.IConsoleConstants;
import org.eclipse.ui.console.IOConsole;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.menus.IWorkbenchContribution;
import org.eclipse.ui.progress.UIJob;
import org.eclipse.ui.services.IEvaluationService;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.views.FileViewer;
import org.netxms.ui.eclipse.objecttools.views.TableToolResults;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Dynamic object tools menu creator
 *
 */
public class ObjectToolsDynamicMenu extends ContributionItem implements IWorkbenchContribution
{
	private IEvaluationService evalService;
	
	/**
	 * Creates a contribution item with a null id.
	 */
	public ObjectToolsDynamicMenu()
	{
		super();
	}

	/**
	 * Creates a contribution item with the given (optional) id.
	 * 
	 * @param id the contribution item identifier, or null
	 */
	public ObjectToolsDynamicMenu(String id)
	{
		super(id);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.menus.IWorkbenchContribution#initialize(org.eclipse.ui.services.IServiceLocator)
	 */
	@Override
	public void initialize(IServiceLocator serviceLocator)
	{
		evalService = (IEvaluationService)serviceLocator.getService(IEvaluationService.class);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.action.ContributionItem#dispose()
	 */
	@Override
	public void dispose()
	{
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.action.ContributionItem#fill(org.eclipse.swt.widgets.Menu, int)
	 */
	@Override
	public void fill(Menu menu, int index)
	{
		Object selection = evalService.getCurrentState().getVariable(ISources.ACTIVE_MENU_SELECTION_NAME);
		if ((selection == null) || !(selection instanceof IStructuredSelection))
			return;
		if ((((IStructuredSelection)selection).size() != 1) ||
				!(((IStructuredSelection)selection).getFirstElement() instanceof Node))
			return;
		final Node node = (Node)((IStructuredSelection)selection).getFirstElement();
		
		Menu toolsMenu = new Menu(menu);
		
		ObjectTool[] tools = ObjectToolsCache.getTools();
		Arrays.sort(tools, new Comparator<ObjectTool>() {
			@Override
			public int compare(ObjectTool arg0, ObjectTool arg1)
			{
				return arg0.getName().replace("&", "").compareToIgnoreCase(arg1.getName().replace("&", ""));
			}
		});
		
		Map<String, Menu> menus = new HashMap<String, Menu>();
		int added = 0;
		for(int i = 0; i < tools.length; i++)
		{
			if (tools[i].isApplicableForNode(node))
			{
				String[] path = tools[i].getName().split("\\-\\>");
			
				Menu rootMenu = toolsMenu;
				for(int j = 0; j < path.length - 1; j++)
				{
					String key = path[j].replace("&", "");
					Menu currMenu = menus.get(key);
					if (currMenu == null)
					{
						currMenu = new Menu(rootMenu);
						MenuItem item = new MenuItem(rootMenu, SWT.CASCADE);
						item.setText(path[j]);
						item.setMenu(currMenu);
						menus.put(key, currMenu);
					}
					rootMenu = currMenu;
				}
				
				final MenuItem item = new MenuItem(rootMenu, SWT.PUSH);
				item.setText(path[path.length - 1]);
				item.setData(tools[i]);
				item.addSelectionListener(new SelectionAdapter() {
					@Override
					public void widgetSelected(SelectionEvent e)
					{
						executeObjectTool(node, (ObjectTool)item.getData());
					}
				});
				
				added++;
			}
		}

		if (added > 0)
		{
			MenuItem toolsMenuItem = new MenuItem(menu, SWT.CASCADE, index);
			toolsMenuItem.setText("&Tools");
			toolsMenuItem.setMenu(toolsMenu);
		}
		else
		{
			toolsMenu.dispose();
		}
	}
	
	/**
	 * Execute object tool
	 * @param tool Object tool
	 */
	private void executeObjectTool(final Node node, final ObjectTool tool)
	{
		switch(tool.getType())
		{
			case ObjectTool.TYPE_LOCAL_COMMAND:
				executeLocalCommand(node, tool);
				break;
			case ObjectTool.TYPE_ACTION:
				executeAgentAction(node, tool);
				break;
			case ObjectTool.TYPE_TABLE_AGENT:
			case ObjectTool.TYPE_TABLE_SNMP:
				executeTableTool(node, tool);
				break;
			case ObjectTool.TYPE_URL:
				break;
			case ObjectTool.TYPE_FILE_DOWNLOAD:
				executeFileDownload(node, tool);
				break;
		}
	}
	
	/**
	 * Execute table tool
	 * 
	 * @param node
	 * @param tool
	 */
	private void executeTableTool(final Node node, final ObjectTool tool)
	{
		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			final IWorkbenchPage page = window.getActivePage();
			final TableToolResults view = (TableToolResults)page.showView(TableToolResults.ID,
					Long.toString(tool.getId()) + "&" + Long.toString(node.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
			view.refreshTable();
		}
		catch(PartInitException e)
		{
			MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
		}
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeAgentAction(final Node node, final ObjectTool tool)
	{
		new ConsoleJob("Execute action on node " + node.getObjectName(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute action on node " + node.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				session.executeAction(node.getObjectId(), tool.getData());
				new UIJob("Notify user about action execution") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						MessageDialog.openInformation(null, "Tool Execution", "Action " + tool.getData() + " executed successfully on node " + node.getObjectName());
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		}.start();
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeLocalCommand(final Node node, final ObjectTool tool)
	{
		String temp = tool.getData();
		temp = temp.replace("%OBJECT_IP_ADDR%", node.getPrimaryIP().getHostAddress());
		temp = temp.replace("%OBJECT_NAME%", node.getObjectName());
		final String command = temp.replace("%OBJECT_ID%", Long.toString(node.getObjectId()));
		final IOConsole console = new IOConsole(command, Activator.getImageDescriptor("icons/console.png"));
		IViewPart consoleView = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().findView(IConsoleConstants.ID_CONSOLE_VIEW);
		System.out.println("Console view is " + consoleView);
		ConsolePlugin.getDefault().getConsoleManager().addConsoles(new IConsole[] { console });
		ConsolePlugin.getDefault().getConsoleManager().showConsoleView(console);
		//console.createPage((IConsoleView)consoleView);
		//console.activate();
		final IOConsoleOutputStream out = console.newOutputStream();
		
		ConsoleJob job = new ConsoleJob("Execute external command", null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute external command";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				Process proc = Runtime.getRuntime().exec(command);
				BufferedReader in = new BufferedReader(new InputStreamReader(proc.getInputStream()));
				try
				{
					while(true)
					{
						String line = in.readLine();
						if (line == null)
							break;
						out.write(line);
						out.write("\n");
					}
				}
				catch(IOException e)
				{
					e.printStackTrace();
				}
				//console.setName("FINISHED: " + console.getName());
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeFileDownload(final Node node, final ObjectTool tool)
	{
		String temp = tool.getData();
		temp = temp.replace("%OBJECT_IP_ADDR%", node.getPrimaryIP().getHostAddress());
		temp = temp.replace("%OBJECT_NAME%", node.getObjectName());
		final String fileName = temp.replace("%OBJECT_ID%", Long.toString(node.getObjectId()));
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		ConsoleJob job = new ConsoleJob("Download file from agent", null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot download file " + fileName + " from node " + node.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final File file = session.downloadFileFromAgent(node.getObjectId(), fileName);
				Display.getDefault().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
						try
						{
							String secondaryId = Long.toString(node.getObjectId()) + "&" + URLEncoder.encode(fileName, "UTF-8");
							FileViewer view = (FileViewer)window.getActivePage().showView(FileViewer.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
							view.showFile(file);
						}
						catch(Exception e)
						{
							MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
						}
					}
				});
			}
		};
		job.start();
	}
}
