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
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.helpers.NodeListComparator;
import org.netxms.nxmc.modules.objects.views.helpers.NodeListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Nodes" view
 */
public class NodesView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(NodesView.class);
   
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_IP_ADDRESS = 2;
   public static final int COLUMN_RACK = 3;
   public static final int COLUMN_PLATFORM = 4;
   public static final int COLUMN_AGENT_VERSION = 5;
   public static final int COLUMN_SYS_DESCRIPTION = 6;
   public static final int COLUMN_STATUS = 7;
	
	private SortableTableViewer viewer;
	private Action actionExportToCsv;
   private SessionListener sessionListener;

   /**
    * Create "Services" view
    */
   public NodesView()
   {
      super(LocalizationHelper.getI18n(NodesView.class).tr("Nodes"), ResourceManager.getImageDescriptor("icons/object-views/nodes.png"), "objects.nodes", false);
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
      return 100;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof Subnet) || (context instanceof Cluster) || (context instanceof Collector) || (context instanceof Container) || (context instanceof ServiceRoot) ||
            (context instanceof Rack);
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
            i18n.tr("Primary IP"),
            i18n.tr("Rack"),
            i18n.tr("Platform"),
            i18n.tr("Agent Version"),
            i18n.tr("Sys Description"),
            i18n.tr("Status")
      };
		final int[] widths = { 60, 150, 100, 150, 150, 100, 300, 100 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setLabelProvider(new NodeListLabelProvider());
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new NodeListComparator());
		viewer.getTable().setHeaderVisible(true);
		viewer.getTable().setLinesVisible(true);
		WidgetHelper.restoreTableViewerSettings(viewer, "NodeTable.V2");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveColumnSettings(viewer.getTable(), "NodeTable.V2");
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
               if ((object != null) && (getObject() != null) && object.isChildOf(getObject().getObjectId()))
               {
                  viewer.getTable().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        refresh();
                     }
                  });
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
            NodesView.this.fillContextMenu(this);
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
	      for(AbstractObject o : getObject().getAllChildren(AbstractObject.OBJECT_NODE))
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
