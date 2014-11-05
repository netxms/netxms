/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URLEncoder;
import java.util.Date;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.ViewerDropAdapter;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSourceAdapter;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.ProgressListener;
import org.netxms.client.AgentFile;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerFile;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.filemanager.Activator;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.dialogs.CreateFolderDialog;
import org.netxms.ui.eclipse.filemanager.dialogs.RenameFileDialog;
import org.netxms.ui.eclipse.filemanager.dialogs.StartClientToServerFileUploadDialog;
import org.netxms.ui.eclipse.filemanager.views.helpers.AgentFileComparator;
import org.netxms.ui.eclipse.filemanager.views.helpers.AgentFileFilter;
import org.netxms.ui.eclipse.filemanager.views.helpers.ServerFileLabelProvider;
import org.netxms.ui.eclipse.filemanager.views.helpers.ViewAgentFilesProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.views.FileViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Editor for server files
 */
public class AgentFileManager extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.filemanager.views.AgentFileManager"; //$NON-NLS-1$

   private static final String TABLE_CONFIG_PREFIX = "AgentFileManager"; //$NON-NLS-1$

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_SIZE = 2;
   public static final int COLUMN_MODIFYED = 3;

   private boolean filterEnabled = false;
   private Composite content;
   private ServerFile[] files;
   private AgentFileFilter filter;
   private FilterText filterText;
   private SortableTreeViewer viewer;
   private NXCSession session;
   private Action actionRefreshAll;
   private Action actionUploadFile;
   private Action actionUploadFolder;
   private Action actionDelete;
   private Action actionRename;
   private Action actionRefreshDirectory;
   private Action actionShowFilter;
   private Action actionDownloadFile;
   private Action actionTailFile;
   private Action actionCreateDirectory;
   private long objectId = 0;
   private String workspaceDir; 

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      session = (NXCSession)ConsoleSharedData.getSession();
      objectId = Long.parseLong(site.getSecondaryId());
      setPartName(String.format("File Manager - %s", session.getObjectName(objectId)));
      workspaceDir = Platform.getInstanceLocation().getURL().getPath();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
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
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });

      final String[] columnNames = { "Name", "Type", "Size", "Date modified" };
      final int[] columnWidths = { 300, 120, 150, 150 };
      viewer = new SortableTreeViewer(content, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      WidgetHelper.restoreTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ViewAgentFilesProvider());
      viewer.setLabelProvider(new ServerFileLabelProvider());
      viewer.setComparator(new AgentFileComparator());
      filter = new AgentFileFilter();
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
      viewer.getTree().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
         }
      });
      enableDragSupport();
      enableDropSupport();

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getTree().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createActions();
      contributeToActionBars();
      createPopupMenu();

      filterText.setCloseAction(actionShowFilter);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      refreshFileList();
   }

   /**
    * Enable drag support in object tree
    */
   public void enableDragSupport()
   {
      Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      viewer.addDragSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new DragSourceAdapter() {
         @Override
         public void dragStart(DragSourceEvent event)
         {
            LocalSelectionTransfer.getTransfer().setSelection(viewer.getSelection());
            event.doit = true;
         }

         @Override
         public void dragSetData(DragSourceEvent event)
         {
            event.data = LocalSelectionTransfer.getTransfer().getSelection();
         }
      });
   }

   /**
    * Enable drop support in object tree
    */
   public void enableDropSupport()// SubtreeType infrastructure
   {
      final Transfer[] transfers = new Transfer[] { LocalSelectionTransfer.getTransfer() };
      viewer.addDropSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new ViewerDropAdapter(viewer) {

         @Override
         public boolean performDrop(Object data)
         {
            IStructuredSelection selection = (IStructuredSelection)data;
            List<?> movableSelection = selection.toList();
            for(int i = 0; i < movableSelection.size(); i++)
            {
               ServerFile movableObject = (ServerFile)movableSelection.get(i);

               moveFile((ServerFile)getCurrentTarget(), movableObject);

            }
            return true;
         }

         @Override
         public boolean validateDrop(Object target, int operation, TransferData transferType)
         {
            if ((target == null) || !LocalSelectionTransfer.getTransfer().isSupportedType(transferType))
               return false;

            IStructuredSelection selection = (IStructuredSelection)LocalSelectionTransfer.getTransfer().getSelection();
            if (selection.isEmpty())
               return false;

            for(final Object object : selection.toList())
            {
               if (!(object instanceof ServerFile))
                  return false;
            }
            if (!(target instanceof ServerFile))
            {
               return false;
            }
            else
            {
               if (!((ServerFile)target).isDirectory())
               {
                  return false;
               }
            }
            return true;
         }
      });
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      actionRefreshDirectory = new Action("Refresh this folder", SharedIcons.REFRESH) {
         @Override
         public void run()
         {
            refreshFileOrDirectory();
         }
      };

      actionRefreshAll = new RefreshAction(this) {
         @Override
         public void run()
         {
            refreshFileList();
         }
      };

      actionUploadFile = new Action("&Upload file...") {
         @Override
         public void run()
         {
            uploadFile();
         }
      };
      
     /* actionUploadFolder = new Action("&Upload folder...") {
         @Override
         public void run()
         {
            uploadFolder();
         }
      }; */

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteFile();
         }
      };

      actionRename = new Action("&Rename") {
         @Override
         public void run()
         {
            renameFile();
         }
      };

      actionShowFilter = new Action(Messages.get().ViewServerFile_ShowFilterAction, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(!filterEnabled);
            actionShowFilter.setChecked(filterEnabled);
         }
      };
      actionShowFilter.setChecked(filterEnabled);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
      final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);

      actionDownloadFile = new Action("&Download...") {
         @Override
         public void run()
         {
            startDownload();
         }
      };

      actionTailFile = new Action("&Show") {
         @Override
         public void run()
         {
            tailFile();
         }
      };
      
      actionCreateDirectory = new Action("&Create folder") {
         @Override
         public void run()
         {
            createFolder();
         }
      };
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
      manager.add(actionRefreshAll);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local tool bar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionRefreshAll);
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

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      if (selection.size() == 1)
      {
         if (((ServerFile)selection.getFirstElement()).isDirectory())
         {
            mgr.add(actionUploadFile);
            //mgr.add(actionUploadFolder);
            mgr.add(actionDownloadFile);
         }
         else
         {
            mgr.add(actionTailFile);
         }
         mgr.add(new Separator());
      }
      
      mgr.add(actionRename);
      mgr.add(actionDelete);
      mgr.add(actionCreateDirectory);
      mgr.add(new Separator());
      mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      if ((selection.size() == 1) && ((ServerFile)selection.getFirstElement()).isDirectory())
      {
         mgr.add(new Separator());
         mgr.add(actionRefreshDirectory);
      }
   }

   /**
    * Refresh file list
    */
   private void refreshFileList()
   {
      new ConsoleJob(Messages.get().SelectServerFileDialog_JobTitle, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            files = session.listAgentFiles(null, "/", objectId);
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
    * Refresh file list
    */
   private void refreshFileOrDirectory()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();

      new ConsoleJob(Messages.get().SelectServerFileDialog_JobTitle, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               if (!((ServerFile)objects[i]).isDirectory())
                  objects[i] = ((ServerFile)objects[i]).getParent();

               final ServerFile sf = ((ServerFile)objects[i]);
               sf.setChildren(session.listAgentFiles(sf, sf.getFullName(), objectId));

               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.refresh(sf);
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().SelectServerFileDialog_JobError;
         }
      }.start();
   }
   
   /**
    * Upload local file to agent
    */
   private void uploadFile()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();
      final ServerFile upladFolder = ((ServerFile)objects[0]).isDirectory() ? ((ServerFile)objects[0]) : ((ServerFile)objects[0])
            .getParent();

      final StartClientToServerFileUploadDialog dlg = new StartClientToServerFileUploadDialog(getSite().getShell());
      if (dlg.open() == Window.OK)
      {
         final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
         new ConsoleJob("Upload file to agent", null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(final IProgressMonitor monitor) throws Exception
            {
               session.uploadLocalFileToAgent(dlg.getLocalFile(), upladFolder.getFullName()+"/"+dlg.getRemoteFileName(), objectId, new ProgressListener() {
                  private long prevWorkDone = 0;

                  @Override
                  public void setTotalWorkAmount(long workTotal)
                  {
                           monitor.beginTask(Messages.get(getDisplay()).UploadFileToServer_TaskNamePrefix
                                 + dlg.getLocalFile().getAbsolutePath(),
                           (int)workTotal);
                  }

                  @Override
                  public void markProgress(long workDone)
                  {
                     monitor.worked((int)(workDone - prevWorkDone));
                     prevWorkDone = workDone;
                  }
               });
               monitor.done(); 
               
               upladFolder.setChildren(session.listAgentFiles(upladFolder, upladFolder.getFullName(), objectId));
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {

                     viewer.refresh(upladFolder, true);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().UploadFileToServer_JobError;
            }
         }.start();
      }
   }
   
   /**
    * Upload local folder to agent
    */
   /*
   private void uploadFolder()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();
      final ServerFile upladFolder = ((ServerFile)objects[0]).isDirectory() ? ((ServerFile)objects[0]) : ((ServerFile)objects[0])
            .getParent();

      final StartClientToAgentFolderUploadDialog dlg = new StartClientToAgentFolderUploadDialog(getSite().getShell());
      if (dlg.open() == Window.OK)
      {
         final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
         new ConsoleJob("Upload file to agent", null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(final IProgressMonitor monitor) throws Exception
            {
               File folder = dlg.getLocalFile();
               session.createFolderOnAgent(upladFolder.getFullName()+"/"+dlg.getRemoteFileName(), objectId);
               listFilesForFolder(folder, upladFolder.getFullName()+"/"+dlg.getRemoteFileName(), monitor);
               
               upladFolder.setChildren(session.listAgentFiles(upladFolder, upladFolder.getFullName(), objectId));
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.refresh(upladFolder, true);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().UploadFileToServer_JobError;
            }
         }.start();
      }
   }   
    */
   
   /**
    * Recursively uploads files to agent and creates correct folders
    * 
    * @param folder
    * @param upladFolder
    * @param monitor
    * @throws NXCException
    * @throws IOException
    */
   public void listFilesForFolder(final File folder, final String upladFolder, final IProgressMonitor monitor) throws NXCException, IOException 
   {
      for (final File fileEntry : folder.listFiles()) 
      {
          if (fileEntry.isDirectory()) 
          {
             session.createFolderOnAgent(upladFolder+"/"+fileEntry.getName(), objectId);
             listFilesForFolder(fileEntry, upladFolder+"/"+fileEntry.getName(), monitor);
          } 
          else 
          {
             session.uploadLocalFileToAgent(fileEntry, upladFolder+"/"+fileEntry.getName(), objectId, new ProgressListener() {
                private long prevWorkDone = 0;

                @Override
                public void setTotalWorkAmount(long workTotal)
                {
                   monitor.beginTask(Messages.get().UploadFileToServer_TaskNamePrefix + fileEntry.getAbsolutePath(),
                         (int)workTotal);
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
  }

   /**
    * Delete selected file
    */
   private void deleteFile()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().ViewServerFile_DeleteConfirmation,
            Messages.get().ViewServerFile_DeletAck))
         return;

      final Object[] objects = selection.toArray();
      new ConsoleJob(Messages.get().ViewServerFile_DeletFileFromServerJob, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
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
               final ServerFile sf = (ServerFile)objects[i];
               session.deleteAgentFile(objectId, sf.getFullName());

               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     sf.getParent().removeChield(sf);
                     viewer.refresh(sf.getParent());
                  }
               });
            }
         }
      }.start();
   }

   /**
    * Starts file tail view&messages
    */
   private void tailFile()
   {      
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();

      if (((ServerFile)objects[0]).isDirectory())
         return;

      final ServerFile sf = ((ServerFile)objects[0]);

      ConsoleJob job = new ConsoleJob("Download file from agent and start tail", null, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return String.format("Error while downloading %s file from %d node.", sf.getFullName(), objectId);
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final AgentFile file = session.downloadFileFromAgent(objectId, sf.getFullName(), 0, true);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
                  try
                  {
                     String secondaryId = Long.toString(objectId) + "&" + URLEncoder.encode(sf.getName(), "UTF-8"); //$NON-NLS-1$ //$NON-NLS-2$
                     FileViewer view = (FileViewer)window.getActivePage().showView(FileViewer.ID, secondaryId,
                           IWorkbenchPage.VIEW_ACTIVATE);
                     view.showFile(file.getFile(), true, file.getId(), 0);
                  }
                  catch(Exception e)
                  {
                     MessageDialogHelper.openError(window.getShell(), "Error opening view.",
                           String.format("Error opening view: %s", e.getLocalizedMessage()));
                  }
               }
            });
         }
      };
      job.start();
   }

   /**
    * Download file from agent
    */
   private void startDownload()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      final ServerFile sf = (ServerFile)selection.getFirstElement();
      
      if (!sf.isDirectory())
         downloadFile(sf.getFullName());

      if (sf.isDirectory())
      {
         ConsoleJob job = new ConsoleJob(Messages.get().SelectServerFileDialog_JobTitle, null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {				
				//create zip from download folder wile download
				FileOutputStream fos = new FileOutputStream(workspaceDir+File.separator+sf.getName()+".zip");
				ZipOutputStream zos = new ZipOutputStream(fos);
				downloadDir(sf, sf.getName(), zos);
				
				zos.close();
				fos.close();
				
				//javascript for client to download this zip
				final File zipArchive = new File(workspaceDir+File.separator+sf.getName()+".zip");
				DownloadServiceHandler.addDownload(zipArchive.getName(), zipArchive.getName(), zipArchive, "application/octet-stream");
	            runInUIThread(new Runnable() {
	               @Override
	               public void run()
	               {
	                  JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
	                  if( executor != null ) 
	                  {
	                     StringBuilder js = new StringBuilder();
	                     js.append("var hiddenIFrameID = 'hiddenDownloader',");
	                     js.append("   iframe = document.getElementById(hiddenIFrameID);");
	                     js.append("if (iframe === null) {");
	                     js.append("   iframe = document.createElement('iframe');");
	                     js.append("   iframe.id = hiddenIFrameID;");
	                     js.append("   iframe.style.display = 'none';");
	                     js.append("   document.body.appendChild(iframe);");
	                     js.append("}");
	                     js.append("iframe.src = '");
	                     js.append(DownloadServiceHandler.createDownloadUrl(zipArchive.getName()));
	                     js.append("';");
	                     executor.execute(js.toString());
	                  }                 
	               }
	            });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Error while retrieving children of server file";
            }
         };
         job.setUser(false);
         job.start();
      }
   }
   
   public static void addToZipFile(String fileName, ZipOutputStream zos) throws FileNotFoundException, IOException 
   {
		File file = new File(fileName);
		FileInputStream fis = new FileInputStream(file);
		ZipEntry zipEntry = new ZipEntry(fileName+File.separator);
		zos.putNextEntry(zipEntry);

		byte[] bytes = new byte[1024];
		int length;
		while ((length = fis.read(bytes)) >= 0) 
		{
			zos.write(bytes, 0, length);
		}

		zos.closeEntry();
		fis.close();
	}
   
   /**
    * Recursively download directory from agent to local pc
    * 
    * @param sf
    * @param localFileName
    * @throws IOException 
    * @throws NXCException 
    */
   private void downloadDir(final ServerFile sf, String localFileName,ZipOutputStream zos) throws NXCException, IOException
   {
	   //create directory inside of zip
      zos.putNextEntry(new ZipEntry(localFileName+File.separator));
      zos.closeEntry();
      ServerFile[] files = sf.getChildren();
      if(files == null)
      {
         files = session.listAgentFiles(sf, sf.getFullName(), sf.getNodeId());
      }
      for(int i = 0; i < files.length; i++)
      {
         if(files[i].isDirectory())
         {
            downloadDir(files[i], localFileName+"/"+sf.getName(),zos);
         }
         else
         {
            final AgentFile file = session.downloadFileFromAgent(objectId,  files[i].getFullName(), 0, false);
            
    		FileInputStream fis = new FileInputStream(file.getFile());
    		ZipEntry zipEntry = new ZipEntry(localFileName+"/"+files[i].getName());
    		zos.putNextEntry(zipEntry);

    		byte[] bytes = new byte[1024];
    		int length;
    		while ((length = fis.read(bytes)) >= 0) 
    		{
    			zos.write(bytes, 0, length);
    		}

    		zos.closeEntry();
    		fis.close();
         }
      }
   }
   
   /**
    * Downloads file
    */
   private void downloadFile(final String remoteName)
   {
      ConsoleJob job = new ConsoleJob("Download file from agent", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final AgentFile file = session.downloadFileFromAgent(objectId, remoteName, 0, false);
            DownloadServiceHandler.addDownload(file.getFile().getName(), remoteName, file.getFile(), "application/octet-stream");
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
                  if( executor != null ) 
                  {
                     StringBuilder js = new StringBuilder();
                     js.append("var hiddenIFrameID = 'hiddenDownloader',");
                     js.append("   iframe = document.getElementById(hiddenIFrameID);");
                     js.append("if (iframe === null) {");
                     js.append("   iframe = document.createElement('iframe');");
                     js.append("   iframe.id = hiddenIFrameID;");
                     js.append("   iframe.style.display = 'none';");
                     js.append("   document.body.appendChild(iframe);");
                     js.append("}");
                     js.append("iframe.src = '");
                     js.append(DownloadServiceHandler.createDownloadUrl(file.getFile().getName()));
                     js.append("';");
                     executor.execute(js.toString());
                  }                 
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Error while downloading file %s from node %s [%d]", remoteName, session.getObjectName(objectId), objectId);
         }
      };
      job.start();
   }
   
   /**
    * Rename selected file
    */
   private void renameFile()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();

      RenameFileDialog dlg = new RenameFileDialog(getSite().getShell(), ((ServerFile)objects[0]).getName());
      if (dlg.open() != Window.OK)
         return;

      final String newName = dlg.getNewName();

      new ConsoleJob(Messages.get().ViewServerFile_DeletFileFromServerJob, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return "Error while rename file job";
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final ServerFile sf = (ServerFile)objects[0];
            session.renameAgentFile(objectId, sf.getFullName(), sf.getParent().getFullName() + "/" + newName);

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  sf.setName(newName);
                  viewer.refresh(sf, true);
               }
            });
         }
      }.start();
   }

   /**
    * Move selected file
    */
   private void moveFile(final ServerFile target, final ServerFile object)
   {
      new ConsoleJob(Messages.get().ViewServerFile_DeletFileFromServerJob, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return "Error while move file job";
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.renameAgentFile(objectId, object.getFullName(), target.getFullName() + "/" + object.getName());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  object.getParent().removeChield(object);
                  viewer.refresh(object.getParent(), true);
                  object.setParent(target);
                  target.addChield(object);
                  viewer.refresh(object.getParent(), true);
               }
            });
         }
      }.start();
   }
   
   /**
    * Create new folder
    */
   private void createFolder()
   {      
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();
      final ServerFile parentFolder = ((ServerFile)objects[0]).isDirectory() ? ((ServerFile)objects[0]) : ((ServerFile)objects[0])
            .getParent();

      final CreateFolderDialog dlg = new CreateFolderDialog(getSite().getShell());
      if (dlg.open() != Window.OK)
         return;
      
      final String newFolder = dlg.getNewName();
      
      new ConsoleJob("Create folder job", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return "Error while create folder job";
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.createFolderOnAgent(parentFolder.getFullName()+"/"+newFolder, objectId);
            parentFolder.setChildren(session.listAgentFiles(parentFolder, parentFolder.getFullName(), objectId));

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.refresh(parentFolder, true);
               }
            });
         }
      }.start();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterText.setVisible(filterEnabled);
      FormData fd = (FormData)viewer.getTree().getLayoutData();
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
