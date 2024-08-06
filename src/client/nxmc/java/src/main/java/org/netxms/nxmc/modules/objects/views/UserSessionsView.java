/**
 * NetXMS - open source network management system
 * Copyright (C) 2019-2023 Raden Solutions
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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.UserSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.UserSessionComparator;
import org.netxms.nxmc.modules.objects.views.helpers.UserSessionFilter;
import org.netxms.nxmc.modules.objects.views.helpers.UserSessionLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.xnap.commons.i18n.I18n;

/**
 * User session tab tab
 */
public class UserSessionsView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(UserSessionsView.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_USER = 1;
   public static final int COLUMN_TERMINAL = 2;
   public static final int COLUMN_STATE = 3;
   public static final int COLUMN_CLIENT_NAME = 4;
   public static final int COLUMN_CLIENT_ADDRESS = 5;
   public static final int COLUMN_DISPLAY = 6;
   public static final int COLUMN_LOGIN_TIME = 7;
   public static final int COLUMN_IDLE_TIME = 8;
   public static final int COLUMN_AGENT_TYPE = 9;
   public static final int COLUMN_AGENT_PID = 10;

   private SortableTableViewer viewer;
   private ViewRefreshController refreshController;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionTakeScreenshot;

   /**
    * Create "User Sessions" view
    */
   public UserSessionsView()
   {
      super(LocalizationHelper.getI18n(UserSessionsView.class).tr("User Sessions"), ResourceManager.getImageDescriptor("icons/object-views/user-sessions.png"), "objects.user-sessions", true);
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
      return 75;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("ID"), i18n.tr("User"), i18n.tr("Terminal"), i18n.tr("State"), i18n.tr("Client name"), i18n.tr("Client address"), i18n.tr("Display"), i18n.tr("Login timestamp"),
            i18n.tr("Idle time"), i18n.tr("Agent type"), i18n.tr("Agent PID") };
      final int[] widths = { 100, 150, 150, 100, 150, 150, 150, 180, 120, 150, 120 };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setLabelProvider(new UserSessionLabelProvider());
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new UserSessionComparator());

      UserSessionFilter filter = new UserSessionFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      VisibilityValidator validator = new VisibilityValidator() {
         @Override
         public boolean isVisible()
         {
            return isActive();
         }
      };

      refreshController = new ViewRefreshController(this, -1, new Runnable() {
         @Override
         public void run()
         {
            if (viewer.getTable().isDisposed())
               return;

            refresh();
         }
      }, validator);
      refreshController.setInterval(30);

      createActions();
      createContextMenu();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      refreshController.dispose();
      super.dispose();
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getTable());
      viewer.getTable().setMenu(menu);
   }

   /**
    * Fill context menu 
    * 
    * @param manager menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() >= 1))
      {
         manager.add(actionExportToCsv);
         manager.add(actionExportAllToCsv);
         manager.add(new Separator());
         manager.add(actionTakeScreenshot);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
      super.fillLocalMenu(manager);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionExportToCsv = new ExportToCsvAction(this, viewer, true);
      actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);

      actionTakeScreenshot = new Action(i18n.tr("Take &screenshot"), ResourceManager.getImageDescriptor("icons/screenshot.png")) {
         @Override
         public void run()
         {
            takeScreenshot();
         }
      };
      addKeyBinding("M1+T", actionTakeScreenshot);
   }

   /**
    * Take screenshot action implementation
    */
   private void takeScreenshot()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() >= 1))
      {
         for(Object o : selection.toList())
         {
            UserSession s = (UserSession)o;
            openView(new ScreenshotView((AbstractNode)getObject(), s.getTerminal(), s.getLoginName(), getObjectId()));
         }
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (!isActive())
         return;

      final long objectId = getObjectId();
      if (objectId == 0)
      {
         viewer.setInput(new Object[0]);
         return;
      }

      Job job = new Job(i18n.tr("Reading user session list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<UserSession> sessions = session.getUserSessions(objectId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  viewer.setInput(sessions);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of user sessions");
         }
      };
      job.setUser(false);
      job.start();
   }
}
