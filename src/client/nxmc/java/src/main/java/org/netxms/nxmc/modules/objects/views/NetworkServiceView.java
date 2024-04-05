/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.netxms.client.objects.NetworkService;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.NetworkServiceFilter;
import org.netxms.nxmc.modules.objects.views.helpers.NetworkServiceListComparator;
import org.netxms.nxmc.modules.objects.views.helpers.NetworkServiceListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
/**
 * "NetworkServices" tab
 */
public class NetworkServiceView extends NodeSubObjectTableView
{
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
   public static final int COLUMN_STATUS = 2;
	public static final int COLUMN_SERVICE_TYPE = 3;
	public static final int COLUMN_ADDRESS = 4;
   public static final int COLUMN_PORT = 5;
   public static final int COLUMN_REQUEST = 6;
   public static final int COLUMN_RESPONSE = 7;
   public static final int COLUMN_POLLER_NODE = 8;
   public static final int COLUMN_POLL_COUNT = 9;

	private NetworkServiceListLabelProvider labelProvider;

   /**
    * Create "Network Services" view
    */
   public NetworkServiceView()
   {
      super(LocalizationHelper.getI18n(NetworkServiceView.class).tr("Network Services"), ResourceManager.getImageDescriptor("icons/object-views/network_service.png"), "objects.network-services", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if ((context != null) && (context instanceof AbstractNode))
      {
         return ((AbstractNode)context).hasNetworkServices();
      }
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 70;
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
         viewer.setInput(getObject().getAllChildren(AbstractObject.OBJECT_NETWORKSERVICE).toArray());
      else
         viewer.setInput(new NetworkService[0]);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.NodeComponentViewerTab#createViewer()
    */
   @Override
   protected void createViewer()
   {
      final String[] names = { "Id", "Name", "Status", "Service type", "Address", "Port", "Request", "Response", "Poller node", "Poll count" };
      
      final int[] widths = { 60, 150, 80, 80, 80, 80, 200, 200, 200, 80 };
      viewer = new SortableTableViewer(mainArea, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      labelProvider = new NetworkServiceListLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new NetworkServiceListComparator(labelProvider));
      viewer.getTable().setHeaderVisible(true);
      viewer.getTable().setLinesVisible(true);
      filter = new NetworkServiceFilter(labelProvider);
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, "NetworkService.V1"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), "NetworkService.V1"); //$NON-NLS-1$
         }
      });   
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return (object instanceof NetworkService) && object.isChildOf(getObjectId());
   }
}
