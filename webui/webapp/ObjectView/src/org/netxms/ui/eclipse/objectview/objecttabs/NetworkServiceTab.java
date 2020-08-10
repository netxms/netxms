/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs;

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkService;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.NetworkServiceListComparator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.NetworkServiceListLabelProvider;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.NetworkServiceTabFilter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "NetworkServices" tab
 */
public class NetworkServiceTab extends NodeComponentViewerTab
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
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		ObjectContextMenu.fill(manager, getViewPart().getSite(), viewer);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		if (getObject() != null)
			viewer.setInput(getObject().getAllChildren(AbstractObject.OBJECT_NETWORKSERVICE).toArray());
		else
			viewer.setInput(new NetworkService[0]);
	}

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
      filter = new NetworkServiceTabFilter(labelProvider);
      viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "NetworkService.V1"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "NetworkService.V1"); //$NON-NLS-1$
         }
      });
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText, 0, SWT.BOTTOM);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);      
   }

   @Override
   public String getFilterSettingName()
   {
      return "NetworkServiceTab.showFilter";
   }

   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return object instanceof NetworkService;
   }

   @Override
   protected void syncAdditionalObjects() throws IOException, NXCException
   {
   }
}
