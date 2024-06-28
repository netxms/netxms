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
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Display and manage processes
 */
public class ProcessesTab extends ObjectTab
{
   public static final int COLUMN_PID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_USER = 2;
   public static final int COLUMN_THREADS = 3;
   public static final int COLUMN_HANDLES = 4;
   public static final int COLUMN_VMSIZE = 5;
   public static final int COLUMN_RSS = 6;
   public static final int COLUMN_PAGE_FAULTS = 7;
   public static final int COLUMN_KTIME = 8;
   public static final int COLUMN_UTIME = 9;
   public static final int COLUMN_CMDLINE = 10;

   private CompositeWithMessageBar viewerContainer;
   private FilterText filterText;
   private SortableTableViewer viewer;
   private Action actionTerminate;
   private boolean showFilter;
   private String filterString = null;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return (object instanceof Node) && ((Node)object).hasAgent();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   {
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      showFilter = getBooleanFromSettings(settings, "ProcessesTab.showFilter", true);

      viewerContainer = new CompositeWithMessageBar(parent, SWT.NONE) {
         @Override
         protected Composite createContent(Composite parent)
         {
            Composite content = super.createContent(parent);
            content.setLayout(new FormLayout());
            return content;
         }
      };

      // Create filter area
      filterText = new FilterText(viewerContainer.getContent(), SWT.NONE, null, true);
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

      final String[] names = { "PID", "Name", "User", "Threads", "Handles", "VM Size", "RSS", "Page faults", "System time", "User time", "Command line" };
      final int[] widths = { 90, 200, 140, 90, 90, 90, 90, 90, 90, 90, 500 };
      viewer = new SortableTableViewer(viewerContainer.getContent(), names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ProcessLabelProvider());
      viewer.setComparator(new ProcessComparator());
      viewer.addFilter(new ProcessFilter());
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "ProcessTable.V2"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "ProcessTable.V2"); //$NON-NLS-1$
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
            settings.put("ProcessesTab.showFilter", showFilter);
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
      actionTerminate = new Action("&Terminate") {
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
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager(getViewPart().getSite().getId() + ".ProcessesTab");
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
      manager.add(actionTerminate);
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
      if (isActive())
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
      new ConsoleJob("Get process list", getViewPart(), Activator.PLUGIN_ID, viewerContainer) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Table processTable = session.queryTable(nodeId, DataOrigin.AGENT, "System.Processes");

            int[] indexes = new int[11];
            indexes[COLUMN_PID] = processTable.getColumnIndex("PID");
            indexes[COLUMN_NAME] = processTable.getColumnIndex("NAME");
            indexes[COLUMN_USER] = processTable.getColumnIndex("USER");
            indexes[COLUMN_THREADS] = processTable.getColumnIndex("THREADS");
            indexes[COLUMN_HANDLES] = processTable.getColumnIndex("HANDLES");
            indexes[COLUMN_VMSIZE] = processTable.getColumnIndex("VMSIZE");
            indexes[COLUMN_RSS] = processTable.getColumnIndex("RSS");
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
               processes[i] = process;
            }

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getControl().isDisposed())
                     viewer.setInput(processes);
               }
            });
         }

         @Override
         protected void jobFailureHandler()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(new Object[0]);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get process list";
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

      if (!MessageDialogHelper.openQuestion(getViewPart().getSite().getShell(), "Terminate Process", "Selected processes will be terminated. Are you sure?"))
         return;

      final List<Long> processes = new ArrayList<Long>(selection.size());
      for(Object o : selection.toList())
         processes.add(((Process)o).data[COLUMN_PID]);

      final long nodeId = getObject().getObjectId();
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Terminate process", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Long pid : processes)
               session.executeAction(nodeId, "Process.Terminate", new String[] { Long.toString(pid) });
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
    * Process representation
    */
   private static class Process
   {
      long[] data = new long[11];
      String name;
      String commandLine;
      String user;
   }

   /**
    * Process label provider
    */
   private static class ProcessLabelProvider extends LabelProvider implements ITableLabelProvider
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
         switch(columnIndex)
         {
            case COLUMN_CMDLINE:
               return ((Process)element).commandLine;
            case COLUMN_NAME:
               return ((Process)element).name;
            case COLUMN_USER:
               return ((Process)element).user;
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
