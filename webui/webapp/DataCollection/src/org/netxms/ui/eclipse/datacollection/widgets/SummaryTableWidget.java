/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeColumn;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableCell;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.views.helpers.ObjectSelectionProvider;
import org.netxms.ui.eclipse.datacollection.widgets.internal.SummaryTableContentProvider;
import org.netxms.ui.eclipse.datacollection.widgets.internal.SummaryTableItemComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.TableLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objects.ObjectWrapper;
import org.netxms.ui.eclipse.objectview.views.TabbedObjectView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * DCI summary table display widget
 */
public class SummaryTableWidget extends Composite
{
   private int tableId;
   private long baseObjectId;
   private IViewPart viewPart;
   private SortableTreeViewer viewer;
   private TableLabelProvider labelProvider;
   private Action actionExportToCsv;
   private Action actionUseMultipliers;
   private Action actionForcePollAll;
   private Action actionShowObjectDetails;
   private ViewRefreshController refreshController;
   private boolean useMultipliers = true;
   private TreeColumn currentColumn = null;
   private ObjectSelectionProvider objectSelectionProvider;
   private int showLineCount;
   private List<String> sortingColumnList = null;

   /**
    * Create summary table widget
    * 
    * @param parent parent composite
    * @param style widget style
    * @param viewPart view part this widget is associated with
    * @param tableId summary table ID
    * @param baseObjectId base object ID
    */
   public SummaryTableWidget(Composite parent, int style, IViewPart viewPart, int tableId, long baseObjectId)
   {
      super(parent, style);
      
      this.viewPart = viewPart;
      this.tableId = tableId;
      this.baseObjectId = baseObjectId;
      
      setLayout(new FillLayout());

      viewer = new SortableTreeViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new SummaryTableContentProvider());
      labelProvider = new TableLabelProvider();
      labelProvider.setUseMultipliers(useMultipliers);
      viewer.setLabelProvider(labelProvider);
      
      objectSelectionProvider = new ObjectSelectionProvider(viewer);
      
      createActions();
      createPopupMenu();
      
      refreshController = new ViewRefreshController(viewPart, -1, new Runnable() {
         @Override
         public void run()
         {
            if (isDisposed())
               return;
            refresh();
         }
      });
      
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
      
