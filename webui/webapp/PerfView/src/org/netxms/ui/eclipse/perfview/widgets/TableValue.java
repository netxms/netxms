/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.widgets;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.views.DataComparisonView;
import org.netxms.ui.eclipse.perfview.views.HistoricalDataView;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;
import org.netxms.ui.eclipse.perfview.widgets.helpers.CellSelectionManager;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableContentProvider;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableItemComparator;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableLabelProvider;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableValueFilter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Widget to show table DCI last value
 */
public class TableValue extends Composite
{
   private static long uniqueId = 1;

   private NXCSession session;
   private IViewPart viewPart;
   private long objectId = 0;
   private long dciId = 0;
   private String configId;
   private String objectName = null;
   private Table currentData = null;
   private SortableTableViewer viewer;
   private TableLabelProvider labelProvider;
   private CLabel errorLabel;
   private CellSelectionManager cellSelectionManager;
   private Action actionShowHistory;
   private Action actionShowLineChart;
   private Action actionShowBarChart;
   private Action actionShowPieChart;
   private Action actionUseMultipliers;
   private Action actionShowFilter;
   private FilterText filterText;
   private TableValueFilter filter;

   /**
    * @param parent
    * @param style
    * @param viewPart
    */
   public TableValue(Composite parent, int style, IViewPart viewPart, String configSubId)
   {
      super(parent, style);

      this.viewPart = viewPart;
      session = (NXCSession)ConsoleSharedData.getSession();

      setLayout(new FormLayout());
      
      // Create filter area
      filterText = new FilterText(this, SWT.NONE);
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
         }
      });

      viewer = new SortableTableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.getTable().setData(RWT.MARKUP_ENABLED, Boolean.TRUE);
      viewer.getTable().setData(RWT.CUSTOM_VARIANT, "cellselect");
      viewer.setContentProvider(new TableContentProvider());
      labelProvider = new TableLabelProvider();
      viewer.setLabelProvider(labelProvider);
      filter = new TableValueFilter();
      viewer.addFilter(filter);
      cellSelectionManager = new CellSelectionManager(viewer);
      
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
      
      StringBuilder sb = new StringBuilder("TableLastValues."); //$NON-NLS-1$
      sb.append(dciId);
      if (configSubId != null)
      {
         sb.append('.');
         sb.append(configSubId);
      }
      configId = sb.toString();
      
      final IDialogSettings ds = Activator.getDefault().getDialogSettings();
      labelProvider.setUseMultipliers(getBoolean(ds, configId + ".useMultipliers", false));
      
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, ds, configId);
            ds.put(configId + ".initShowfilter", actionShowFilter.isChecked());
         }
      });
      
      createActions();
      createPopupMenu();
      
      // Set initial focus to filter input line
      if (getBoolean(ds, configId + ".initShowfilter", false))
      {
         filterText.setFocus();
         actionShowFilter.setChecked(true);
      }
      else
         enableFilter(false); // Will hide filter area correctly
   }
   
   /**
    * @param ds IDialogSettings
    * @param s Settings string
    * @param defval default value
    * @return
    */
   private boolean getBoolean(IDialogSettings ds, String s, boolean defval)
   {
      return ds.getBoolean(s) ? true : defval;
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionShowHistory = new Action("History", Activator.getImageDescriptor("icons/data_history.gif")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showHistory();
         }
      };
      
      actionShowLineChart = new Action(Messages.get().TableValue_LineChart, Activator.getImageDescriptor("icons/chart_line.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showLineChart();
         }
      };

      actionShowBarChart = new Action(Messages.get().TableValue_BarChart, Activator.getImageDescriptor("icons/chart_bar.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showDataComparisonChart(DataComparisonChart.BAR_CHART);
         }
      };

      actionShowPieChart = new Action(Messages.get().TableValue_PieChart, Activator.getImageDescriptor("icons/chart_pie.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            showDataComparisonChart(DataComparisonChart.PIE_CHART);
         }
      };
      
      actionUseMultipliers = new Action("Use &multipliers", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setUseMultipliers(actionUseMultipliers.isChecked());
            viewer.refresh(true);
         }
      };
      actionUseMultipliers.setChecked(labelProvider.areMultipliersUsed());
      
      actionShowFilter = new Action() {
         /*
          * (non-Javadoc)
          * 
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setText("Show filter");
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.perfview.commands.show_table_values_filter"); //$NON-NLS-1$
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension
      if (viewPart != null)
      {
         viewPart.getSite().registerContextMenu(menuMgr, viewer);
      }
   }

   /**
    * Fill context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionShowHistory);
      manager.add(actionShowLineChart);
      manager.add(actionShowBarChart);
      manager.add(actionShowPieChart);
      manager.add(new Separator());
      manager.add(actionUseMultipliers);
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
   }

   /**
    * @param objectId
    * @param dciId
    */
   public void setObject(long objectId, long dciId)
   {
      this.objectId = objectId;
      this.dciId = dciId;
      objectName = session.getObjectName(objectId);
   }

   /**
    * Refresh table
    */
   public void refresh(final Runnable postRefreshHook)
   {
      viewer.setInput(null);
      ConsoleJob job = new ConsoleJob(String.format(Messages.get().TableValue_JobName, dciId), viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Table table = session.getTableLastValues(objectId, dciId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  
                  if (errorLabel != null)
                  {
                     errorLabel.dispose();
                     errorLabel = null;
                     viewer.getControl().setVisible(true);
                     viewer.getControl().getParent().layout(true, true);
                  }
                  updateViewer(table);
                  if (postRefreshHook != null)
                  {
                     postRefreshHook.run();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().TableValue_JobError, dciId);
         }

         @Override
         protected IStatus createFailureStatus(final Exception e)
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  
                  if (errorLabel == null)
                  {
                     errorLabel = new CLabel(viewer.getControl().getParent(), SWT.NONE);
                     errorLabel.setFont(JFaceResources.getBannerFont());
                     errorLabel.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
                     errorLabel.moveAbove(viewer.getControl());
                     FormData fd = new FormData();
                     fd.top = new FormAttachment(0, 0);
                     fd.left = new FormAttachment(0, 0);
                     fd.right = new FormAttachment(100, 0);
                     fd.bottom = new FormAttachment(100, 0);
                     errorLabel.setLayoutData(fd);
                     viewer.getControl().getParent().layout(true, true);
                     viewer.getControl().setVisible(false);
                  }
                  errorLabel.setText(getErrorMessage() + " (" + e.getLocalizedMessage() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
               }
            });
            return Status.OK_STATUS;
         }
      };
      job.setUser(false);
      job.start();
   }
   
   /**
    * Update viewer with fresh table data
    * 
    * @param table new table DCI data
    */
   private void updateViewer(final Table table)
   {
      final IDialogSettings ds = Activator.getDefault().getDialogSettings();
      
      if (!viewer.isInitialized())
      {
         final String[] names = table.getColumnDisplayNames();
         final int[] widths = new int[names.length];
         Arrays.fill(widths, 150);
         viewer.createColumns(names, widths, 0, SWT.UP);
         
         WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configId); //$NON-NLS-1$
         viewer.getTable().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configId); //$NON-NLS-1$
               ds.put(configId + ".useMultipliers", labelProvider.areMultipliersUsed()); //$NON-NLS-1$
            }
         });
         viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));        
      }
      
      labelProvider.setColumns(table.getColumns());
      viewer.setInput(table);
      
      // FIXME: workaround for Eclipse RAP bug #324721 (Items should inherit custom variants) 
      for(TableItem i : viewer.getTable().getItems())
         i.setData(RWT.CUSTOM_VARIANT, "cellselect");
      
      currentData = table;
   }

   /**
    * @param viewerRow
    * @return
    */
   private String buildInstanceString(ViewerRow viewerRow)
   {
      StringBuilder instance = new StringBuilder();
      boolean first = true;
      for(int i = 0; i < currentData.getColumnCount(); i++)
      {
         TableColumnDefinition cd = currentData.getColumnDefinition(i);
         if (cd.isInstanceColumn())
         {
            if (!first)
               instance.append("~~~"); //$NON-NLS-1$
            instance.append(viewerRow.getText(i));
            first = false;
         }
      }
      return instance.toString();
   }
   
   /**
    * Show history
    */
   private void showHistory()
   {
      if (currentData == null)
         return;

      ViewerCell[] cells = cellSelectionManager.getSelectedCells();
      if (cells.length == 0)
         return;
      
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         final String instance = buildInstanceString(cells[i].getViewerRow());

         String id = Long.toString(objectId) + "&" + Long.toString(dciId) + "@" //$NON-NLS-1$ //$NON-NLS-2$
               + safeEncode(column.getDisplayName() + ": " + instance.replace("~~~", " / ")) + "@" //$NON-NLS-1$
               + safeEncode(instance) + "@" + safeEncode(column.getName());//$NON-NLS-1$ //$NON-NLS-2$
         
         final IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench()
               .getActiveWorkbenchWindow().getActivePage();
         try
         {
            page.showView(HistoricalDataView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.get().TableValue_Error,
                  String.format(Messages.get().TableValue_ErrorOpeningView, e.getLocalizedMessage()));
         }
      }
   }
   
   /**
    * Show line chart
    */
   private void showLineChart()
   {
      if (currentData == null)
         return;

      ViewerCell[] cells = cellSelectionManager.getSelectedCells();
      if (cells.length == 0)
         return;

      StringBuilder sb = new StringBuilder();
      sb.append(uniqueId++);
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         final String instance = buildInstanceString(cells[i].getViewerRow());
         
         sb.append("&");
         sb.append(ChartDciConfig.TABLE);
         sb.append("@");
         sb.append(objectId);
         sb.append("@");
         sb.append(dciId);
         sb.append("@");
         sb.append(safeEncode(column.getDisplayName() + ": " + instance.replace("~~~", " / ")));
         sb.append("@");
         sb.append(safeEncode(instance));
         sb.append("@");
         sb.append(safeEncode(column.getName()));
      }

      final IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench()
            .getActiveWorkbenchWindow().getActivePage();
      try
      {
         page.showView(HistoricalGraphView.ID, sb.toString(), IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.get().TableValue_Error,
               String.format(Messages.get().TableValue_ErrorOpeningView, e.getLocalizedMessage()));
      }
   }

   /**
    * Show line chart
    */
   private void showDataComparisonChart(int chartType)
   {
      if (currentData == null)
         return;

      ViewerCell[] cells = cellSelectionManager.getSelectedCells();
      if (cells.length == 0)
         return;

      String id = Long.toString(uniqueId++) + "&" + Integer.toString(chartType); //$NON-NLS-1$
      for(int i = 0; i < cells.length; i++)
      {
         TableColumnDefinition column = currentData.getColumnDefinition(cells[i].getColumnIndex());
         String instance = buildInstanceString(cells[i].getViewerRow());
         int source = currentData.getSource();

         id += "&" + Long.toString(objectId) + "@" + Long.toString(dciId) + "@" + Integer.toString(source) + "@" //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
               + Integer.toString(column.getDataType()) + "@" + safeEncode(currentData.getTitle()) + "@" //$NON-NLS-1$ //$NON-NLS-2$
               + safeEncode(column.getDisplayName() + ": " + instance.replace("~~~", " / ")) + "@" + safeEncode(instance) + "@" //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
               + safeEncode(column.getName());
      }

      final IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench()
            .getActiveWorkbenchWindow().getActivePage();
      try
      {
         page.showView(DataComparisonView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(page.getWorkbenchWindow().getShell(), Messages.get().TableValue_Error,
               String.format(Messages.get().TableValue_ErrorOpeningView, e.getLocalizedMessage()));
      }
   }

   /**
    * @param text
    * @return
    */
   private static String safeEncode(String text)
   {
      if (text == null)
         return ""; //$NON-NLS-1$

      try
      {
         return URLEncoder.encode(text, "UTF-8"); //$NON-NLS-1$
      }
      catch(UnsupportedEncodingException e)
      {
         return "none"; //$NON-NLS-1$
      }
   }

   /**
    * @return
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @return
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * @return
    */
   public String getObjectName()
   {
      return objectName;
   }

   /**
    * @return
    */
   public String getTitle()
   {
      return (currentData != null) ? currentData.getTitle() : ("[" + dciId + "]"); //$NON-NLS-1$ //$NON-NLS-2$
   }

   /**
    * @return
    */
   public SortableTableViewer getViewer()
   {
      return viewer;
   }
   
   public Action getActionUseMultipliers()
   {
      return actionUseMultipliers;
   }
   
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      actionShowFilter.setChecked(enable);
      filterText.setVisible(enable);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      layout();
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
   public void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilterText()
   {
      return filterText.getText();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }
   
   /**
    * Get show filter action
    */
   public Action getShowFilterAction()
   {
      return actionShowFilter;
   }
}
