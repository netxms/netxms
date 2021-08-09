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

import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.netxms.client.constants.NodePollType;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.modules.objects.views.ObjectPollerView;

/**
 * Dynamic object tools menu creator
 */
public class ObjectPollsMenuFactory
{
   
   /**
    * Create poll menu for selected object
    *
    * @param selection selection of objects
    * @param parentMenu parent menu
    * @param window owning window (can be used for displaying messages)
    * @param p perspective if exists
    * @return newly constructed menu
    */
   public static Menu createMenu(IStructuredSelection selection, Menu parentMenu, Control parentControl, final Window window, final Perspective p)
	{
      if (((IStructuredSelection)selection).size() > 1)
         return null;
      
		final AbstractObject object = (AbstractObject)((IStructuredSelection)selection).getFirstElement();
      if (!(object instanceof PollingTarget))
         return null;
      
      final Menu pollsMenu = (parentMenu != null) ? new Menu(parentMenu) : new Menu(parentControl);
		boolean added = false;		
		if (object instanceof AbstractNode)
		{
		   addMenuItem(pollsMenu, object, NodePollType.STATUS, p);
         addMenuItem(pollsMenu, object, NodePollType.CONFIGURATION_NORMAL, p);
         addMenuItem(pollsMenu, object, NodePollType.CONFIGURATION_FULL, p);
         addMenuItem(pollsMenu, object, NodePollType.INSTANCE_DISCOVERY, p);
         addMenuItem(pollsMenu, object, NodePollType.INTERFACES, p);
         addMenuItem(pollsMenu, object, NodePollType.TOPOLOGY, p);
         added = true;
		}
      else if (object instanceof BusinessServicePrototype)
      {
         addMenuItem(pollsMenu, object, NodePollType.INSTANCE_DISCOVERY, p);   
         added = true;
      }
		else if (object instanceof Cluster || object instanceof BusinessService)
		{
         addMenuItem(pollsMenu, object, NodePollType.STATUS, p);
         addMenuItem(pollsMenu, object, NodePollType.CONFIGURATION_NORMAL, p);	
         added = true;	   
		}
      else if (object instanceof Sensor)
      {
         addMenuItem(pollsMenu, object, NodePollType.STATUS, p);
         addMenuItem(pollsMenu, object, NodePollType.CONFIGURATION_NORMAL, p);     
         addMenuItem(pollsMenu, object, NodePollType.INSTANCE_DISCOVERY, p);   
         added = true;
      }
		
      if (!added)
		{
         pollsMenu.dispose();
         return null;
		}

      return pollsMenu;
	}
   
   public static void addMenuItem(final Menu pollsMenu, AbstractObject object, NodePollType type, Perspective p)
   {
      final MenuItem item = new MenuItem(pollsMenu, SWT.PUSH);
      item.setText(ObjectPollerView.POLL_NAME[type.getValue()]);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            ObjectPollerView view = new ObjectPollerView(object, type);
            if (p != null)
            {
               p.addMainView(view, true, false);
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
}