      viewer.getTree().addMouseListener(new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
         }
         
         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 3)
               currentColumn = viewer.getColumnAtPoint(new Point(e.x, e.y));
         }
         
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
         }
      });
   }

   /**
    * Create actions 
    */
   private void createActions()
   {
      actionExportToCsv = new ExportToCsvAction(viewPart, viewer, true);
      
      actionUseMultipliers = new Action(Messages.get().LastValues_UseMultipliers, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            setUseMultipliers(!useMultipliers);
         }
      };
      actionUseMultipliers.setChecked(useMultipliers);
      
      actionForcePollAll = new Action(Messages.get().SummaryTableWidget_ForcePollForAllColumns) {
         @Override
         public void run()
         {
            forcePoll(true);
         }
      };
      
      actionShowObjectDetails = new Action(Messages.get().SummaryTableWidget_ShowObjectDetails) {
         @Override
         public void run()
         {
            showObjectDetails();
         }
      };
      actionShowObjectDetails.setId("org.netxms.ui.eclipse.datacollection.popupActions.ShowObjectDetails"); //$NON-NLS-1$
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager for underlying node object
      final MenuManager nodeMenuManager = new MenuManager() {
         @Override
         public String getMenuText()
         {
            return Messages.get().SummaryTableWidget_Node;
         }
         
      };
      nodeMenuManager.setRemoveAllWhenShown(true);
      nodeMenuManager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            ObjectContextMenu.fill(mgr, viewPart.getSite(), objectSelectionProvider);
         }
      });
      
      // Create menu manager for rows
      MenuManager rowMenuManager = new MenuManager();
      rowMenuManager.setRemoveAllWhenShown(true);
      rowMenuManager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr, nodeMenuManager);
         }
      });

      // Create menu.
      Menu menu = rowMenuManager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
      
      // Register menu for extension.
      if (viewPart != null)
      {
         viewPart.getSite().registerContextMenu(viewPart.getSite().getId() + ".data", rowMenuManager, viewer); //$NON-NLS-1$
         viewPart.getSite().registerContextMenu(viewPart.getSite().getId() + ".node", nodeMenuManager, objectSelectionProvider); //$NON-NLS-1$
      }
   }

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager, MenuManager nodeMenuManager)
   {
      manager.add(actionUseMultipliers);
      manager.add(new Separator());
      manager.add(nodeMenuManager);
      manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_TOOLS));
      manager.add(new Separator());
      manager.add(actionShowObjectDetails);
      manager.add(new Separator());
      if ((currentColumn != null) && ((Integer)currentColumn.getData("ID") > 0)) //$NON-NLS-1$
      {
         manager.add(new Action(String.format(Messages.get().SummaryTableWidget_ForcePollForNode, currentColumn.getText())) {
            @Override
            public void run()
            {
               forcePoll(false);
            }
         });
      }
      manager.add(actionForcePollAll);
      manager.add(actionExportToCsv);
   }

   /**
    * Refresh table
    */
   public void refresh()
   {
      viewer.setInput(null);
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().SummaryTable_JobName, viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Table table = session.queryDciSummaryTable(tableId, baseObjectId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  update(table);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().SummaryTable_JobError;
         }
      }.start();
   }
   
   /**
    * Update viewer with fresh table data
    * 
    * @param table table
    */
   public void update(final Table table)
   {
      if (!viewer.isInitialized())
      {
         final String[] names = table.getColumnDisplayNames();
         final int[] widths = new int[names.length];
         Arrays.fill(widths, 100);
         viewer.createColumns(names, widths, 0, SWT.UP);
         final IDialogSettings settings = Activator.getDefault().getDialogSettings();
         final String key = viewPart.getViewSite().getId() + ".SummaryTable." + Integer.toString(tableId); //$NON-NLS-1$
         WidgetHelper.restoreTreeViewerSettings(viewer, settings, key);
         String value = settings.get(key + ".useMultipliers"); //$NON-NLS-1$
         if (value != null)
            useMultipliers = Boolean.parseBoolean(value);
         labelProvider.setUseMultipliers(useMultipliers);
         viewer.getControl().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               WidgetHelper.saveTreeViewerSettings(viewer, settings, key);
               settings.put(key + ".useMultipliers", useMultipliers); //$NON-NLS-1$
            }
         });
         viewer.setComparator(new SummaryTableItemComparator(table));
      }
      labelProvider.setColumnDataTypes(table.getColumnDataTypes());
      if(sortingColumnList != null && sortingColumnList.size() > 0)
      {
         List<SortItem> sortItem = new ArrayList<SortItem>();
         for(int i = 0; i < sortingColumnList.size() ; i++)
         {            
            boolean isDesc = sortingColumnList.get(i).charAt(0) == '>' ? true : false;
            int index = table.getColumnIndex(sortingColumnList.get(i).substring(1));
            if(index >= 0)
            {
               sortItem.add(new SortItem(index, isDesc));
            }
         }
         
         //find index of columns to compare and desc or asc
         final List<SortItem> sortItemFin = sortItem;
         table.sort(new Comparator<TableRow>() {
            public int compare(TableRow row1, TableRow row2)
            {
               //compare lines
               int result = 0;
               int i = 0;
               while(result == 0 && i < sortItemFin.size())
               {
                  result = compareItem(row1, row2, sortItemFin.get(i).colIndex, sortItemFin.get(i).isDesc);
                  i++;
               }
               return result;
            }
            
            private int compareItem(TableRow row1, TableRow row2, int index, boolean sortDesc)
            {
               TableCell c1 = row1.get(index);
               TableCell c2 = row2.get(index);

               String s1 = (c1 != null) ? c1.getValue() : "";
               String s2 = (c2 != null) ? c2.getValue() : "";
               
               int result = 0;
               try
               {
                  double value1 = Double.parseDouble(s1);
                  double value2 = Double.parseDouble(s2);
                  result = Double.compare(value1, value2);
               }
               catch(NumberFormatException e)
               {
                  result = s1.compareToIgnoreCase(s2);
               }
               return sortDesc ? -result : result;
            }
         });
         viewer.setComparator(null);
      }
      if(showLineCount > 0)
         viewer.setInput(table.getFirstRows(showLineCount));
      else
         viewer.setInput(table);
      viewer.expandAll();
   }
   
   /**
    * Class for advanced column sorting
    */
   class SortItem
   {
      public int colIndex;
      public boolean isDesc;
      
      public SortItem(int colIndex, boolean isDesc)
      {
         this.colIndex = colIndex;
         this.isDesc = isDesc;
      }
   };

   /**
    * @return the viewer
    */
   public SortableTreeViewer getViewer()
   {
      return viewer;
   }
   
   /**
    * @return
    */
   public ISelectionProvider getObjectSelectionProvider()
   {
      return objectSelectionProvider;
   }
   
   /**
    * Set automatic refresh interval. If less or equal 0, automatic refresh will be disabled.
    * 
    * @param interval auto refresh interval in seconds
    */
   public void setAutoRefresh(int interval)
   {
      refreshController.setInterval(interval);
   }

   /**
    * @return
    */
   public boolean areMultipliersUsed()
   {
      return useMultipliers;
   }

   /**
    * @param value
    */
   public void setUseMultipliers(boolean value)
   {
      useMultipliers = value;
      actionUseMultipliers.setChecked(value);
      if (viewer.isInitialized())
      {
         labelProvider.setUseMultipliers(value);
         viewer.refresh();
      }
   }

   /**
    * @return the actionUseMultipliers
    */
   public Action getActionUseMultipliers()
   {
      return actionUseMultipliers;
   }
   
   /**
    * Show details for selected object
    */
   private void showObjectDetails()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      AbstractObject object = ((ObjectWrapper)selection.getFirstElement()).getObject();
      if (object != null)
      {
         try
         {
            TabbedObjectView view = (TabbedObjectView)PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().showView(TabbedObjectView.ID);
            view.setObject(object);
         }
         catch(PartInitException e)
         {
            MessageDialogHelper.openError(getShell(), Messages.get().SummaryTableWidget_Error, String.format(Messages.get().SummaryTableWidget_CannotOpenObjectDetails, e.getLocalizedMessage()));
         }
      }
   }
   
   /**
    * @param pollAll
    */
   private void forcePoll(boolean pollAll)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      final List<PollRequest> requests = new ArrayList<PollRequest>();
      for(Object o : selection.toList())
      {
         TableRow r = (TableRow)o;
         long nodeId = r.getObjectId();
         if (pollAll)
         {
            int count = ((Table)viewer.getInput()).getColumnCount();
            for(int i = 1; i < count; i++)
            {
               long dciId = r.get(i).getObjectId();
               if (dciId != 0)
               {
                  requests.add(new PollRequest(nodeId, dciId));
               }
            }
         }
         else
         {
            int index = ((Table)viewer.getInput()).getColumnIndex(currentColumn.getText());
            long dciId = r.get(index).getObjectId();
            if (dciId != 0)
            {
               requests.add(new PollRequest(nodeId, dciId));
            }
         }
      }
      
      if (requests.isEmpty())
         return;
      
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().SummaryTableWidget_ForceDciPoll, viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(Messages.get().SummaryTableWidget_DciPoll, requests.size());
            for(PollRequest r : requests)
            {
               session.forceDCIPoll(r.nodeId, r.dciId);
               monitor.worked(1);
            }
            monitor.done();
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
            return Messages.get().SummaryTableWidget_13;
         }
      }.start();
   }
   
   /**
    * Forced poll request
    */
   private class PollRequest
   {
      public long nodeId;
      public long dciId;
      
      public PollRequest(long nodeId, long dciId)
      {
         this.nodeId = nodeId;
         this.dciId = dciId;
      }
   }
   
   /**
    * Set number of lines to show
    * 
    * @param lineCount line count to be shown
    */
   public void setShowNumLine(int lineCount)
   {
      this.showLineCount = lineCount;
   }

   /**
    * Set columns that will be used to sort table
    * 
    * @param sortingColumnList columns to be used while sorting
    */
   public void setSortColumns(List<String> sortingColumnList)
   {
      this.sortingColumnList = sortingColumnList;      
   }
}
