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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.OSPFInfo;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.OSPFAreaComparator;
import org.netxms.nxmc.modules.objects.views.helpers.OSPFAreaLabelProvider;
import org.netxms.nxmc.modules.objects.views.helpers.OSPFNeighborComparator;
import org.netxms.nxmc.modules.objects.views.helpers.OSPFNeighborLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Switch forwarding database view
 */
public class OSPFView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(OSPFView.class);

   public static final int COLUMN_AREA_ID = 0;
   public static final int COLUMN_AREA_LSA = 1;
   public static final int COLUMN_AREA_ABR = 2;
   public static final int COLUMN_AREA_ASBR = 3;

   public static final int COLUMN_NEIGHBOR_ROUTER_ID = 0;
   public static final int COLUMN_NEIGHBOR_IP_ADDRESS = 1;
   public static final int COLUMN_NEIGHBOR_NODE = 2;
   public static final int COLUMN_NEIGHBOR_IF_INDEX = 3;
   public static final int COLUMN_NEIGHBOR_INTERFACE = 4;
   public static final int COLUMN_NEIGHBOR_VIRTUAL = 5;
   public static final int COLUMN_NEIGHBOR_AREA_ID = 6;
   public static final int COLUMN_NEIGHBOR_STATE = 7;

   private SortableTableViewer viewerAreas;
   private SortableTableViewer viewerNeighbors;
   private Action actionAreasExportToCsv;
   private Action actionAreasExportAllToCsv;
   private Action actionAreasCopyRowToClipboard;
   private Action actionNeighborsExportToCsv;
   private Action actionNeighborsExportAllToCsv;
   private Action actionNeighborsCopyRowToClipboard;

   /**
    * Default constructor
    */
   public OSPFView()
   {
      super(LocalizationHelper.getI18n(OSPFView.class).tr("OSPF"), ResourceManager.getImageDescriptor("icons/object-views/ospf.png"), "objects.ospf", false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && ((Node)context).isOSPF();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 190;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      parent.setLayout(layout);

      createAreasSection(parent);
      createNeighborsSection(parent);

		createActions();
		createPopupMenu();
	}

   /**
    * Create "Areas" section
    *
    * @param parent parent composite
    */
   private void createAreasSection(Composite parent)
   {
      Section section = new Section(parent, i18n.tr("Areas"), false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 1, 1));

      final String[] names = { i18n.tr("Area"), i18n.tr("LSA"), i18n.tr("ABR"), i18n.tr("ASBR") };
      final int[] widths = { 120, 90, 90, 90 };
      viewerAreas = new SortableTableViewer(section.getClient(), names, widths, COLUMN_AREA_ID, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
      viewerAreas.setContentProvider(new ArrayContentProvider());
      viewerAreas.setLabelProvider(new OSPFAreaLabelProvider());
      viewerAreas.setComparator(new OSPFAreaComparator());
   }

   /**
    * Create "Neighbors" section
    *
    * @param parent parent composite
    */
   private void createNeighborsSection(Composite parent)
   {
      Section section = new Section(parent, i18n.tr("Neighbors"), false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 2, 1));

      final String[] names = { i18n.tr("Router ID"), i18n.tr("IP address"), i18n.tr("Node"), i18n.tr("ifIndex"), i18n.tr("Interface"), i18n.tr("Virtual"), i18n.tr("Area"), i18n.tr("State") };
      final int[] widths = { 120, 120, 200, 90, 200, 60, 120, 150 };
      viewerNeighbors = new SortableTableViewer(section.getClient(), names, widths, COLUMN_NEIGHBOR_ROUTER_ID, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
      viewerNeighbors.setContentProvider(new ArrayContentProvider());
      viewerNeighbors.setLabelProvider(new OSPFNeighborLabelProvider());
      viewerNeighbors.setComparator(new OSPFNeighborComparator());
   }

	/**
	 * Create actions
	 */
   private void createActions()
	{
      actionAreasCopyRowToClipboard = new CopyTableRowsAction(viewerAreas, true);
      actionAreasExportToCsv = new ExportToCsvAction(this, viewerAreas, true);
      actionAreasExportAllToCsv = new ExportToCsvAction(this, viewerAreas, false);

      actionNeighborsCopyRowToClipboard = new CopyTableRowsAction(viewerNeighbors, true);
      actionNeighborsExportToCsv = new ExportToCsvAction(this, viewerNeighbors, true);
      actionNeighborsExportAllToCsv = new ExportToCsvAction(this, viewerNeighbors, false);
	}

	/**
	 * Create pop-up menu
	 */
   private void createPopupMenu()
	{
		MenuManager manager = new MenuManager();
		manager.setRemoveAllWhenShown(true);
		manager.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
            mgr.add(actionAreasCopyRowToClipboard);
            mgr.add(actionAreasExportToCsv);
            mgr.add(actionAreasExportAllToCsv);
			}
		});
      Menu menu = manager.createContextMenu(viewerAreas.getControl());
      viewerAreas.getControl().setMenu(menu);

      manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            mgr.add(actionNeighborsCopyRowToClipboard);
            mgr.add(actionNeighborsExportToCsv);
            mgr.add(actionNeighborsExportAllToCsv);
         }
      });
      menu = manager.createContextMenu(viewerNeighbors.getControl());
      viewerNeighbors.getControl().setMenu(menu);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
	@Override
   public void refresh()
	{
      if (getObject() == null)
         return;

      final AbstractObject object = getObject();
      new Job(i18n.tr("Reading OSPF information"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final OSPFInfo info = session.getOSPFInfo(object.getObjectId());
            session.syncChildren(object);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewerAreas.setInput(info.getAreas());
                  viewerNeighbors.setInput(info.getNeighbors());
                  viewerAreas.packColumns();
                  viewerNeighbors.packColumns();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot read OSPF information for node %s"), getObject().getObjectName());
         }
      }.start();
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
}
