/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.reporting.views;

import java.io.File;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.reporting.ReportPackageInfo;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Report definition package manager
 */
public class ReportDefinitionManager extends ConfigurationView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(ReportDefinitionManager.class);

   private static final String TABLE_CONFIG_PREFIX = "ReportDefinitionManager";

   public static final int COLUMN_FILE_NAME = 0;
   public static final int COLUMN_REPORT_NAME = 1;
   public static final int COLUMN_STATUS = 2;
   public static final int COLUMN_GUID = 3;
   public static final int COLUMN_MODIFIED = 4;

   private NXCSession session = Registry.getSession();
   private SortableTableViewer viewer;
   private String filterString = "";
   private Action actionUpload;
   private Action actionDelete;

   /**
    * Create report definition manager view
    */
   public ReportDefinitionManager()
   {
      super(LocalizationHelper.getI18n(ReportDefinitionManager.class).tr("Report Definitions"), ResourceManager.getImageDescriptor("icons/config-views/reports.png"), "config.report-definitions", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      final String[] columnNames = { i18n.tr("File"), i18n.tr("Report name"), i18n.tr("Status"), i18n.tr("GUID"), i18n.tr("Modified") };
      final int[] columnWidths = { 200, 300, 100, 280, 200 };
      viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_FILE_NAME, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ReportPackageLabelProvider());
      viewer.setComparator(new ReportPackageComparator());
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            if (filterString.isEmpty())
               return true;
            ReportPackageInfo p = (ReportPackageInfo)element;
            return p.getFileName().toLowerCase().contains(filterString) ||
                  (p.getReportName() != null && p.getReportName().toLowerCase().contains(filterString));
         }
      });
      setFilterClient(viewer, new AbstractViewerFilter() {
         @Override
         public void setFilterString(String string)
         {
            filterString = string.toLowerCase();
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      createActions();
      createContextMenu();
      session.addListener(this);

      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionUpload = new Action(i18n.tr("&Upload..."), SharedIcons.UPLOAD) {
         @Override
         public void run()
         {
            uploadFile();
         }
      };
      addKeyBinding("M1+U", actionUpload);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteFile();
         }
      };
      actionDelete.setEnabled(false);
      addKeyBinding("M1+D", actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionUpload);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionUpload);
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
      Action autoSizeAction = viewer.getAutoSizeColumnsAction();
      if (autoSizeAction != null)
      {
         manager.add(new Separator());
         manager.add(autoSizeAction);
      }
   }

   /**
    * Create context menu for file list
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param mgr menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      if (!viewer.getStructuredSelection().isEmpty())
      {
         mgr.add(actionDelete);
         mgr.add(new Separator());
      }
      mgr.add(actionUpload);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Reading report package list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final ReportPackageInfo[] packages = session.listReportPackages();
            runInUIThread(() -> viewer.setInput(packages));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of report packages");
         }
      }.start();
   }

   /**
    * Upload file(s)
    */
   private void uploadFile()
   {
      FileDialog fd = new FileDialog(getWindow().getShell(), SWT.OPEN | SWT.MULTI);
      fd.setText(i18n.tr("Select Report Packages"));
      WidgetHelper.setFileDialogFilterExtensions(fd, new String[] { "*.jar;*.zip", "*.*" });
      WidgetHelper.setFileDialogFilterNames(fd, new String[] { i18n.tr("Report packages (*.jar, *.zip)"), i18n.tr("All files") });
      if (fd.open() == null)
         return;

      final List<File> files = new ArrayList<File>();
      WidgetHelper.getFileDialogFileList(fd, files);
      if (files.isEmpty())
         return;
      new Job(i18n.tr("Uploading report definition"), this) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            for(final File localFile : files)
            {
               final String remoteFile = localFile.getName();
               session.uploadReportDefinition(localFile, remoteFile, new ProgressListener() {
                  private long prevWorkDone = 0;

                  @Override
                  public void setTotalWorkAmount(long workTotal)
                  {
                     monitor.beginTask(localFile.getAbsolutePath(), (int)workTotal);
                  }

                  @Override
                  public void markProgress(long workDone)
                  {
                     monitor.worked((int)(workDone - prevWorkDone));
                     prevWorkDone = workDone;
                  }
               });
               monitor.done();
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot upload report definition");
         }
      }.start();
   }

   /**
    * Delete selected file(s)
    */
   private void deleteFile()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Delete Report Package"), i18n.tr("Are you sure you want to delete selected report packages?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Deleting report packages"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteReportPackage(((ReportPackageInfo)objects[i]).getFileName());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete report package");
         }
      }.start();
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
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.RS_DEFINITIONS_MODIFIED)
      {
         getDisplay().asyncExec(() -> refresh());
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      super.dispose();
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
    * Label provider for report package table
    */
   private class ReportPackageLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private DateFormat dateFormat = DateFormatFactory.getDateTimeFormat();

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
         ReportPackageInfo p = (ReportPackageInfo)element;
         switch(columnIndex)
         {
            case COLUMN_FILE_NAME:
               return p.getFileName();
            case COLUMN_REPORT_NAME:
               return (p.getReportName() != null) ? p.getReportName() : "";
            case COLUMN_STATUS:
               return p.isDeployed() ? i18n.tr("Deployed") : i18n.tr("Not deployed");
            case COLUMN_GUID:
               return (p.getGuid() != null) ? p.getGuid().toString() : "";
            case COLUMN_MODIFIED:
               return (p.getModificationTime() != null) ? dateFormat.format(p.getModificationTime()) : "";
            default:
               return "";
         }
      }
   }

   /**
    * Comparator for report package table
    */
   private class ReportPackageComparator extends ViewerComparator
   {
      /**
       * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
       */
      @Override
      public int compare(Viewer viewer, Object e1, Object e2)
      {
         TableColumn sortColumn = ((SortableTableViewer)viewer).getTable().getSortColumn();
         if (sortColumn == null)
            return 0;

         ReportPackageInfo p1 = (ReportPackageInfo)e1;
         ReportPackageInfo p2 = (ReportPackageInfo)e2;

         int result;
         switch((Integer)sortColumn.getData("ID"))
         {
            case COLUMN_FILE_NAME:
               result = p1.getFileName().compareToIgnoreCase(p2.getFileName());
               break;
            case COLUMN_REPORT_NAME:
               result = safeCompare(p1.getReportName(), p2.getReportName());
               break;
            case COLUMN_STATUS:
               result = Boolean.compare(p1.isDeployed(), p2.isDeployed());
               break;
            case COLUMN_GUID:
               result = safeCompare(p1.getGuid() != null ? p1.getGuid().toString() : null, p2.getGuid() != null ? p2.getGuid().toString() : null);
               break;
            case COLUMN_MODIFIED:
               result = Long.compare(p1.getModificationTime() != null ? p1.getModificationTime().getTime() : 0,
                     p2.getModificationTime() != null ? p2.getModificationTime().getTime() : 0);
               break;
            default:
               result = 0;
               break;
         }
         int dir = ((SortableTableViewer)viewer).getTable().getSortDirection();
         return (dir == SWT.UP) ? result : -result;
      }

      /**
       * Safe string compare handling nulls.
       */
      private int safeCompare(String s1, String s2)
      {
         if (s1 == null)
            return (s2 == null) ? 0 : -1;
         if (s2 == null)
            return 1;
         return s1.compareToIgnoreCase(s2);
      }
   }
}
