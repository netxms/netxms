/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
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
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.MeasurementUnit;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Display and manage processes
 */
public class ProcessesView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ProcessesView.class);

   public static final int COLUMN_PID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_USER = 2;
   public static final int COLUMN_THREADS = 3;
   public static final int COLUMN_HANDLES = 4;
   public static final int COLUMN_VMSIZE = 5;
   public static final int COLUMN_RSS = 6;
   public static final int COLUMN_MEMORY_USAGE = 7;
   public static final int COLUMN_PAGE_FAULTS = 8;
   public static final int COLUMN_KTIME = 9;
   public static final int COLUMN_UTIME = 10;
   public static final int COLUMN_CMDLINE = 11;

   private SortableTableViewer viewer;
   private Action actionTerminate;
   private String filterString = null;

   /**
    * Create "Processes" view
    */
   public ProcessesView()
   {
      super(LocalizationHelper.getI18n(ProcessesView.class).tr("Processes"), ResourceManager.getImageDescriptor("icons/object-views/processes.png"), "objects.processes", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && ((Node)context).hasAgent();
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
      final String[] names = { i18n.tr("PID"), i18n.tr("Name"), i18n.tr("User"), i18n.tr("Threads"), i18n.tr("Handles"), i18n.tr("VM Size"), i18n.tr("RSS"), i18n.tr("Memory %"),
            i18n.tr("Page faults"), i18n.tr("System time"), i18n.tr("User time"), i18n.tr("Command line") };
      final int[] widths = { 90, 200, 140, 90, 90, 90, 90, 90, 90, 90, 90, 500 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ProcessLabelProvider());
      viewer.setComparator(new ProcessComparator());
      viewer.addFilter(new ProcessFilter());
      WidgetHelper.restoreTableViewerSettings(viewer, "ProcessTable");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), "ProcessTable");
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
      actionTerminate = new Action("&Terminate", SharedIcons.TERMINATE) {
         @Override
         public void run()
         {
            terminateProcess();
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
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

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
      manager.add(actionTerminate);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long nodeId = getObjectId();
      new Job(i18n.tr("Reading process list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Table processTable = session.queryAgentTable(nodeId, "System.Processes");

            int[] indexes = new int[12];
            indexes[COLUMN_PID] = processTable.getColumnIndex("PID");
            indexes[COLUMN_NAME] = processTable.getColumnIndex("NAME");
            indexes[COLUMN_USER] = processTable.getColumnIndex("USER");
            indexes[COLUMN_THREADS] = processTable.getColumnIndex("THREADS");
            indexes[COLUMN_HANDLES] = processTable.getColumnIndex("HANDLES");
            indexes[COLUMN_VMSIZE] = processTable.getColumnIndex("VMSIZE");
            indexes[COLUMN_RSS] = processTable.getColumnIndex("RSS");
            indexes[COLUMN_MEMORY_USAGE] = processTable.getColumnIndex("MEMORY_USAGE");
            indexes[COLUMN_PAGE_FAULTS] = processTable.getColumnIndex("PAGE_FAULTS");
            indexes[COLUMN_KTIME] = processTable.getColumnIndex("KTIME");
            indexes[COLUMN_UTIME] = processTable.getColumnIndex("UTIME");
            indexes[COLUMN_CMDLINE] = processTable.getColumnIndex("CMDLINE");

            final Process[] processes = new Process[processTable.getRowCount()];
            for(int i = 0; i < processTable.getRowCount(); i++)
            {
               TableRow r = processTable.getRow(i);
               Process process = new Process();
               for(int j = 0; j < indexes.length; j++)
                  process.data[j] = r.getValueAsLong(indexes[j]);
               process.name = (indexes[COLUMN_NAME] != -1) ? r.getValue(indexes[COLUMN_NAME]) : "";
               process.user = (indexes[COLUMN_USER] != -1) ? r.getValue(indexes[COLUMN_USER]) : "";
               process.commandLine = (indexes[COLUMN_CMDLINE] != -1) ? r.getValue(indexes[COLUMN_CMDLINE]) : "";
               process.rss = (indexes[COLUMN_RSS] != -1) ? r.getValue(indexes[COLUMN_RSS]) : "0";
               process.vmsize = (indexes[COLUMN_VMSIZE] != -1) ? r.getValue(indexes[COLUMN_VMSIZE]) : "0";
               process.memoryUsage = (indexes[COLUMN_MEMORY_USAGE] != -1) ? r.getValueAsDouble(indexes[COLUMN_MEMORY_USAGE]) : null;
               processes[i] = process;
            }

            runInUIThread(() -> {
               if (!viewer.getControl().isDisposed())
                  viewer.setInput(processes);
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
            return i18n.tr("Cannot get process list");
         }
      }.start();
   }

   /**
    * Terminate selected process(es)
    */
   private void terminateProcess()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Terminate Process"), i18n.tr("Selected processes will be terminated. Are you sure?")))
         return;

      final List<Long> processes = new ArrayList<Long>(selection.size());
      for(Object o : selection.toList())
         processes.add(((Process)o).data[COLUMN_PID]);

      final long nodeId = getObjectId();
      new Job(i18n.tr("Terminating process"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Long pid : processes)
               session.executeAction(nodeId, "Process.Terminate", new String[] { Long.toString(pid) });
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
    * Process representation
    */
   private static class Process
   {
      long[] data = new long[12];
      String name;
      String commandLine;
      String user;
      String rss;
      String vmsize;
      Double memoryUsage;
   }

   /**
    * Process label provider
    */
   private static class ProcessLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private DataFormatter formatter = new DataFormatter().setDataType(DataType.UINT64).setMeasurementUnit(MeasurementUnit.BYTES_IEC).setFormatString("%*sB");

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
         switch(columnIndex)
         {
            case COLUMN_CMDLINE:
               return ((Process)element).commandLine;
            case COLUMN_MEMORY_USAGE:
               return (((Process)element).memoryUsage != null) ? String.format("%,.2f", ((Process)element).memoryUsage) : "";
            case COLUMN_NAME:
               return ((Process)element).name;
            case COLUMN_RSS:
               return formatter.format(((Process)element).rss, null);
            case COLUMN_USER:
               return ((Process)element).user;
            case COLUMN_VMSIZE:
               return formatter.format(((Process)element).vmsize, null);
            default:
               return Long.toString(((Process)element).data[columnIndex]);
         }
      }
   }

   /**
    * Comparator for service list
    */
   private static class ProcessComparator extends ViewerComparator
   {
      /**
       * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
       */
      @Override
      public int compare(Viewer viewer, Object e1, Object e2)
      {
         final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
         int result;
         switch(column)
         {
            case COLUMN_CMDLINE:
               result = ((Process)e1).commandLine.compareToIgnoreCase(((Process)e2).commandLine);
               break;
            case COLUMN_MEMORY_USAGE:
               if (((Process)e1).memoryUsage == null)
                  result = (((Process)e2).memoryUsage == null) ? 0 : -1;
               else if (((Process)e2).memoryUsage == null)
                  result = 1;
               else
                  result = Double.compare(((Process)e1).memoryUsage, ((Process)e2).memoryUsage);
               break;
            case COLUMN_NAME:
               result = ((Process)e1).name.compareToIgnoreCase(((Process)e2).name);
               break;
            case COLUMN_USER:
               result = ((Process)e1).user.compareToIgnoreCase(((Process)e2).user);
               break;
            default:
               result = Long.signum(((Process)e1).data[column] - ((Process)e2).data[column]);
               break;
         }
         return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
      }
   }

   /**
    * Filter for service list
    */
   private class ProcessFilter extends ViewerFilter
   {
      @Override
      public boolean select(Viewer viewer, Object parentElement, Object element)
      {
         if ((filterString == null) || (filterString.isEmpty()))
            return true;

         Process process = (Process)element;
         return process.name.toLowerCase().contains(filterString) || process.commandLine.toLowerCase().contains(filterString) || process.user.toLowerCase().contains(filterString);
      }
   }
}
