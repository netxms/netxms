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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.MacAddress;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ArpCacheEntry;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.ArpCacheComparator;
import org.netxms.nxmc.modules.objects.views.helpers.ArpCacheFilter;
import org.netxms.nxmc.modules.objects.views.helpers.ArpCacheLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * ARP cache view
 */
public class ArpCacheView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ArpCacheView.class);

   public static final int COLUMN_IP_ADDRESS = 0;
   public static final int COLUMN_MAC_ADDRESS = 1;
   public static final int COLUMN_VENDOR = 2;
	public static final int COLUMN_INTERFACE = 3;
   public static final int COLUMN_NODE = 4;

	private SortableTableViewer viewer;
   private boolean refreshPending = true;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionCopyRowToClipboard;
   private Action actionCopyMACToClipboard;

   /**
    * Default constructor
    */
   public ArpCacheView()
   {
      super(LocalizationHelper.getI18n(ArpCacheView.class).tr("ARP Cache"), ResourceManager.getImageDescriptor("icons/object-views/fdb.gif"), "objects.arp-cache", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && (((Node)context).hasAgent() || ((Node)context).hasSnmpAgent() || ((Node)context).isManagementServer());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 155;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{	   
		final String[] names = { 
            i18n.tr("IP Address"), i18n.tr("MAC Address"), i18n.tr("NIC Vendor"), i18n.tr("Interface"), i18n.tr("Node")
		   };
      final int[] widths = { 180, 180, 200, 200, 250 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_IP_ADDRESS, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ArpCacheLabelProvider(viewer));
      viewer.setComparator(new ArpCacheComparator());
      ArpCacheFilter filter = new ArpCacheFilter();
		setFilterClient(viewer, filter);
		viewer.addFilter(filter);

		createActions();
		createPopupMenu();
	}

	/**
	 * Create actions
	 */
   private void createActions()
	{
      actionCopyRowToClipboard = new CopyTableRowsAction(viewer, true);
      actionCopyMACToClipboard = new Action(i18n.tr("Copy MAC address to clipboard")) {
         @Override
         public void run()
         {
            @SuppressWarnings("unchecked")
            final List<ArpCacheEntry> selection = viewer.getStructuredSelection().toList();
            if (selection.size() > 0)
            {
               final StringBuilder sb = new StringBuilder();
               for(int i = 0; i < selection.size(); i++)
               {
                  if (i > 0)
                     sb.append('\t');

                  MacAddress addr = selection.get(i).getMacAddress();
                  sb.append(addr != null ? addr.toString() : "");
               }
               WidgetHelper.copyToClipboard(sb.toString());
            }
         }
      };
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
      manager.add(actionCopyRowToClipboard);
      manager.add(actionCopyMACToClipboard);
      manager.add(actionExportToCsv);
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
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
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

      new Job(i18n.tr("Reading ARP cache"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ArpCacheEntry> fdb = session.getArpCache(objectId, true);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(fdb.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot get ARP cache for node %s"), session.getObjectName(objectId));
         }
      }.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      if (refreshPending)
      {
         refreshPending = false;
         refresh();
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      viewer.setInput(new Object[0]);
      if (isActive())
         refresh();
      else
         refreshPending = true;
   }
}
