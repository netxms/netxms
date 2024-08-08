/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.CopyTableCellsAction;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.views.helpers.OverviewDciLabelProvider;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * DCI last values
 */
public class LastValues extends OverviewPageElement
{
   private static Logger logger = LoggerFactory.getLogger(LastValues.class);

   private final I18n i18n = LocalizationHelper.getI18n(LastValues.class);

   private TableViewer viewer;
   private ViewRefreshController refreshController;
   private Action actionCopy;
   private Action actionCopyName;
   private Action actionCopyValue;

   /**
    * @param parent
    * @param anchor
    * @param objectTab
    */
   public LastValues(Composite parent, OverviewPageElement anchor, ObjectView objectView)
   {
      super(parent, anchor, objectView);
      refreshController = new ViewRefreshController(objectView, -1, () -> refresh());
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return i18n.tr("Last Values");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      if (!(object instanceof DataCollectionTarget))
         return false;
      return !((DataCollectionTarget)object).getOverviewDciData().isEmpty();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {
      viewer = new TableViewer(parent, SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.H_SCROLL | SWT.MULTI);
      setupTable();
     
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new OverviewDciLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((DciValue)e1).getDescription().compareToIgnoreCase(((DciValue)e2).getDescription());
         }
      });

      viewer.setInput(((DataCollectionTarget)getObject()).getOverviewDciData().toArray());
      adjustCollumns();

      createActions();
      createPopupMenu();

      refreshController.setInterval(30);
      refresh();

      return viewer.getTable();
   }

   /**
    * Create actions required for popup menu
    */
   protected void createActions()
   {
      actionCopy = new CopyTableRowsAction(viewer, true);
      actionCopyName = new CopyTableCellsAction(viewer, 0, true, i18n.tr("Copy &name to clipboard"));
      actionCopyValue = new CopyTableCellsAction(viewer, 1, true, i18n.tr("Copy &value to clipboard"));
   }

   /**
    * Adjust column size
    */
   private void adjustCollumns()
   {
      for(TableColumn cl : viewer.getTable().getColumns())
      {
         cl.pack();
         cl.setWidth(cl.getWidth() + 10); // compensate for pack issues on Linux         
      }
   }

   /**
    * Setup table widget
    */
   private void setupTable()
   {
      TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText(i18n.tr("Metric"));
      tc.setWidth(300);

      tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText(i18n.tr("Value"));
      tc.setWidth(100);

      tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText(i18n.tr("Timestamp"));
      tc.setWidth(100);

      viewer.getTable().setHeaderVisible(false);
      viewer.getTable().setLinesVisible(false);
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
      manager.add(actionCopy);
      manager.add(actionCopyName);
      manager.add(actionCopyValue);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
   @Override
   protected void onObjectChange()
   {
      viewer.setInput(((DataCollectionTarget)getObject()).getOverviewDciData().toArray());
      adjustCollumns();
   }
   
   /**
    * Refresh element
    */
   private void refresh()
   {
      final NXCSession session = Registry.getSession();
      final long nodeId = getObject().getObjectId();
      Job job = new Job(i18n.tr("Read last DCI values"), getObjectView()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final DciValue[] list = session.getDataCollectionSummary(nodeId, false, true, false);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (viewer.getControl().isDisposed() || (getObject().getObjectId() != nodeId))
                        return;
                     viewer.setInput(list);
                     adjustCollumns();
                  }
               });
            }
            catch(Exception e)
            {
               logger.error("Exception in last values overview element", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read last DCI values");
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }
}
