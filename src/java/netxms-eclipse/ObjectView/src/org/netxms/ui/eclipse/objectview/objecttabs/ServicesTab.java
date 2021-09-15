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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Display and manage system services
 */
public class ServicesTab extends ObjectTab
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DISPLAY_NAME = 1;
   public static final int COLUMN_STATE = 2;
   public static final int COLUMN_PID = 3;
   public static final int COLUMN_TYPE = 4;
   public static final int COLUMN_STARTUP = 5;
   public static final int COLUMN_RUN_AS = 6;
   public static final int COLUMN_CMDLINE = 7;
   public static final int COLUMN_DEPENDENCIES = 8;

   private Composite viewerContainer;
   private FilterText filterText;
   private SortableTableViewer viewer;
   private Action actionStart;
   private Action actionStop;
   private boolean showFilter;
   private String filterString = null;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return (object instanceof Node) && ((Node)object).hasAgent() && ((Node)object).getPlatformName().startsWith("windows-");
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   {
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      showFilter = getBooleanFromSettings(settings, "ServicesTab.showFilter", true);

      viewerContainer = new Composite(parent, SWT.NONE);
      viewerContainer.setLayout(new FormLayout());

      // Create filter area
      filterText = new FilterText(viewerContainer, SWT.NONE, null, true);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
            Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
            State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
            state.setValue(false);
            service.refreshElements(command.getId(), null);
         }
      });

      final String[] names = { "Name", "Display name", "State", "PID", "Type", "Startup", "Run as", "Command line", "Dependencies" };
      final int[] widths = { 180, 280, 90, 80, 100, 150, 300, 500, 200 };
      viewer = new SortableTableViewer(viewerContainer, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ServiceLabelProvider());
      viewer.setComparator(new ServiceComparator());
      viewer.addFilter(new ServiceFilter());
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "ServiceTable"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "ServiceTable"); //$NON-NLS-1$
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.put("ServicesTab.showFilter", showFilter);
         }
      });

      // Set initial focus to filter input line
      if (showFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      createActions();
      createPopupMenu();
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
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager(getViewPart().getSite().getId() + ".ServicesTab");
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu.
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
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();

      // Check/uncheck menu items
      ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);

      Command command = service.getCommand("org.netxms.ui.eclipse.objectview.commands.show_filter"); //$NON-NLS-1$
      State state = command.getState("org.netxms.ui.eclipse.objectview.commands.show_filter.state"); //$NON-NLS-1$
      state.setValue(showFilter);
      service.refreshElements(command.getId(), null);

      refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void objectChanged(AbstractObject object)
   {
      if (isVisible())
         refresh();
      else
         viewer.setInput(new Object[0]);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      final long nodeId = getObject().getObjectId();
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Get system services", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
                  service.data[j] = r.getValue(indexes[j]);
               services[i] = service;
            }

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getControl().isDisposed())
                     viewer.setInput(services);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get system service information";
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

      final long nodeId = getObject().getObjectId();
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Execute service control command", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(String s : services)
               session.executeAction(nodeId, action, new String[] { s });
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot execute command on remote system";
         }
      }.start();
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      showFilter = enable;
      filterText.setVisible(showFilter);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      viewerContainer.layout();
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$
   }

   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   private void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filterString = text;
      viewer.refresh(false);
   }

   /**
    * Get boolean value from settings.
    *
    * @param settings settings
    * @param name parameter name
    * @param defaultValue default value
    * @return parameter value or default if not found
    */
   private static boolean getBooleanFromSettings(IDialogSettings settings, String name, boolean defaultValue)
   {
      if (settings.get(name) == null)
         return defaultValue;
      return settings.getBoolean(name);
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
            if (service.data[i].toLowerCase().contains(filterString))
               return true;
         }

         return false;
      }
   }
}
