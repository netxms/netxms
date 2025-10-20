/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Display and manage system services
 */
public class ServicesView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ServicesView.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DISPLAY_NAME = 1;
   public static final int COLUMN_STATE = 2;
   public static final int COLUMN_PID = 3;
   public static final int COLUMN_TYPE = 4;
   public static final int COLUMN_STARTUP = 5;
   public static final int COLUMN_RUN_AS = 6;
   public static final int COLUMN_CMDLINE = 7;
   public static final int COLUMN_DEPENDENCIES = 8;

   private SortableTableViewer viewer;
   private Action actionStart;
   private Action actionStop;
   private Action actionSetAutoStart;
   private Action actionSetManualStart;
   private Action actionDisable;
   private String filterString = null;

   /**
    * Create "Services" view
    */
   public ServicesView()
   {
      super(LocalizationHelper.getI18n(ServicesView.class).tr("Services"), ResourceManager.getImageDescriptor("icons/object-views/services.png"), "objects.services", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && ((Node)context).hasAgent() && ((Node)context).hasServiceManager();
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
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (isActive())
         refresh();
      else
         viewer.setInput(new Object[0]);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] names = { "Name", "Display name", "State", "PID", "Type", "Startup", "Run as", "Command line", "Dependencies" };
      final int[] widths = { 180, 280, 90, 80, 100, 150, 300, 500, 200 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ServiceLabelProvider());
      viewer.setComparator(new ServiceComparator());
      viewer.addFilter(new ServiceFilter());
      WidgetHelper.restoreTableViewerSettings(viewer, "ServiceTable");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), "ServiceTable");
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
      actionStart = new Action("&Start") {
         @Override
         public void run()
         {
            executeAgentAction("Service.Start");
         }
      };

      actionStop = new Action("S&top") {
         @Override
         public void run()
         {
            executeAgentAction("Service.Stop");
         }
      };

      actionSetAutoStart = new Action("&Automatic") {
         @Override
         public void run()
         {
            executeAgentAction("Service.SetStartType.Automatic");
         }
      };

      actionSetManualStart = new Action("&Manual") {
         @Override
         public void run()
         {
            executeAgentAction("Service.SetStartType.Manual");
         }
      };

      actionDisable = new Action("&Disabled") {
         @Override
         public void run()
         {
            executeAgentAction("Service.SetStartType.Disabled");
         }
      };
   }

   /**
    * Create pop-up menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionStart);
      manager.add(actionStop);

      if (((Node)getObject()).getPlatformName().startsWith("windows-"))
      {
         MenuManager startTypeMenu = new MenuManager("&Change start type");
         startTypeMenu.add(actionSetAutoStart);
         startTypeMenu.add(actionSetManualStart);
         startTypeMenu.add(actionDisable);
         manager.add(new Separator());
         manager.add(startTypeMenu);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long nodeId = getObjectId();
      new Job(i18n.tr("Reading service list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Table serviceTable = session.queryAgentTable(nodeId, "System.Services");

            int[] indexes = new int[9];
            indexes[COLUMN_NAME] = serviceTable.getColumnIndex("NAME");
            indexes[COLUMN_DISPLAY_NAME] = serviceTable.getColumnIndex("DISPNAME");
            indexes[COLUMN_STATE] = serviceTable.getColumnIndex("STATE");
            indexes[COLUMN_PID] = serviceTable.getColumnIndex("PID");
            indexes[COLUMN_TYPE] = serviceTable.getColumnIndex("TYPE");
            indexes[COLUMN_STARTUP] = serviceTable.getColumnIndex("STARTUP");
            indexes[COLUMN_RUN_AS] = serviceTable.getColumnIndex("RUN_AS");
            indexes[COLUMN_CMDLINE] = serviceTable.getColumnIndex("BINARY");
            indexes[COLUMN_DEPENDENCIES] = serviceTable.getColumnIndex("DEPENDENCIES");

            final Service[] services = new Service[serviceTable.getRowCount()];
            for(int i = 0; i < serviceTable.getRowCount(); i++)
            {
               TableRow r = serviceTable.getRow(i);
               Service service = new Service();
               for(int j = 0; j < indexes.length; j++)
               {
                  service.data[j] = r.getValue(indexes[j]);
                  if (service.data[j] == null)
                     service.data[j] = "";
               }
               services[i] = service;
            }

            runInUIThread(() -> {
               if (!viewer.getControl().isDisposed())
                  viewer.setInput(services);
               clearMessages();
            });
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(() -> viewer.setInput(new Object[0]));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get service list");
         }
      }.start();
   }

   /**
    * Execute given action on agent for selected service(s)
    *
    * @param action action to execute
    */
   private void executeAgentAction(final String action)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final List<String> services = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
         services.add(((Service)o).data[COLUMN_NAME]);

      final long nodeId = getObjectId();
      new Job(i18n.tr("Executing service control command"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String s : services)
               session.executeAction(nodeId, action, new String[] { s });
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot execute command on remote system");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#onFilterModify()
    */
   @Override
   protected void onFilterModify()
   {
      filterString = getFilterText();
      viewer.refresh(false);
   }

   /**
    * Service representation
    */
   private static class Service
   {
      String[] data = new String[9];
      Long pid = null;

      long getPid()
      {
         if (pid != null)
            return pid;

         try
         {
            pid = Long.parseLong(data[COLUMN_PID]);
         }
         catch(NumberFormatException e)
         {
            pid = 0L;
         }
         return pid;
      }
   }

   /**
    * Service label provider
    */
   private static class ServiceLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         return ((Service)element).data[columnIndex];
      }
   }

   /**
    * Comparator for service list
    */
   private static class ServiceComparator extends ViewerComparator
   {
      /**
       * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
       */
      @Override
      public int compare(Viewer viewer, Object e1, Object e2)
      {
         final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
         int result;
         if (column == COLUMN_PID)
         {
            result = Long.signum(((Service)e1).getPid() - ((Service)e2).getPid());
         }
         else
         {
            result = ((Service)e1).data[column].compareToIgnoreCase(((Service)e2).data[column]);
         }
         return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
      }
   }

   /**
    * Filter for service list
    */
   private class ServiceFilter extends ViewerFilter
   {
      @Override
      public boolean select(Viewer viewer, Object parentElement, Object element)
      {
         if ((filterString == null) || (filterString.isEmpty()))
            return true;

         Service service = (Service)element;
         for(int i = 0; i < service.data.length; i++)
         {
            if ((service.data[i] != null) && service.data[i].toLowerCase().contains(filterString))
               return true;
         }

         return false;
      }
   }
}
