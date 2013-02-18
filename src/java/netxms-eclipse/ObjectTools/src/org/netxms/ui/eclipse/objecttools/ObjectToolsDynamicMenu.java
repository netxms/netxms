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
package org.netxms.ui.eclipse.objecttools;

import java.io.File;
import java.io.IOException;
import java.net.URLEncoder;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.ContributionItem;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.ui.ISources;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.menus.IWorkbenchContribution;
import org.eclipse.ui.services.IEvaluationService;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolHandler;
import org.netxms.ui.eclipse.objecttools.views.BrowserView;
import org.netxms.ui.eclipse.objecttools.views.FileViewer;
import org.netxms.ui.eclipse.objecttools.views.LocalCommandResults;
import org.netxms.ui.eclipse.objecttools.views.TableToolResults;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Dynamic object tools menu creator
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
	 * @see org.eclipse.jface.action.ContributionItem#fill(org.eclipse.swt.widgets.Menu, int)
	 */
	@Override
	public void fill(Menu menu, int index)
	{
		Object selection = evalService.getCurrentState().getVariable(ISources.ACTIVE_MENU_SELECTION_NAME);
		if ((selection == null) || !(selection instanceof IStructuredSelection))
			return;

		final Set<Node> nodes = buildNodeSet((IStructuredSelection)selection);
		final Menu toolsMenu = new Menu(menu);
		
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
			boolean allowed = isToolAllowed(tools[i], nodes);
			
			if (allowed && isToolApplicable(tools[i], nodes))
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
						executeObjectTool(nodes, (ObjectTool)item.getData());
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
	 * Build node set from selection
	 * 
	 * @param selection
	 * @return
	 */
	private Set<Node> buildNodeSet(IStructuredSelection selection)
	{
		final Set<Node> nodes = new HashSet<Node>();
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		for(Object o : selection.toList())
		{
			if (o instanceof Node)
			{
				nodes.add((Node)o);
			}
			else if ((o instanceof Container) || (o instanceof ServiceRoot) || (o instanceof Subnet) || (o instanceof Cluster))
			{
				for(GenericObject n : ((GenericObject)o).getAllChilds(GenericObject.OBJECT_NODE))
					nodes.add((Node)n);
			}
			else if (o instanceof Alarm)
			{
				Node n = (Node)session.findObjectById(((Alarm)o).getSourceObjectId(), Node.class);
				if (n != null)
					nodes.add(n);
			}
		}
		return nodes;
	}
	
	/**
	 * Check if tool is allowed for execution on each node from set
	 * 
	 * @param tool
	 * @param nodes
	 * @return
	 */
	private static boolean isToolAllowed(ObjectTool tool, Set<Node> nodes)
	{
		if (tool.getType() != ObjectTool.TYPE_INTERNAL)
			return true;
		
		ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
		if (handler != null)
		{
			for(Node n : nodes)
				if (!handler.canExecuteOnNode(n, tool))
					return false;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	/**
	 * Check if given tool is applicable for all nodes in set
	 * 
	 * @param tool
	 * @param nodes
	 * @return
	 */
	private static boolean isToolApplicable(ObjectTool tool, Set<Node> nodes)
	{
		for(Node n : nodes)
			if (!tool.isApplicableForNode(n))
				return false;
		return true;
	}
	
	/**
	 * Execute object tool on node set
	 * @param tool Object tool
	 */
	private void executeObjectTool(final Set<Node> nodes, final ObjectTool tool)
	{
		if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
		{
			String message = tool.getConfirmationText();
			if (nodes.size() == 1)
			{
				Node node = nodes.iterator().next();
				message = message.replace("%OBJECT_IP_ADDR%", node.getPrimaryIP().getHostAddress());
				message = message.replace("%OBJECT_NAME%", node.getObjectName());
				message = message.replace("%OBJECT_ID%", Long.toString(node.getObjectId()));
			}
			else
			{
				message = message.replace("%OBJECT_IP_ADDR%", "<multiple nodes>");
				message = message.replace("%OBJECT_NAME%", "<multiple nodes>");
				message = message.replace("%OBJECT_ID%", "<multiple nodes>");
			}
			if (!MessageDialogHelper.openQuestion(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), 
					"Confirm Tool Execution", message))
				return;
		}
		
		for(Node n : nodes)
			executeObjectToolOnNode(n, tool);
	}
	
	/**
	 * Execute object tool on single node
	 * 
	 * @param node
	 * @param tool
	 */
	private void executeObjectToolOnNode(final Node node, final ObjectTool tool)
	{
		switch(tool.getType())
		{
			case ObjectTool.TYPE_INTERNAL:
				executeInternalTool(node, tool);
				break;
			case ObjectTool.TYPE_LOCAL_COMMAND:
				executeLocalCommand(node, tool);
				break;
			case ObjectTool.TYPE_SERVER_COMMAND:
				executeServerCommand(node, tool);
				break;
			case ObjectTool.TYPE_ACTION:
				executeAgentAction(node, tool);
				break;
			case ObjectTool.TYPE_TABLE_AGENT:
			case ObjectTool.TYPE_TABLE_SNMP:
				executeTableTool(node, tool);
				break;
			case ObjectTool.TYPE_URL:
				openURL(node, tool);
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
			MessageDialogHelper.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
		}
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeAgentAction(final Node node, final ObjectTool tool)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Execute action on node " + node.getObjectName(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute action on node " + node.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.executeAction(node.getObjectId(), tool.getData());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						MessageDialogHelper.openInformation(null, "Tool Execution", "Action " + tool.getData() + " executed successfully on node " + node.getObjectName());
					}
				});
			}
		}.start();
	}

	/**
	 * Execute server command
	 * 
	 * @param node
	 * @param tool
	 */
	private void executeServerCommand(final Node node, final ObjectTool tool)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Execute server command", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.executeServerCommand(node.getObjectId(), tool.getData());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						MessageDialogHelper.openInformation(null, "Information", "Server command executed successfully");
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Canot execute command on server";
			}
		}.start();
	}
	
	/**
	 * Execute local command
	 * 
	 * @param node
	 * @param tool
	 */
	private void executeLocalCommand(final Node node, final ObjectTool tool)
	{
		String temp = tool.getData();
		temp = temp.replace("%OBJECT_IP_ADDR%", node.getPrimaryIP().getHostAddress());
		temp = temp.replace("%OBJECT_NAME%", node.getObjectName());
		String command = temp.replace("%OBJECT_ID%", Long.toString(node.getObjectId()));
		
		if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
		{
			final String os = Platform.getOS();
			
			try
			{
				if (os.equals(Platform.OS_WIN32))
				{
					command = "CMD.EXE /C START \"NetXMS\" " + command;
					Runtime.getRuntime().exec(command);
				}
				else
				{
					Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", command });
				}
			}
			catch(IOException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		else
		{
			final String secondaryId = Long.toString(node.getObjectId()) + "&" + Long.toString(tool.getId());
			final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
			try
			{
				LocalCommandResults view = (LocalCommandResults)window.getActivePage().showView(LocalCommandResults.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
				view.runCommand(command);
			}
			catch(Exception e)
			{
				MessageDialogHelper.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
			}
		}
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeFileDownload(final Node node, final ObjectTool tool)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		ConsoleJob job = new ConsoleJob("Download file from agent", null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot download file " + tool.getData() + " from node " + node.getObjectName();
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final File file = session.downloadFileFromAgent(node.getObjectId(), tool.getData());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
						try
						{
							String secondaryId = Long.toString(node.getObjectId()) + "&" + URLEncoder.encode(tool.getData(), "UTF-8");
							FileViewer view = (FileViewer)window.getActivePage().showView(FileViewer.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
							view.showFile(file);
						}
						catch(Exception e)
						{
							MessageDialogHelper.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
						}
					}
				});
			}
		};
		job.start();
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeInternalTool(final Node node, final ObjectTool tool)
	{
		ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
		if (handler != null)
		{
			handler.execute(node, tool);
		}
		else
		{
			MessageDialogHelper.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), "Error", "Cannot execute object tool: handler not defined");
		}
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void openURL(final Node node, final ObjectTool tool)
	{
		String temp = tool.getData();
		temp = temp.replace("%OBJECT_IP_ADDR%", node.getPrimaryIP().getHostAddress());
		temp = temp.replace("%OBJECT_NAME%", node.getObjectName());
		final String url = temp.replace("%OBJECT_ID%", Long.toString(node.getObjectId()));
		
		final String sid = Long.toString(node.getObjectId()) + "&" + Long.toString(tool.getId());
		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			BrowserView view = (BrowserView)window.getActivePage().showView(BrowserView.ID, sid, IWorkbenchPage.VIEW_ACTIVATE);
			view.openUrl(url);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(window.getShell(), "Error", "Cannot open web browser: " + e.getLocalizedMessage());
		}
	}
}
