/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.VPNConnector;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.VPNConnectorFilter;
import org.netxms.nxmc.modules.objects.views.helpers.VPNConnectorListComparator;
import org.netxms.nxmc.modules.objects.views.helpers.VPNConnectorListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "VPNs" tab
 */
public class VpnView extends NodeSubObjectTableView
{
   private final I18n i18n = LocalizationHelper.getI18n(VpnView.class);
   
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
   public static final int COLUMN_STATUS = 2;
	public static final int COLUMN_PEER_GATEWAY = 3;
	public static final int COLUMN_LOCAL_SUBNETS = 4;
	public static final int COLUMN_REMOTE_SUBNETS = 5;

	private VPNConnectorListLabelProvider labelProvider;

   /**
    * Create "VPNs" view
    */
   public VpnView()
   {
      super(LocalizationHelper.getI18n(VpnView.class).tr("VPNs"), ResourceManager.getImageDescriptor("icons/object-views/vpn.png"), "objects.vpns", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if ((context != null) && (context instanceof AbstractNode))
      {
         return ((AbstractNode)context).hasVpnConnectors();
      }
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 60;
   }

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionCopyToClipboard);
		manager.add(actionExportToCsv);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#refresh()
    */
   @Override
   public void refresh()
	{
		if (getObject() != null)
			viewer.setInput(getObject().getAllChildren(AbstractObject.OBJECT_VPNCONNECTOR).toArray());
		else
			viewer.setInput(new VPNConnector[0]);
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentViewerTab#createViewer()
    */
   @Override
   protected void createViewer()
   {
      final String[] names = { i18n.tr("Id"), i18n.tr("Name"), i18n.tr("Status"), i18n.tr("Peer gateway"), i18n.tr("Local subnets"), i18n.tr("Remote subnets") };

      final int[] widths = { 60, 150, 80, 150, 200, 200 };
      viewer = new SortableTableViewer(mainArea, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      labelProvider = new VPNConnectorListLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new VPNConnectorListComparator(labelProvider));
      viewer.getTable().setHeaderVisible(true);
      viewer.getTable().setLinesVisible(true);
      filter = new VPNConnectorFilter(labelProvider);
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, "VPNTable.V1"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), "VPNTable.V1"); //$NON-NLS-1$
         }
      });   
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return (object instanceof VPNConnector) && object.isChildOf(getObjectId());
   }
}
