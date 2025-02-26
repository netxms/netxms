/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.WirelessDomain;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.helpers.AccessPointFilter;
import org.netxms.nxmc.modules.objects.views.helpers.AccessPointListComparator;
import org.netxms.nxmc.modules.objects.views.helpers.AccessPointListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Access Points" view
 */
public class AccessPointsView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(AccessPointsView.class);

	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
   public static final int COLUMN_MAC_ADDRESS = 2;
   public static final int COLUMN_IP_ADDRESS = 3;
   public static final int COLUMN_CONTROLLER = 4;
   public static final int COLUMN_VENDOR = 5;
   public static final int COLUMN_MODEL = 6;
   public static final int COLUMN_SERIAL_NUMBER = 7;
   public static final int COLUMN_STATE = 8;
   public static final int COLUMN_STATUS = 9;

	private SortableTableViewer viewer;
	private Action actionExportToCsv;
   private SessionListener sessionListener;

   /**
    * Create "Services" view
    */
   public AccessPointsView()
   {
      super(LocalizationHelper.getI18n(AccessPointsView.class).tr("Access Points"), ResourceManager.getImageDescriptor("icons/object-views/access-points.png"), "objects.access-points", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 40;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof WirelessDomain);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
      final String[] names = {
            i18n.tr("ID"),
            i18n.tr("Name"),
            i18n.tr("MAC Address"),
            i18n.tr("IP Address"),
            i18n.tr("Controller"),
            i18n.tr("Vendor"),
            i18n.tr("Model"),
            i18n.tr("Serial Number"),
            i18n.tr("State"),
            i18n.tr("Status")
      };
      final int[] widths = { 60, 150, 100, 100, 150, 150, 100, 100, 80, 80 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setLabelProvider(new AccessPointListLabelProvider());
		viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new AccessPointListComparator());
		viewer.getTable().setHeaderVisible(true);
		viewer.getTable().setLinesVisible(true);

      AccessPointFilter filter = new AccessPointFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "AccessPointsTable");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveColumnSettings(viewer.getTable(), "AccessPointsTable");
         }
		});

		createActions();
		createContextMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               AbstractObject object = (AbstractObject)n.getObject();
               if ((object != null) && (object instanceof AccessPoint) && object.isDirectChildOf(getObjectId()))
               {
                  getDisplay().asyncExec(() -> refresh());
               }
            }
         }
      };
      session.addListener(sessionListener);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionExportToCsv  = new ExportToCsvAction(this, viewer, true);
	}

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new ObjectContextMenuManager(this, viewer, null) {
         @Override
         protected void fillContextMenu()
         {
            AccessPointsView.this.fillContextMenu(this);
            add(new Separator());
            super.fillContextMenu();
         }
      };

      // Create menu
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
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
	@Override
	public void refresh()
	{
		if (getObject() != null)
		{
	      List<AbstractObject> list = new ArrayList<AbstractObject>();
         for(AbstractObject o : getObject().getAllChildren(AbstractObject.OBJECT_ACCESSPOINT))
	      {
	         list.add(o);
	      }
         viewer.setInput(list.toArray());
		}
		else
		{
			viewer.setInput(new AbstractNode[0]);
		}
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
	   if ((object != null) && isActive())
	      refresh();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      refresh();
      super.activate();
   }   
}
