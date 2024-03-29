/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.ui.eclipse.filemanager.views;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.server.ServerFile;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.filemanager.Activator;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.views.helpers.ServerFileComparator;
import org.netxms.ui.eclipse.filemanager.views.helpers.ServerFileFilter;
import org.netxms.ui.eclipse.filemanager.views.helpers.ServerFileLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Editor for server files
 */
public class ServerFileManager extends ViewPart implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.usermanager.view.server_files"; //$NON-NLS-1$

   private static final String TABLE_CONFIG_PREFIX = "ServerFilesEditor"; //$NON-NLS-1$

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_SIZE = 2;
   public static final int COLUMN_MODIFYED = 3;

   private boolean initShowFilter = true;
   private Composite content;
   private ServerFile[] files;
   private ServerFileFilter filter;
   private FilterText filterText;
   private SortableTableViewer viewer;
   private NXCSession session;
   private Action actionRefresh;
   private Action actionUpload;
   private Action actionRename;
   private Action actionDelete;
   private Action actionShowFilter;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      IDialogSettings settings = Activator.getDefault().getDialogSettings(); 
      initShowFilter = safeCast(settings.get("ServerFileManager.showFilter"), settings.getBoolean("ServerFileManager.showFilter"), initShowFilter);
   }

   /**
    * @param b
    * @param defval
    * @return
    */
   private static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      session = ConsoleSharedData.getSession();

      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FormLayout());

      // Create filter area
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });

      final String[] columnNames = { Messages.get().ViewServerFile_FileName, Messages.get().ViewServerFile_FileType,
            Messages.get().ViewServerFile_FileSize, Messages.get().ViewServerFile_ModificationDate };
      final int[] columnWidths = { 300, 150, 300, 300 };
      viewer = new SortableTableViewer(content, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ServerFileLabelProvider());
      viewer.setComparator(new ServerFileComparator());
      filter = new ServerFileFilter();
      viewer.addFilter(filter);
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
            WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
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
               new ConsoleJob("Rename server file", ServerFileManager.this, Activator.PLUGIN_ID) {
                  @Override
                  protected void runInternal(IProgressMonitor monitor) throws Exception
                  {
                     session.renameServerFile(((ServerFile)data).getName(), newName);
                  }

                  @Override
                  protected String getErrorMessage()
                  {
                     return String.format("Cannot rename server file \"%s\"", ((ServerFile)data).getName());
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

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getTable().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createActions();
      contributeToActionBars();
      createPopupMenu();
      activateContext();
      session.addListener(this);

      filterText.setCloseAction(actionShowFilter);

      // Set initial focus to filter input line
      if (initShowFilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      refreshFileList();
   }

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.filemanager.context.FileManager"); //$NON-NLS-1$
      }
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refreshFileList();
         }
      };

      actionUpload = new Action("&Upload...", SharedIcons.UPLOAD) {
         @Override
         public void run()
         {
            uploadFile();
         }
      };

      actionRename = new Action("&Rename") {
         @Override
         public void run()
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            if (selection.size() == 1)
               viewer.editElement(selection.getFirstElement(), 0);
         }
      };

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteFile();
         }
      };

      actionShowFilter = new Action(Messages.get().ViewServerFile_ShowFilterAction, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(!initShowFilter);
            actionShowFilter.setChecked(initShowFilter);
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(initShowFilter);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.filemanager.commands.showFilter"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
   }

   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionUpload);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local tool bar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionUpload);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

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
      mgr.add(actionRename);
      mgr.add(actionDelete);
      mgr.add(new Separator());
      mgr.add(actionUpload);
   }

   /**
    * Refresh file list
    */
   private void refreshFileList()
   {
      new ConsoleJob(Messages.get().SelectServerFileDialog_JobTitle, this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            files = session.listServerFiles();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(files);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().SelectServerFileDialog_JobError;
         }
      }.start();
   }

   /**
    * Upload file
    */
   private void uploadFile()
   {
      FileDialog fd = new FileDialog(getSite().getShell(), SWT.OPEN | SWT.MULTI);
      fd.setText("Select Files");
      String selection = fd.open();
      if (selection == null)
         return;

      final List<File> files = new ArrayList<File>();
      for(String fname : fd.getFileNames())
      {
         files.add((fname.charAt(0) == File.separatorChar) ? new File(fname) : new File(new File(selection).getParentFile(), fname));
      }
      new ConsoleJob(Messages.get().UploadFileToServer_JobTitle, this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(final IProgressMonitor monitor) throws Exception
         {
            for(final File localFile : files)
            {
               final String remoteFile = localFile.getName();
               session.uploadFileToServer(localFile, remoteFile, new ProgressListener() {
                  private long prevWorkDone = 0;

                  @Override
                  public void setTotalWorkAmount(long workTotal)
                  {
                     monitor.beginTask(Messages.get().UploadFileToServer_TaskNamePrefix + localFile.getAbsolutePath(), (int)workTotal);
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
            return Messages.get().UploadFileToServer_JobError;
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

      if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().ViewServerFile_DeleteConfirmation,
            Messages.get().ViewServerFile_DeletAck))
         return;

      final Object[] objects = selection.toArray();
      new ConsoleJob(Messages.get().ViewServerFile_DeletFileFromServerJob, this, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ViewServerFile_ErrorDeleteFileJob;
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteServerFile(((ServerFile)objects[i]).getName());
            }
         }
      }.start();
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
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.FILE_LIST_CHANGED)
      {
         refreshFileList();
      }
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put("ServerFileManager.showFilter", initShowFilter);
      super.dispose();
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      initShowFilter = enable;
      filterText.setVisible(initShowFilter);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
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
}
