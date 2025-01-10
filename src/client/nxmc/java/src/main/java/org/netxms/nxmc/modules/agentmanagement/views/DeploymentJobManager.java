/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.packages.PackageDeploymentJob;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentJobComparator;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentJobFilter;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentJobLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Package deployment job manager view
 */
public class DeploymentJobManager extends ConfigurationView implements SessionListener
{
   private static final Logger logger = LoggerFactory.getLogger(DeploymentJobManager.class);
   private final I18n i18n = LocalizationHelper.getI18n(DeploymentJobManager.class);
   public static final String ID = "configuration.deployment-job-manager";

   public static final int COL_ID = 0;
   public static final int COL_NODE = 1;
   public static final int COL_USER = 2;
   public static final int COL_STATUS = 3;
   public static final int COL_ERROR_MESSAGE = 4;
   public static final int COL_CREATION_TIME = 5;
   public static final int COL_EXECUTION_TIME = 6;
   public static final int COL_COMPLETION_TIME = 7;
   public static final int COL_PACKAGE_ID = 8;
   public static final int COL_PACKAGE_TYPE = 9;
   public static final int COL_PACKAGE_NAME = 10;
   public static final int COL_PLATFORM = 11;
   public static final int COL_VERSION = 12;
   public static final int COL_FILE = 13;
   public static final int COL_DESCRIPTION = 14;

   private NXCSession session = Registry.getSession();
   private Map<Long, PackageDeploymentJob> jobs = new HashMap<>();
   private SortableTableViewer viewer;
   private DeploymentJobFilter filter;
   private Action actionCancel;
   private Action actionHideInactive;
   private Action actionExportToCsv;
   
   /**
    * Constructor
    */
   public DeploymentJobManager()
   {
      super(LocalizationHelper.getI18n(DeploymentJobManager.class).tr("Package Deployment Jobs"), ResourceManager.getImageDescriptor("icons/config-views/deployment-jobs.png"), ID, true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {     
      final String[] names = {
            i18n.tr("ID"), i18n.tr("Node"), i18n.tr("User"), i18n.tr("Status"), i18n.tr("Error message"), i18n.tr("Creation time"), i18n.tr("Execution time"), i18n.tr("Completion time"),
            i18n.tr("Package ID"), i18n.tr("Package type"), i18n.tr("Package name"), i18n.tr("Platform"), i18n.tr("Version"), i18n.tr("File"), i18n.tr("Description")
         };
      final int[] widths = { 80, 140, 140, 70, 200, 120, 120, 120, 80, 100, 150, 120, 120, 300, 300 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new DeploymentJobLabelProvider(viewer));
      viewer.setComparator(new DeploymentJobComparator());
      filter = new DeploymentJobFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, ID);
      viewer.getTable().addDisposeListener((e) -> WidgetHelper.saveTableViewerSettings(viewer, ID));

      createActions();
      createPopupMenu();

      refresh();

      session.addListener(this);
      new Job(i18n.tr("Subscribing to deployment job change notifications"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(NXCSession.CHANNEL_PACKAGE_JOBS);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot subscribe to deployment job change notifications");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      new Job(i18n.tr("Unsubscribing from deployment job change notifications"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               session.unsubscribe(NXCSession.CHANNEL_PACKAGE_JOBS);
            }
            catch(Exception e)
            {
               logger.error("Cannot remove subscription for deployment job change notifications", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      }.start();
      super.dispose();
   }

   /**
    * Create actions
    */
   private void createActions()
   {      
      actionCancel = new Action("&Cancel", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            cancelJob();
         }
      };

      actionHideInactive = new Action(i18n.tr("&Hide inactive jobs"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setHideInactive(actionHideInactive.isChecked());
            viewer.refresh();
         }
      };

      actionExportToCsv = new ExportToCsvAction(this, viewer, false);
   }

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }
   
   /**
    * Fill context menu
    * @param manager Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      for(Object o : selection.toList())
      {
         if (!((PackageDeploymentJob)o).isFinished())
         {
            manager.add(actionCancel);
            manager.add(new Separator());
            break;
         }
      }
      manager.add(actionHideInactive);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportToCsv);
      super.fillLocalMenu(manager);
   }  
   
   /**
    * Refresh view
    */
   public void refresh()
   {
      new Job(i18n.tr("Reading list of package deployment jobs"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<PackageDeploymentJob> jobList = session.getPackageDeploymentJobs();
            runInUIThread(() -> {
               jobs.clear();
               for(PackageDeploymentJob j : jobList)
                  jobs.put(j.getId(), j);
               viewer.setInput(jobs.values());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of package deployment jobs");
         }
      }.start();
   }

   /**
    * Cancel job
    */
   private void cancelJob()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Cancel Job"), i18n.tr("Selected jobs will be cancelled. Are you sure?")))
         return;

      final Object[] jobs = selection.toArray();
      new Job(i18n.tr("Cancelling package deployment jobs"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : jobs)
            {
               PackageDeploymentJob j = (PackageDeploymentJob)o;
               if (j.isFinished())
                  continue;
               session.cancelPackageDeploymentJob(j.getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot cancel package deployment job");
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

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.PACKAGE_DEPLOYMENT_JOB_CHANGED)
      {
         getDisplay().asyncExec(() -> {
            PackageDeploymentJob j = (PackageDeploymentJob)n.getObject();
            jobs.put(j.getId(), j);
            viewer.refresh();
         });
      }
   }
}
