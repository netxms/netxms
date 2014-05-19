/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.netxms.client.AgentFile;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
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

		final Set<NodeInfo> nodes = buildNodeSet((IStructuredSelection)selection);
		final Menu toolsMenu = new Menu(menu);
		
		ObjectTool[] tools = ObjectToolsCache.getInstance().getTools();
		Arrays.sort(tools, new Comparator<ObjectTool>() {
			@Override
			public int compare(ObjectTool arg0, ObjectTool arg1)
			{
				return arg0.getName().replace("&", "").compareToIgnoreCase(arg1.getName().replace("&", "")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
			}
		});
		
		Map<String, Menu> menus = new HashMap<String, Menu>();
		int added = 0;
		for(int i = 0; i < tools.length; i++)
		{
			boolean enabled = (tools[i].getFlags() & ObjectTool.DISABLED) == 0;
			if (enabled && isToolAllowed(tools[i], nodes) && isToolApplicable(tools[i], nodes))
			{
				String[] path = tools[i].getName().split("\\-\\>"); //$NON-NLS-1$
			
				Menu rootMenu = toolsMenu;
				for(int j = 0; j < path.length - 1; j++)
				{
               final String key = rootMenu.hashCode() + "@" + path[j].replace("&", ""); //$NON-NLS-1$ //$NON-NLS-2$
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
			toolsMenuItem.setText(Messages.get().ObjectToolsDynamicMenu_TopLevelLabel);
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
	private Set<NodeInfo> buildNodeSet(IStructuredSelection selection)
	{
		final Set<NodeInfo> nodes = new HashSet<NodeInfo>();
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		for(Object o : selection.toList())
		{
			if (o instanceof AbstractNode)
			{
				nodes.add(new NodeInfo((AbstractNode)o, null));
			}
			else if ((o instanceof Container) || (o instanceof ServiceRoot) || (o instanceof Subnet) || (o instanceof Cluster))
			{
				for(AbstractObject n : ((AbstractObject)o).getAllChilds(AbstractObject.OBJECT_NODE))
					nodes.add(new NodeInfo((AbstractNode)n, null));
			}
			else if (o instanceof Alarm)
			{
				AbstractNode n = (AbstractNode)session.findObjectById(((Alarm)o).getSourceObjectId(), AbstractNode.class);
				if (n != null)
					nodes.add(new NodeInfo(n, (Alarm)o));
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
	private static boolean isToolAllowed(ObjectTool tool, Set<NodeInfo> nodes)
	{
		if (tool.getType() != ObjectTool.TYPE_INTERNAL)
			return true;
		
		ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
		if (handler != null)
		{
			for(NodeInfo n : nodes)
				if (!handler.canExecuteOnNode(n.object, tool))
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
	private static boolean isToolApplicable(ObjectTool tool, Set<NodeInfo> nodes)
	{
		for(NodeInfo n : nodes)
			if (!tool.isApplicableForNode(n.object))
				return false;
		return true;
	}
	
	/**
	 * Execute object tool on node set
	 * @param tool Object tool
	 */
	private void executeObjectTool(final Set<NodeInfo> nodes, final ObjectTool tool)
	{
		if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
		{
			String message = tool.getConfirmationText();
			if (nodes.size() == 1)
			{
			   NodeInfo node = nodes.iterator().next();
				message = message.replace("%OBJECT_IP_ADDR%", node.object.getPrimaryIP().getHostAddress()); //$NON-NLS-1$
				message = message.replace("%OBJECT_NAME%", node.object.getObjectName()); //$NON-NLS-1$
				message = message.replace("%OBJECT_ID%", Long.toString(node.object.getObjectId())); //$NON-NLS-1$
			}
			else
			{
				message = message.replace("%OBJECT_IP_ADDR%", Messages.get().ObjectToolsDynamicMenu_MultipleNodes); //$NON-NLS-1$
				message = message.replace("%OBJECT_NAME%", Messages.get().ObjectToolsDynamicMenu_MultipleNodes); //$NON-NLS-1$
				message = message.replace("%OBJECT_ID%", Messages.get().ObjectToolsDynamicMenu_MultipleNodes); //$NON-NLS-1$
			}
			if (!MessageDialogHelper.openQuestion(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), 
					Messages.get().ObjectToolsDynamicMenu_ConfirmExec, message))
				return;
		}
		
		for(NodeInfo n : nodes)
			executeObjectToolOnNode(n, tool);
	}
	
	/**
	 * Execute object tool on single node
	 * 
	 * @param node
	 * @param tool
	 */
	private void executeObjectToolOnNode(final NodeInfo node, final ObjectTool tool)
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
	private void executeTableTool(final NodeInfo node, final ObjectTool tool)
	{
		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			final IWorkbenchPage page = window.getActivePage();
			final TableToolResults view = (TableToolResults)page.showView(TableToolResults.ID,
					Long.toString(tool.getId()) + "&" + Long.toString(node.object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE); //$NON-NLS-1$
			view.refreshTable();
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
		}
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeAgentAction(final NodeInfo node, final ObjectTool tool)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final String action = substituteMacros(tool.getData(), node);
		new ConsoleJob(String.format(Messages.get().ObjectToolsDynamicMenu_ExecuteOnNode, node.object.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().ObjectToolsDynamicMenu_CannotExecuteOnNode, node.object.getObjectName());
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.executeAction(node.object.getObjectId(), action);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_ToolExecution, String.format(Messages.get().ObjectToolsDynamicMenu_ExecSuccess, action, node.object.getObjectName()));
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
	private void executeServerCommand(final NodeInfo node, final ObjectTool tool)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().ObjectToolsDynamicMenu_ExecuteServerCmd, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.executeServerCommand(node.object.getObjectId(), tool.getData());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_Information, Messages.get().ObjectToolsDynamicMenu_ServerCommandExecuted);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ObjectToolsDynamicMenu_ServerCmdExecError;
			}
		}.start();
	}
	
	/**
	 * Execute local command
	 * 
	 * @param node
	 * @param tool
	 */
	private void executeLocalCommand(final NodeInfo node, final ObjectTool tool)
	{
		String command = substituteMacros(tool.getData(), node);
		
		if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
		{
			final String os = Platform.getOS();
			
			try
			{
				if (os.equals(Platform.OS_WIN32))
				{
					command = "CMD.EXE /C START \"NetXMS\" " + command; //$NON-NLS-1$
					Runtime.getRuntime().exec(command);
				}
				else
				{
					Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", command }); //$NON-NLS-1$ //$NON-NLS-2$
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
			final String secondaryId = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
			final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
			try
			{
				LocalCommandResults view = (LocalCommandResults)window.getActivePage().showView(LocalCommandResults.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
				view.runCommand(command);
			}
			catch(Exception e)
			{
				MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
			}
		}
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void executeFileDownload(final NodeInfo node, final ObjectTool tool)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		String[] parameters = tool.getData().split("\u007F"); //$NON-NLS-1$
		
		final String fileName = parameters[0];
		final int maxFileSize = Integer.parseInt(parameters[1]);
		final boolean follow = parameters[2].equals("true") ? true : false; //$NON-NLS-1$
		
		ConsoleJob job = new ConsoleJob(Messages.get().ObjectToolsDynamicMenu_DownloadFromAgent, null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().ObjectToolsDynamicMenu_DownloadError, fileName, node.object.getObjectName());
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            final AgentFile file = session.downloadFileFromAgent(node.object.getObjectId(), fileName, maxFileSize, follow);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
						try
						{
							String secondaryId = Long.toString(node.object.getObjectId()) + "&" + URLEncoder.encode(fileName, "UTF-8"); //$NON-NLS-1$ //$NON-NLS-2$
                     FileViewer view = (FileViewer)window.getActivePage().showView(FileViewer.ID, secondaryId,
                           IWorkbenchPage.VIEW_ACTIVATE);
                     view.showFile(file.getFile(), follow, file.getId(), maxFileSize);
						}
						catch(Exception e)
						{
							MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
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
	private void executeInternalTool(final NodeInfo node, final ObjectTool tool)
	{
		ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
		if (handler != null)
		{
			handler.execute(node.object, tool);
		}
		else
		{
			MessageDialogHelper.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), Messages.get().ObjectToolsDynamicMenu_Error, Messages.get().ObjectToolsDynamicMenu_HandlerNotDefined);
		}
	}

	/**
	 * @param node
	 * @param tool
	 */
	private void openURL(final NodeInfo node, final ObjectTool tool)
	{
		final String url = substituteMacros(tool.getData(), node);
		
		final String sid = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			BrowserView view = (BrowserView)window.getActivePage().showView(BrowserView.ID, sid, IWorkbenchPage.VIEW_ACTIVATE);
			view.openUrl(url);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, Messages.get().ObjectToolsDynamicMenu_CannotOpenWebBrowser + e.getLocalizedMessage());
		}
	}
	
	/**
	 * Substitute macros in string
	 * 
	 * @param s
	 * @param node
	 * @return
	 */
	private static String substituteMacros(String s, NodeInfo node)
	{
		StringBuilder sb = new StringBuilder();
		
		char[] src = s.toCharArray();
		for(int i = 0; i < s.length(); i++)
		{
			if (src[i] == '%')
			{
				StringBuilder p = new StringBuilder();
				for(i++; src[i] != '%' && i < s.length(); i++)
					p.append(src[i]);
				if (p.length() == 0)		// %%
				{
					sb.append('%');
				}
				else
				{
					String name = p.toString();
					if (name.equals("OBJECT_IP_ADDR")) //$NON-NLS-1$
					{
						sb.append(node.object.getPrimaryIP().getHostAddress());
					}
					else if (name.equals("OBJECT_NAME")) //$NON-NLS-1$
					{
						sb.append(node.object.getObjectName());
					}
					else if (name.equals("OBJECT_ID")) //$NON-NLS-1$
					{
						sb.append(node.object.getObjectId());
					}
               else if (name.equals("ALARM_ID")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getId());
               }
               else if (name.equals("ALARM_MESSAGE")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getMessage());
               }
               else if (name.equals("ALARM_SEVERITY")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getCurrentSeverity());
               }
               else if (name.equals("ALARM_SEVERITY_TEXT")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(StatusDisplayInfo.getStatusText(node.alarm.getCurrentSeverity()));
               }
               else if (name.equals("ALARM_STATE")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getState());
               }
					else
					{
						String custAttr = node.object.getCustomAttributes().get(name);
						if (custAttr != null)
							sb.append(custAttr);
					}
				}
			}
			else
			{
				sb.append(src[i]);
			}
		}
		
		return sb.toString();
	}
	
	/**
	 * Class to hold information about selected node
	 */
	private class NodeInfo
	{
	   AbstractNode object;
	   Alarm alarm;
	   
      NodeInfo(AbstractNode object, Alarm alarm)
      {
         this.object = object;
         this.alarm = alarm;
      }

      /* (non-Javadoc)
       * @see java.lang.Object#hashCode()
       */
      @Override
      public int hashCode()
      {
         final int prime = 31;
         int result = 1;
         result = prime * result + ObjectToolsDynamicMenu.this.hashCode();
         result = prime * result + ((alarm == null) ? 0 : alarm.hashCode());
         result = prime * result + ((object == null) ? 0 : object.hashCode());
         return result;
      }

      /* (non-Javadoc)
       * @see java.lang.Object#equals(java.lang.Object)
       */
      @Override
      public boolean equals(Object obj)
      {
         if (this == obj)
            return true;
         if (obj == null)
            return false;
         if (getClass() != obj.getClass())
            return false;
         NodeInfo other = (NodeInfo)obj;
         if ((other.object == null) || (this.object == null))
            return (other.object == null) && (this.object == null);
         return other.object.getObjectId() == this.object.getObjectId();
      }
	}
}
