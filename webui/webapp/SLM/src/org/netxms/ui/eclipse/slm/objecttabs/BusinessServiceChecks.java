/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.ui.eclipse.slm.objecttabs;

import java.util.Arrays;
import java.util.Collection;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.slm.Activator;
import org.netxms.ui.eclipse.slm.dialogs.EditBusinessServiceCheckDlg;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceCheckFilter;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceCheckLabelProvider;
import org.netxms.ui.eclipse.slm.objecttabs.helpers.BusinessServiceComparator;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Checks for business services
 */
public class BusinessServiceChecks extends ObjectTab
{
   public static final String ID = "org.netxms.ui.eclipse.slm.objecttabs.BusinessServiceChecks";

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_TYPE = 2;
   public static final int COLUMN_OBJECT = 3;
   public static final int COLUMN_DCI = 4;
   public static final int COLUMN_STATUS = 5;
   public static final int COLUMN_FAIL_REASON = 6;

   private NXCSession session;
   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private BusinessServiceCheckLabelProvider labelProvider;
   private Action actionEdit;
   private Action actionCreate;
   private Action actionDelete;
   private Map<Long, BusinessServiceCheck> checks;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   { 
      session = ConsoleSharedData.getSession();

      // Setup table columns
      final String[] names = { "ID", "Description", "Type", "Object", "DCI", "Status", "Reason" };
      final int[] widths = { 70, 200, 100, 200, 200, 70, 300 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
      labelProvider = new BusinessServiceCheckLabelProvider();
      viewer.addFilter(new BusinessServiceCheckFilter(labelProvider));
      viewer.setComparator(new BusinessServiceComparator(labelProvider));
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = event.getStructuredSelection();
            actionEdit.setEnabled(selection.size() == 1);
            actionDelete.setEnabled(selection.size() > 0);
         }
      });
      
      WidgetHelper.restoreColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), ID);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), ID);
         }
      });

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      createActions();
      createPopupMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.BUSINESS_SERVICE_CHECK_MODIFY:
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        checks.put(n.getSubCode(), (BusinessServiceCheck)n.getObject());
                        updateDciLabels(Arrays.asList((BusinessServiceCheck)n.getObject()));
                        viewer.refresh();
                     }
                  });
                  break;
               case SessionNotification.BUSINESS_SERVICE_CHECK_DELETE:
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        checks.remove(n.getSubCode());
                        viewer.refresh();
                     }
                  });
                  break;
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
      actionCreate = new Action("&New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createCheck();
         }
      };

      actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editCheck();
         }
      };

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteCheck();
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
      manager.add(actionEdit);
      manager.add(actionCreate);
      manager.add(actionDelete);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean showForObject(AbstractObject object)
   {
      return object instanceof BusinessService;
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void objectChanged(AbstractObject object)
   {
      viewer.setInput(new Object[0]);
      refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
   @Override
   public void refresh()
   {
      new ConsoleJob("Get business service checks", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            Map<Long, BusinessServiceCheck> checks = session.getBusinessServiceChecks(getObject().getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  BusinessServiceChecks.this.checks = checks;
                  viewer.setInput(checks.values());
                  updateDciLabels(checks.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot get checks for business service %s", getObject().getObjectName());
         }
      }.start();
   }

   /**
    * Update DCI labels in check list
    * 
    * @param checks
    */
   public void updateDciLabels(Collection<BusinessServiceCheck> checks)
   {
      ConsoleJob job = new ConsoleJob("Resolve DCI names", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Map<Long, String> names = session.dciIdsToNames(checks);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  labelProvider.updateDciNames(names);
                  viewer.refresh(true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot resolve DCI names";
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Delete selected check(s)
    */
   protected void deleteCheck()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getViewPart().getSite().getShell(), "Confirm Delete", "Do you really want to delete selected check?"))
         return;

      final Object[] objects = selection.toArray();
      new ConsoleJob("Delete business service check", getViewPart(), Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteBusinessServiceCheck(getObject().getObjectId(), ((BusinessServiceCheck)objects[i]).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete business service check";
         }
      }.start();
   }

   /**
    * Edit selected check
    */
   protected void editCheck()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final BusinessServiceCheck check = new BusinessServiceCheck((BusinessServiceCheck)selection.getFirstElement());
      final EditBusinessServiceCheckDlg dlg = new EditBusinessServiceCheckDlg(getViewPart().getSite().getShell(), check, false);
      if (dlg.open() == Window.OK)
      {
         new ConsoleJob("Update business service check", getViewPart(), Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.modifyBusinessServiceCheck(getObject().getObjectId(), check);
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot update business service check";
            }
         }.start();
      }
   }

   /**
    * Create new check
    */
   private void createCheck()
   {
      final BusinessServiceCheck check = new BusinessServiceCheck();
      final EditBusinessServiceCheckDlg dlg = new EditBusinessServiceCheckDlg(getViewPart().getSite().getShell(), check, false);
      if (dlg.open() == Window.OK)
      {
         new ConsoleJob("Create business service check", getViewPart(), Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.modifyBusinessServiceCheck(getObject().getObjectId(), check);
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot create business service check";
            }
         }.start();
      }
   }
}
