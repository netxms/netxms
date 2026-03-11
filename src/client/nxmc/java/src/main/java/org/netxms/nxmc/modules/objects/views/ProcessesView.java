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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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
import org.netxms.client.constants.DataOrigin;
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
import org.netxms.nxmc.tools.ViewRefreshController;
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
   public static final int COLUMN_CPU_USAGE = 7;
   public static final int COLUMN_MEMORY_USAGE = 8;
   public static final int COLUMN_PAGE_FAULTS = 9;
   public static final int COLUMN_KTIME = 10;
   public static final int COLUMN_UTIME = 11;
   public static final int COLUMN_CMDLINE = 12;

   private static final int AUTOREFRESH_INTERVAL = 5;

   private SortableTableViewer viewer;
   private ViewRefreshController refreshController;
   private Action actionTerminate;
   private Action actionAutoRefresh;
   private boolean autoRefreshEnabled = true;
   private String filterString = null;
   private Map<Long, long[]> previousCpuTimes = new HashMap<>();
   private long previousTimestamp = 0;
   private int cpuCount = 0;
   private long currentNodeId = 0;

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
      previousCpuTimes.clear();
      previousTimestamp = 0;
      cpuCount = 0;
      currentNodeId = 0;
      if (isActive())
         refresh();
      else
         viewer.setInput(new Object[0]);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("PID"), i18n.tr("Name"), i18n.tr("User"), i18n.tr("Threads"), i18n.tr("Handles"), i18n.tr("VM Size"), i18n.tr("RSS"), i18n.tr("CPU %"),
            i18n.tr("Memory %"), i18n.tr("Page faults"), i18n.tr("System time"), i18n.tr("User time"), i18n.tr("Command line") };
      final int[] widths = { 90, 200, 140, 90, 90, 90, 90, 90, 90, 90, 90, 90, 500 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ProcessLabelProvider());
      viewer.setComparator(new ProcessComparator());
      viewer.addFilter(new ProcessFilter());
      viewer.enableColumnReordering();
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

      refreshController = new ViewRefreshController(this, autoRefreshEnabled ? AUTOREFRESH_INTERVAL : -1, new Runnable() {
         @Override
         public void run()
         {
            refresh();
         }
      });
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

      actionAutoRefresh = new Action(i18n.tr("Refresh &automatically"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            autoRefreshEnabled = actionAutoRefresh.isChecked();
            refreshController.setInterval(autoRefreshEnabled ? AUTOREFRESH_INTERVAL : -1);
         }
      };
      actionAutoRefresh.setChecked(autoRefreshEnabled);
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
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionAutoRefresh);
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
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
            if (cpuCount == 0 || currentNodeId != nodeId)
            {
               currentNodeId = nodeId;
               try
               {
                  cpuCount = Integer.parseInt(session.queryMetric(nodeId, DataOrigin.AGENT, "System.CPU.Count").trim());
               }
               catch(Exception e)
               {
                  cpuCount = 1;
               }
            }

            final Table processTable = session.queryAgentTable(nodeId, "System.Processes");
            long currentTimestamp = System.currentTimeMillis();

            int idxPid = processTable.getColumnIndex("PID");
            int idxName = processTable.getColumnIndex("NAME");
            int idxUser = processTable.getColumnIndex("USER");
            int idxThreads = processTable.getColumnIndex("THREADS");
            int idxHandles = processTable.getColumnIndex("HANDLES");
            int idxVmSize = processTable.getColumnIndex("VMSIZE");
            int idxRss = processTable.getColumnIndex("RSS");
            int idxMemUsage = processTable.getColumnIndex("MEMORY_USAGE");
            int idxPageFaults = processTable.getColumnIndex("PAGE_FAULTS");
            int idxKTime = processTable.getColumnIndex("KTIME");
            int idxUTime = processTable.getColumnIndex("UTIME");
            int idxCmdLine = processTable.getColumnIndex("CMDLINE");

            long deltaTime = currentTimestamp - previousTimestamp;
            boolean hasPreviousData = (previousTimestamp > 0) && (deltaTime > 0);

            Map<Long, long[]> currentCpuTimes = new HashMap<>();
            final Process[] processes = new Process[processTable.getRowCount()];
            for(int i = 0; i < processTable.getRowCount(); i++)
            {
               TableRow r = processTable.getRow(i);
               Process process = new Process();
               process.pid = (idxPid != -1) ? r.getValueAsLong(idxPid) : 0;
               process.name = (idxName != -1) ? r.getValue(idxName) : "";
               process.user = (idxUser != -1) ? r.getValue(idxUser) : "";
               process.threads = (idxThreads != -1) ? r.getValueAsLong(idxThreads) : 0;
               process.handles = (idxHandles != -1) ? r.getValueAsLong(idxHandles) : 0;
               process.commandLine = (idxCmdLine != -1) ? r.getValue(idxCmdLine) : "";
               process.rss = (idxRss != -1) ? r.getValue(idxRss) : "0";
               process.vmsize = (idxVmSize != -1) ? r.getValue(idxVmSize) : "0";
               process.memoryUsage = (idxMemUsage != -1) ? r.getValueAsDouble(idxMemUsage) : null;
               process.pageFaults = (idxPageFaults != -1) ? r.getValueAsLong(idxPageFaults) : 0;
               process.ktime = (idxKTime != -1) ? r.getValueAsLong(idxKTime) : 0;
               process.utime = (idxUTime != -1) ? r.getValueAsLong(idxUTime) : 0;

               long totalCpuTime = process.ktime + process.utime;
               currentCpuTimes.put(process.pid, new long[] { process.ktime, process.utime });

               if (hasPreviousData)
               {
                  long[] prev = previousCpuTimes.get(process.pid);
                  if (prev != null)
                  {
                     long deltaCpu = totalCpuTime - (prev[0] + prev[1]);
                     if (deltaCpu >= 0)
                        process.cpuUsage = (double)deltaCpu / (deltaTime * cpuCount) * 100.0;
                  }
               }

               processes[i] = process;
            }

            previousCpuTimes = currentCpuTimes;
            previousTimestamp = currentTimestamp;

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
         processes.add(((Process)o).pid);

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
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      refreshController.dispose();
      super.dispose();
   }

   /**
    * Process representation
    */
   private static class Process
   {
      long pid;
      String name;
      String user;
      long threads;
      long handles;
      String vmsize;
      String rss;
      Double cpuUsage;
      Double memoryUsage;
      long pageFaults;
      long ktime;
      long utime;
      String commandLine;
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
         Process p = (Process)element;
         switch(columnIndex)
         {
            case COLUMN_PID:
               return Long.toString(p.pid);
            case COLUMN_NAME:
               return p.name;
            case COLUMN_USER:
               return p.user;
            case COLUMN_THREADS:
               return Long.toString(p.threads);
            case COLUMN_HANDLES:
               return Long.toString(p.handles);
            case COLUMN_VMSIZE:
               return formatter.format(p.vmsize, null);
            case COLUMN_RSS:
               return formatter.format(p.rss, null);
            case COLUMN_CPU_USAGE:
               return (p.cpuUsage != null) ? String.format("%.2f", p.cpuUsage) : "";
            case COLUMN_MEMORY_USAGE:
               return (p.memoryUsage != null) ? String.format("%.2f", p.memoryUsage) : "";
            case COLUMN_PAGE_FAULTS:
               return Long.toString(p.pageFaults);
            case COLUMN_KTIME:
               return Long.toString(p.ktime);
            case COLUMN_UTIME:
               return Long.toString(p.utime);
            case COLUMN_CMDLINE:
               return p.commandLine;
            default:
               return "";
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
         Process p1 = (Process)e1;
         Process p2 = (Process)e2;
         int result;
         switch(column)
         {
            case COLUMN_PID:
               result = Long.compare(p1.pid, p2.pid);
               break;
            case COLUMN_NAME:
               result = p1.name.compareToIgnoreCase(p2.name);
               break;
            case COLUMN_USER:
               result = p1.user.compareToIgnoreCase(p2.user);
               break;
            case COLUMN_THREADS:
               result = Long.compare(p1.threads, p2.threads);
               break;
            case COLUMN_HANDLES:
               result = Long.compare(p1.handles, p2.handles);
               break;
            case COLUMN_CPU_USAGE:
               result = compareNullableDoubles(p1.cpuUsage, p2.cpuUsage);
               break;
            case COLUMN_MEMORY_USAGE:
               result = compareNullableDoubles(p1.memoryUsage, p2.memoryUsage);
               break;
            case COLUMN_PAGE_FAULTS:
               result = Long.compare(p1.pageFaults, p2.pageFaults);
               break;
            case COLUMN_KTIME:
               result = Long.compare(p1.ktime, p2.ktime);
               break;
            case COLUMN_UTIME:
               result = Long.compare(p1.utime, p2.utime);
               break;
            case COLUMN_CMDLINE:
               result = p1.commandLine.compareToIgnoreCase(p2.commandLine);
               break;
            default:
               result = 0;
               break;
         }
         return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
      }

      private static int compareNullableDoubles(Double d1, Double d2)
      {
         if (d1 == null)
            return (d2 == null) ? 0 : -1;
         if (d2 == null)
            return 1;
         return Double.compare(d1, d2);
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
