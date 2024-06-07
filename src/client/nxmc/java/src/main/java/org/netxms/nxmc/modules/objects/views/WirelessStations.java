/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.topology.WirelessStation;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.WirelessStationComparator;
import org.netxms.nxmc.modules.objects.views.helpers.WirelessStationFilter;
import org.netxms.nxmc.modules.objects.views.helpers.WirelessStationLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * List of wireless stations
 */
public class WirelessStations extends NodeSubObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(WirelessStations.class);

   public static final int COLUMN_MAC_ADDRESS = 0;
   public static final int COLUMN_VENDOR = 1;
   public static final int COLUMN_IP_ADDRESS = 2;
   public static final int COLUMN_NODE_NAME = 3;
   public static final int COLUMN_ACCESS_POINT = 4;
   public static final int COLUMN_RADIO = 5;
   public static final int COLUMN_SSID = 6;
   public static final int COLUMN_RSSI = 7;

	private SortableTableViewer viewer;
	private Action actionCopyRecord;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;

   /**
    * Constructor
    */
   public WirelessStations()
   {
      super("Wireless Stations", ResourceManager.getImageDescriptor("icons/object-views/wireless-stations.png"), "objects.wireless-stations", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 55;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && 
            ((context instanceof AbstractNode) && (((AbstractNode)context).isWirelessAccessPoint() || ((AbstractNode)context).isWirelessController()) || 
            (context instanceof AccessPoint));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(Composite parent)
	{
      super.createContent(parent);

      final String[] names = { i18n.tr("MAC Address"), i18n.tr("NIC Vendor"), i18n.tr("IP Address"), i18n.tr("Node"), i18n.tr("Access Point"), i18n.tr("Radio"), "SSID", "RSSI" };
      final int[] widths = { 140, 200, 100, 180, 180, 100, 100, 80 };
      viewer = new SortableTableViewer(mainArea, names, widths, 1, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new WirelessStationLabelProvider(viewer));
		viewer.setComparator(new WirelessStationComparator());

      WirelessStationFilter filter = new WirelessStationFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "WirelessStations.V2");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveTableViewerSettings(viewer, "WirelessStations.V2");
			}
		});

		createActions();
      createContextMenu();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionCopyRecord = new Action(i18n.tr("&Copy"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyToClipboard(-1);
         }
      };

		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionExportAllToCsv);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExportAllToCsv);
	}
	
	/**
    * Create context menu
    */
   private void createContextMenu()
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
      manager.add(actionCopyRecord);
		manager.add(actionExportToCsv);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (isActive())
         refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      final long objectId = getObjectId();
      if (objectId == 0)
         return;

      clearMessages();
      new Job(i18n.tr("Reading list of wireless stations"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            final List<WirelessStation> stations = session.getWirelessStations(objectId);
            runInUIThread(() -> viewer.setInput(stations.toArray()));
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot read list of wireless stations");
			}
		}.start();
	}

   /**
    * Copy content to clipboard
    * 
    * @param column column number or -1 to copy all columns
    */
   private void copyToClipboard(int column)
   {
      final TableItem[] selection = viewer.getTable().getSelection();
      if (selection.length > 0)
      {
         final String newLine = WidgetHelper.getNewLineCharacters();
         final StringBuilder sb = new StringBuilder();
         for(int i = 0; i < selection.length; i++)
         {
            if (i > 0)
               sb.append(newLine);
            if (column == -1)
            {
               for(int j = 0; j < viewer.getTable().getColumnCount(); j++)
               {
                  if (j > 0)
                     sb.append('\t');
                  sb.append(selection[i].getText(j));
               }
            }
            else
            {
               sb.append(selection[i].getText(column));
            }
         }
         WidgetHelper.copyToClipboard(sb.toString());
      }
   }
}
