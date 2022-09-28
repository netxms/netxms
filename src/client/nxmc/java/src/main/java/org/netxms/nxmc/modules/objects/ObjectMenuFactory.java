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
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectPollerView;
import org.netxms.nxmc.tools.ImageCache;
import org.xnap.commons.i18n.I18n;

/**
 * Dynamic object tools menu creator
 */
public final class ObjectMenuFactory
{
   private static I18n i18n = LocalizationHelper.getI18n(ObjectMenuFactory.class);
   private static final String[] POLL_NAME = {
         "",
         i18n.tr("Status"), 
         i18n.tr("Configuration (Full)"),
         i18n.tr("Interface"), 
         i18n.tr("Topology"),
         i18n.tr("Configuration"), 
         i18n.tr("Instance discovery"),
         i18n.tr("Routing table"),
         i18n.tr("Network discovery"),
         i18n.tr("Automatic binding")
      };

   /**
    * Create poll menu for selected object
    *
    * @param selection selection of objects
    * @param parentMenu parent menu
    * @param parentControl parent control
    * @param viewPlacement view placement information (can be used for displaying messages and creating new views)
    * @return newly constructed menu
    */
   public static Menu createPollMenu(IStructuredSelection selection, Menu parentMenu, Control parentControl, final ViewPlacement viewPlacement)
	{
      if (((IStructuredSelection)selection).size() > 1)
         return null;

		final AbstractObject object = (AbstractObject)((IStructuredSelection)selection).getFirstElement();
      if (!(object instanceof PollingTarget))
         return null;

      final Menu menu = (parentMenu != null) ? new Menu(parentMenu) : new Menu(parentControl);
		if (object instanceof AbstractNode)
		{
         addPollMenuItem(menu, object, ObjectPollType.STATUS, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.CONFIGURATION_NORMAL, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.CONFIGURATION_FULL, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.INSTANCE_DISCOVERY, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.INTERFACES, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.TOPOLOGY, viewPlacement);
		}
      else if (object instanceof BusinessServicePrototype)
      {
         addPollMenuItem(menu, object, ObjectPollType.INSTANCE_DISCOVERY, viewPlacement);
      }
      else if (object instanceof BusinessService || object instanceof Cluster)
		{
         addPollMenuItem(menu, object, ObjectPollType.STATUS, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.CONFIGURATION_NORMAL, viewPlacement);
		}
      else if ((object instanceof Container) || (object instanceof Template))
      {
         addPollMenuItem(menu, object, ObjectPollType.AUTOBIND, viewPlacement);
      }
      else if (object instanceof Sensor)
      {
         addPollMenuItem(menu, object, ObjectPollType.STATUS, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.CONFIGURATION_NORMAL, viewPlacement);
         addPollMenuItem(menu, object, ObjectPollType.INSTANCE_DISCOVERY, viewPlacement);
      }

      if (menu.getItemCount() == 0)
		{
         menu.dispose();
         return null;
		}

      return menu;
	}

   /**
    * Add single menu item
    *
    * @param menu menu to add item to
    * @param object object to execute action on
    * @param type poll type
    * @param viewPlacement view placement information (can be used for displaying messages and creating new views)
    */
   public static void addPollMenuItem(final Menu menu, AbstractObject object, ObjectPollType type, ViewPlacement viewPlacement)
   {
      final MenuItem item = new MenuItem(menu, SWT.PUSH);
      item.setText(POLL_NAME[type.getValue()]);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            ObjectPollerView view = new ObjectPollerView(object, type);
            if (viewPlacement.getPerspective() != null)
            {
               viewPlacement.getPerspective().addMainView(view, true, false);
            }
            else
            {
               PopOutViewWindow window = new PopOutViewWindow(view);
               window.open();
            }
            view.startPoll();
         }
      });
   }

   /**
    * Create tools menu for given selection of objects
    *
    * @param selection selection of objects
    * @param parentMenu parent menu
    * @param viewPlacement view placement (can be used for displaying messages and selecting perspective for new views)
    * @return newly constructed menu
    */
   public static Menu createToolsMenu(IStructuredSelection selection, Menu parentMenu, Control parentControl, final ViewPlacement viewPlacement)
   {
      final Set<ObjectContext> objects = buildObjectSet((IStructuredSelection)selection);
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
      for(int i = 0; i < tools.length; i++)
      {
         boolean enabled = (tools[i].getFlags() & ObjectTool.DISABLED) == 0;
         if (enabled && ObjectToolExecutor.isToolAllowed(tools[i], objects) && ObjectToolExecutor.isToolApplicable(tools[i], objects))
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
                  ObjectToolExecutor.execute(objects, objects, (ObjectTool)item.getData(), viewPlacement);
               }
            });
         }
      }

      if (toolsMenu.getItemCount() == 0)
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
   private static Set<ObjectContext> buildObjectSet(IStructuredSelection selection)
   {
      final Set<ObjectContext> nodes = new HashSet<ObjectContext>();
      final NXCSession session = Registry.getSession();

      for(Object o : selection.toList())
      {
         if (o instanceof AbstractNode)
         {
            nodes.add(new ObjectContext((AbstractNode)o, null));
         }
         else if ((o instanceof AbstractObject) && ObjectTool.isContainerObject((AbstractObject)o))
         {
            nodes.add(new ObjectContext((AbstractObject)o, null));
            for(AbstractObject n : ((AbstractObject)o).getAllChildren(AbstractObject.OBJECT_NODE))
               nodes.add(new ObjectContext(n, null));
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
