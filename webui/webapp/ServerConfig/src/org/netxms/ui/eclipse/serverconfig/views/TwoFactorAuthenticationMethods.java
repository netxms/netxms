/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.TwoFactorAuthMethodEditDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.TwoFactorMethodLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.TwoFactorMethodListComparator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Manager of two-factor authentication methods
 */
public class TwoFactorAuthenticationMethods extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.TwoFactorAuthenticationMethods"; //$NON-NLS-1$

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DRIVER = 1;
   public static final int COLUMN_STATUS = 2;
   public static final int COLUMN_DESCRIPTION = 3;

   private NXCSession session = ConsoleSharedData.getSession();
   private SessionListener listener;
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionNewMethod;
   private Action actionEditMethod;
   private Action actionDeleteMethod;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      final int[] widths = { 160, 120, 80, 500 };
      final String[] names = { "Name", "Driver", "Status", "Description" };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TwoFactorMethodLabelProvider());
      viewer.setComparator(new TwoFactorMethodListComparator());
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editMethod();
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            actionEditMethod.setEnabled(selection.size() == 1);
            actionDeleteMethod.setEnabled(selection.size() > 0);
         }
      });

      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      WidgetHelper.restoreTableViewerSettings(viewer, settings, "TwoFactorAuthenticationMethods"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, settings, "TwoFactorAuthenticationMethods"); //$NON-NLS-1$
         }
      });

      createActions();
      contributeToActionBars();
      createPopupMenu();

      refresh();

      final Display display = getSite().getShell().getDisplay();
      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.TWO_FACTOR_AUTH_METHOD_CHANGED)
            {
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               });
            }
         }
      };
      session.addListener(listener);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      if (listener != null)
         session.removeListener(listener);
      super.dispose();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };

      actionNewMethod = new Action("&New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewMethod();
         }
      };

      actionEditMethod = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editMethod();
         }
      };
      actionEditMethod.setEnabled(false);

      actionDeleteMethod = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteMethods();
         }
      };
      actionDeleteMethod.setEnabled(false);
   }

   /**
    * Fill action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * @param manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionNewMethod);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNewMethod);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Create pop-up menu for variable list
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
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      mgr.add(actionNewMethod);
      mgr.add(actionEditMethod);
      mgr.add(actionDeleteMethod);
   }

   /**
    * Refresh
    */
   private void refresh()
   {
      new ConsoleJob("Get two-factor authentication methods", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<TwoFactorAuthenticationMethod> methods = session.get2FAMethods();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(methods);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of two-factor authentication methods";
         }
      }.start();
   }

   /**
    * Create new method
    */
   private void createNewMethod()
   {
      final TwoFactorAuthMethodEditDialog dlg = new TwoFactorAuthMethodEditDialog(getSite().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final TwoFactorAuthenticationMethod method = dlg.getMethod();
      new ConsoleJob("Create two-factor authentication method", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modify2FAMethod(method);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot create two-factor authentication method";
         }
      }.start();
   }

   /**
    * Edit selected method
    */
   private void editMethod()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final TwoFactorAuthenticationMethod method = (TwoFactorAuthenticationMethod)selection.getFirstElement();
      final TwoFactorAuthMethodEditDialog dlg = new TwoFactorAuthMethodEditDialog(getSite().getShell(), method);
      if (dlg.open() != Window.OK)
         return;

      new ConsoleJob("Update two-factor authentication method", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modify2FAMethod(method);
            if (dlg.isNameChanged())
            {
               session.rename2FAMethod(method.getName(), dlg.getNewName());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update two-factor authentication method";
         }
      }.start();
   }

   /**
    * Delete selected methods
    */
   private void deleteMethods()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Delete Methods", "Are you sure you want to delete selected methods?"))
         return;

      final List<String> methods = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof TwoFactorAuthenticationMethod)
            methods.add(((TwoFactorAuthenticationMethod)o).getName());
      }

      new ConsoleJob("Delete two-factor authentication method", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(String name : methods)
               session.delete2FAMethod(name);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete two-factor authentication method";
         }
      }.start();
   }
}
