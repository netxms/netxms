/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.filemanager.views;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ColumnViewerEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationStrategy;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewerEditor;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.server.ServerFile;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.views.helpers.ServerFileComparator;
import org.netxms.nxmc.modules.filemanager.views.helpers.ServerFileLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Server files manager
 */
public class ServerFileManager extends ConfigurationView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(ServerFileManager.class);
   private static final String TABLE_CONFIG_PREFIX = "ServerFileManager";

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_SIZE = 2;
   public static final int COLUMN_MODIFYED = 3;

   private NXCSession session = Registry.getSession();
   private SortableTableViewer viewer;
   private String filterString = "";
   private Action actionUpload;
   private Action actionRename;
   private Action actionDelete;

   /**
    * Create server configuration variable view
    */
   public ServerFileManager()
   {
      super(LocalizationHelper.getI18n(ServerFileManager.class).tr("Server Files"), ResourceManager.getImageDescriptor("icons/config-views/server-files.png"), "ServerFileManager", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Size"), i18n.tr("Modified") };
      final int[] columnWidths = { 300, 150, 300, 300 };
      viewer = new SortableTableViewer(parent, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ServerFileLabelProvider());
      viewer.setComparator(new ServerFileComparator());
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            return filterString.isEmpty() || ((ServerFile)element).getName().toLowerCase().contains(filterString);
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
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });
      TableViewerEditor.create(viewer, new ColumnViewerEditorActivationStrategy(viewer) {
         @Override
         protected boolean isEditorActivationEvent(ColumnViewerEditorActivationEvent event)
         {
            return event.eventType == ColumnViewerEditorActivationEvent.PROGRAMMATIC;
         }
      }, ColumnViewerEditor.DEFAULT);
      TextCellEditor editor = new TextCellEditor(viewer.getTable(), SWT.BORDER);
      viewer.setCellEditors(new CellEditor[] { editor });
      viewer.setColumnProperties(new String[] { "name" }); //$NON-NLS-1$
      viewer.setCellModifier(new ICellModifier() {
         @Override
         public void modify(Object element, String property, Object value)
         {
            final Object data = (element instanceof Item) ? ((Item)element).getData() : element;
            if (property.equals("name") && (data instanceof ServerFile)) //$NON-NLS-1$
            {
               final String newName = value.toString();
               new Job(i18n.tr("Rename server file"), ServerFileManager.this) {
                  @Override
                  protected void run(IProgressMonitor monitor) throws Exception
                  {
                     session.renameServerFile(((ServerFile)data).getName(), newName);
                  }

                  @Override
                  protected String getErrorMessage()
                  {
                     return String.format(i18n.tr("Cannot rename server file \"%s\""), ((ServerFile)data).getName());
                  }
               }.start();
            }
         }

         @Override
         public Object getValue(Object element, String property)
         {
            if (property.equals("name") && (element instanceof ServerFile)) //$NON-NLS-1$
               return ((ServerFile)element).getName();
            return null;
         }

         @Override
         public boolean canModify(Object element, String property)
         {
            return property.equals("name"); //$NON-NLS-1$
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

      actionRename = new Action("&Rename") {
         @Override
         public void run()
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            if (selection.size() == 1)
               viewer.editElement(selection.getFirstElement(), 0);
         }
      };
      addKeyBinding("F2", actionRename);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteFile();
         }
      };
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
    * Create context menu for file list
    */
   private void createContextMenu()
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
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      if (!viewer.getStructuredSelection().isEmpty())
      {
         mgr.add(actionRename);
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
      new Job(i18n.tr("Reading server file list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final ServerFile[] files = session.listServerFiles();
            runInUIThread(() -> viewer.setInput(files));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of server files");
         }
      }.start();
   }

   /**
    * Upload file(s)
    */
   private void uploadFile()
   {
      FileDialog fd = new FileDialog(getWindow().getShell(), SWT.OPEN | SWT.MULTI);
      fd.setText("Select Files");
      String selection = fd.open();
      if (selection == null)
         return;

      final List<File> files = new ArrayList<File>();
      for(String fname : fd.getFileNames())
      {
         files.add((fname.charAt(0) == File.separatorChar) ? new File(fname) : new File(new File(selection).getParentFile(), fname));
      }
      new Job(i18n.tr("Uploading file to server"), this) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            for(final File localFile : files)
            {
               final String remoteFile = localFile.getName();
               session.uploadFileToServer(localFile, remoteFile, new ProgressListener() {
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
            return i18n.tr("Cannot upload file to server");
         }
      }.start();
   }

   /**
    * Delete selected file
    */
   private void deleteFile()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Delete File"), i18n.tr("Are you sure you want to delete selected files?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Deleting server files"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteServerFile(((ServerFile)objects[i]).getName());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete server file");
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
      if ((n.getCode() == SessionNotification.FILE_LIST_CHANGED) && (n.getSubCode() == 0))
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
}
