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
package org.netxms.nxmc.modules.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
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
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.TwoFactorAuthMethodEditDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.TwoFactorMethodFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.TwoFactorMethodLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.TwoFactorMethodListComparator;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Manager of two-factor authentication methods
 */
public class TwoFactorAuthenticationMethods extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(TwoFactorAuthenticationMethods.class);

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DRIVER = 1;
   public static final int COLUMN_STATUS = 2;
   public static final int COLUMN_DESCRIPTION = 3;

   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private Action actionNewMethod;
   private Action actionEditMethod;
   private Action actionDeleteMethod;

   /**
    * Create "SSH keys" view
    */
   public TwoFactorAuthenticationMethods()
   {
      super(LocalizationHelper.getI18n(TwoFactorAuthenticationMethods.class).tr("Two-Factor Authentication Methods"), ResourceManager.getImageDescriptor("icons/config-views/2fa.png"),
            "config-2fa-methods", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
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
      TwoFactorMethodFilter filter = new TwoFactorMethodFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "TwoFactorAuthenticationMethods");
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "TwoFactorAuthenticationMethods");
         }
      });

      createActions();
      createPopupMenu();

      final Display display = getDisplay();
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
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (listener != null)
         session.removeListener(listener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
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
      actionNewMethod = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewMethod();
         }
      };
      addKeyBinding("M1+N", actionNewMethod);

      actionEditMethod = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editMethod();
         }
      };
      actionEditMethod.setEnabled(false);
      addKeyBinding("M3+ENTER", actionEditMethod);

      actionDeleteMethod = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteMethods();
         }
      };
      actionDeleteMethod.setEnabled(false);
      addKeyBinding("M1+D", actionDeleteMethod);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionNewMethod);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNewMethod);
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
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading two-factor authentication method list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot get list of two-factor authentication methods");
         }
      }.start();
   }

   /**
    * Create new method
    */
   private void createNewMethod()
   {
      final TwoFactorAuthMethodEditDialog dlg = new TwoFactorAuthMethodEditDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final TwoFactorAuthenticationMethod method = dlg.getMethod();
      new Job(i18n.tr("Creating two-factor authentication method"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modify2FAMethod(method);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create two-factor authentication method");
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
      final TwoFactorAuthMethodEditDialog dlg = new TwoFactorAuthMethodEditDialog(getWindow().getShell(), method);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Updating two-factor authentication method"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot update two-factor authentication method");
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

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Methods"), i18n.tr("Are you sure you want to delete selected methods?")))
         return;

      final List<String> methods = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof TwoFactorAuthenticationMethod)
            methods.add(((TwoFactorAuthenticationMethod)o).getName());
      }

      new Job(i18n.tr("Delete two-factor authentication method"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String name : methods)
               session.delete2FAMethod(name);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete two-factor authentication method");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
