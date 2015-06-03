/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.action.ContributionItem;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.ui.ISources;
import org.eclipse.ui.menus.IWorkbenchContribution;
import org.eclipse.ui.services.IEvaluationService;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objects.ObjectWrapper;
import org.netxms.ui.eclipse.objecttools.api.NodeInfo;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolExecutor;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolsCache;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ImageCache;

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
		
		final ImageCache imageCache = new ImageCache();
		toolsMenu.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            imageCache.dispose();
         }
      });
		
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
			if (enabled && ObjectToolExecutor.isToolAllowed(tools[i], nodes) && ObjectToolExecutor.isToolApplicable(tools[i], nodes))
			{
				String[] path = tools[i].getName().split("\\-\\>"); //$NON-NLS-1$
			
				Menu rootMenu = toolsMenu;
				for(int j = 0; j < path.length - 1; j++)
				{
               final String key = rootMenu.hashCode() + "@" + path[j].replace("&", ""); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
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
				ImageDescriptor icon = ObjectToolsCache.getInstance().findIcon(tools[i].getId());
				if (icon != null)
				   item.setImage(imageCache.add(icon));
				item.setData(tools[i]);
				item.addSelectionListener(new SelectionAdapter() {
					@Override
					public void widgetSelected(SelectionEvent e)
					{
					   ObjectToolExecutor.execute(nodes, (ObjectTool)item.getData());
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
			else if (o instanceof ObjectWrapper)
			{
			   AbstractObject n = ((ObjectWrapper)o).getObject();
			   if ((n != null) && (n instanceof AbstractNode))
			      nodes.add(new NodeInfo((AbstractNode)n, null));
			}
		}
		return nodes;
	}
}
