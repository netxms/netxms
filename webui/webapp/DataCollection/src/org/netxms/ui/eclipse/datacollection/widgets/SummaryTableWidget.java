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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.widgets.internal.TableContentProvider;
import org.netxms.ui.eclipse.datacollection.widgets.internal.TableItemComparator;
import org.netxms.ui.eclipse.datacollection.widgets.internal.TableLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * DCI summary table display widget
 */
public class SummaryTableWidget extends Composite
{
   private int tableId;
   private long baseObjectId;
   private IViewPart viewPart;
   private SortableTableViewer viewer;
   private Action actionExportToCsv;
   private Runnable timer;
   private int autoRefreshInterval = 0;

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

      viewer = new SortableTableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new TableContentProvider());
      viewer.setLabelProvider(new TableLabelProvider());
      
      actionExportToCsv = new ExportToCsvAction(viewPart, viewer, true);
      
      createPopupMenu();
      
      timer = new Runnable() {
         @Override
         public void run()
         {
            if (isDisposed())
               return;
            refresh();
            if (autoRefreshInterval > 0)
               getDisplay().timerExec(autoRefreshInterval, timer);
         }
      };
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
         WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), 
               viewPart.getViewSite().getId() + ".SummaryTable." + Integer.toString(tableId)); //$NON-NLS-1$
         viewer.getTable().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), 
                     viewPart.getViewSite().getId() + ".SummaryTable." + Integer.toString(tableId)); //$NON-NLS-1$
            }
         });
         viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));
      }
      viewer.setInput(table);
   }

   /**
    * @return the viewer
    */
   public SortableTableViewer getViewer()
   {
      return viewer;
   }
   
   /**
    * Set automatic refresh interval. If less or equal 0, automatic refresh will be disabled.
    * 
    * @param interval auto refresh interval in seconds
    */
   public void setAutoRefresh(int interval)
   {
      getDisplay().timerExec(-1, timer);  // kill existing timer if any
      autoRefreshInterval = interval * 1000;
      if (autoRefreshInterval > 0)
         getDisplay().timerExec(autoRefreshInterval, timer);
   }
}
