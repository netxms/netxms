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
package org.netxms.nxmc.modules.objects.views;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.Dot1xPortComparator;
import org.netxms.nxmc.modules.objects.views.helpers.Dot1xPortFilter;
import org.netxms.nxmc.modules.objects.views.helpers.Dot1xPortListLabelProvider;
import org.netxms.nxmc.modules.objects.views.helpers.Dot1xPortSummary;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * View 802.1x status of ports
 */
public class Dot1xStatusView extends ObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(Dot1xStatusView.class);

	public static final int COLUMN_NODE = 0;
	public static final int COLUMN_PORT = 1;
	public static final int COLUMN_INTERFACE = 2;
	public static final int COLUMN_PAE_STATE = 3;
	public static final int COLUMN_BACKEND_STATE = 4;
	
	private SortableTableViewer viewer;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	
   /**
    * @param name
    * @param image
    */
   public Dot1xStatusView()
   {
      super(LocalizationHelper.getI18n(Dot1xStatusView.class).tr("802.1x"), ResourceManager.getImageDescriptor("icons/object-views/pae.png"), "objects.802dot1x-status", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		final String[] names = { i18n.tr("Device"), i18n.tr("Port"), i18n.tr("Interface"), i18n.tr("PAE State"), i18n.tr("Backend State") };
		final int[] widths = { 250, 60, 180, 150, 150 };
		viewer = new SortableTableViewer(parent, names, widths, 1, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new Dot1xPortListLabelProvider());
		viewer.setComparator(new Dot1xPortComparator());
		Dot1xPortFilter filter = new Dot1xPortFilter();
		setFilterClient(viewer, filter);
      viewer.addFilter(filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "Dot1xStatusView");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveTableViewerSettings(viewer, "Dot1xStatusView");
			}
		});

		createActions();
		createPopupMenu();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionExportToCsv);
      manager.add(actionExportAllToCsv);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      if (getObject() != null)
      {
         List<Dot1xPortSummary> portList = new ArrayList<Dot1xPortSummary>();
         Set<Long> nodeList = new HashSet<Long>();
         fillPortList(portList, nodeList, getObject());
         viewer.setInput(portList.toArray());
      }
      else
      {
         viewer.setInput(new Dot1xPortSummary[0]);
      }
	}

   /**
    * Fill port list
    *
    * @param portList list of ports
    * @param nodeList list of nodes
    * @param root root object
    */
   private void fillPortList(List<Dot1xPortSummary> portList, Set<Long> nodeList, AbstractObject root)
	{
      if ((root instanceof Node) && !nodeList.contains(root.getObjectId()))
		{
         for(AbstractObject o : root.getAllChildren(AbstractObject.OBJECT_INTERFACE))
			{
				if ((((Interface)o).getFlags() & Interface.IF_PHYSICAL_PORT) != 0)
               portList.add(new Dot1xPortSummary((Node)root, (Interface)o));
			}
         nodeList.add(root.getObjectId());
		}
		else
		{
         for(AbstractObject object : root.getChildrenAsArray())
            fillPortList(portList, nodeList, object);
		}
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if (context == null)
         return false;
      if (context instanceof Node)
         return ((Node)context).is8021xSupported();
      return (context instanceof Container) || (context instanceof Collector) || (context instanceof Cluster) || (context instanceof Rack) || (context instanceof Subnet) || (context instanceof Zone);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 290;
   }
}
