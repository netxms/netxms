/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.tools.ImageCache;

/**
 * Dynamic object tools menu creator
 */
public class ObjectToolsMenuFactory
{
   /**
    * Create tools menu for given selection of objects
    *
    * @param selection selection of objects
    * @param parentMenu parent menu
    * @param window owning window (can be used for displaying messages)
    * @return newly constructed menu
    */
   public static Menu createMenu(IStructuredSelection selection, Menu parentMenu, Control parentControl, final Window window)
	{
		final Set<ObjectContext> nodes = buildNodeSet((IStructuredSelection)selection);
      final Menu toolsMenu = (parentMenu != null) ? new Menu(parentMenu) : new Menu(parentControl);

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
               item.setImage(imageCache.create(icon));
				item.setData(tools[i]);
				item.addSelectionListener(new SelectionAdapter() {
					@Override
					public void widgetSelected(SelectionEvent e)
					{
                  ObjectToolExecutor.execute(nodes, (ObjectTool)item.getData(), window);
					}
				});
				
				added++;
			}
		}

      if (added == 0)
		{
			toolsMenu.dispose();
         return null;
		}

      return toolsMenu;
	}
	
	/**
	 * Build node set from selection
	 * 
	 * @param selection
	 * @return
	 */
   private static Set<ObjectContext> buildNodeSet(IStructuredSelection selection)
	{
		final Set<ObjectContext> nodes = new HashSet<ObjectContext>();
      final NXCSession session = Registry.getSession();
		
		for(Object o : selection.toList())
		{
			if (o instanceof AbstractNode)
			{
				nodes.add(new ObjectContext((AbstractNode)o, null));
			}
			else if ((o instanceof Container) || (o instanceof ServiceRoot) || (o instanceof Subnet) || (o instanceof Cluster))
			{
				for(AbstractObject n : ((AbstractObject)o).getAllChildren(AbstractObject.OBJECT_NODE))
					nodes.add(new ObjectContext((AbstractNode)n, null));
			}
			else if (o instanceof Alarm)
			{
				AbstractNode n = (AbstractNode)session.findObjectById(((Alarm)o).getSourceObjectId(), AbstractNode.class);
				if (n != null)
					nodes.add(new ObjectContext(n, (Alarm)o));
			} 
			else if (o instanceof ObjectWrapper)
			{
			   AbstractObject n = ((ObjectWrapper)o).getObject();
			   if ((n != null) && (n instanceof AbstractNode))
			      nodes.add(new ObjectContext((AbstractNode)n, null));
			}
		}
		return nodes;
	}
}
