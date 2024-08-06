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
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ColumnViewerEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationStrategy;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.jface.viewers.TreeViewerEditor;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerDropAdapter;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSourceAdapter;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentFileData;
import org.netxms.client.AgentFileFingerprint;
import org.netxms.client.NXCException;
import org.netxms.client.ProgressListener;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.FileDownloadHelper;
import org.netxms.nxmc.modules.filemanager.dialogs.CreateFolderDialog;
import org.netxms.nxmc.modules.filemanager.dialogs.FileFingerprintDialog;
import org.netxms.nxmc.modules.filemanager.dialogs.StartClientToServerFileUploadDialog;
import org.netxms.nxmc.modules.filemanager.views.helpers.AgentFileComparator;
import org.netxms.nxmc.modules.filemanager.views.helpers.AgentFileFilter;
import org.netxms.nxmc.modules.filemanager.views.helpers.AgentFileLabelProvider;
import org.netxms.nxmc.modules.filemanager.views.helpers.NestedVerifyOverwrite;
import org.netxms.nxmc.modules.filemanager.views.helpers.ViewAgentFilesProvider;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * File manager for agent files
 */
public class AgentFileManager extends ObjectView
{
   public static final String ID = "org.netxms.ui.eclipse.filemanager.views.AgentFileManager"; 
   
   private static final String TABLE_CONFIG_PREFIX = "AgentFileManager"; 

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_SIZE = 2;
   public static final int COLUMN_MODIFYED = 3;
   public static final int COLUMN_OWNER = 4;
   public static final int COLUMN_GROUP = 5;
   public static final int COLUMN_ACCESS_RIGHTS = 6;

   private final I18n i18n = LocalizationHelper.getI18n(AgentFileManager.class);

   private AgentFileFilter filter;
   private SortableTreeViewer viewer;
   private Action actionUploadFile;
   private Action actionUploadFolder;
   private Action actionDelete;
   private Action actionRename;
   private Action actionRefreshDirectory;
   private Action actionDownloadFile;
   private Action actionTailFile;
   private Action actionFingerprintFile;
   private Action actionShowFile;
   private Action actionCreateDirectory;
   private Action actionCalculateFolderSize;
   private Action actionCopyFilePath;
   private Action actionCopyFileName;

   /**
    * Constructor
    */
   public AgentFileManager()
   {
      super(LocalizationHelper.getI18n(AgentFileManager.class).tr("File Manager"), ResourceManager.getImageDescriptor("icons/object-views/filemgr.png"), "AgentFileManager", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if ((context != null) && (context instanceof Node))
      {
         return (((Node)context).getCapabilities() & Node.NC_HAS_FILE_MANAGER) != 0;
      }
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 44;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {      
      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Size"), i18n.tr("Date modified"), i18n.tr("Owner"), i18n.tr("Group"), i18n.tr("Access Rights") };
      final int[] columnWidths = { 300, 120, 150, 150, 150, 150, 200 };         
      viewer = new SortableTreeViewer(parent, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);

      WidgetHelper.restoreTreeViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ViewAgentFilesProvider());
      viewer.setLabelProvider(new AgentFileLabelProvider());
      viewer.setComparator(new AgentFileComparator());
      filter = new AgentFileFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionDelete.setEnabled(selection.size() > 0);
               actionCalculateFolderSize.setEnabled(selection.size() > 0);
            }
         }
      });
      viewer.getTree().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTreeViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });
      
      enableDragSupport();
      enableDropSupport();
      enableInPlaceRename();
      
      createActions();
      createPopupMenu();
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
      viewer.addDropSupport(DND.DROP_COPY | DND.DROP_MOVE, transfers, new AgentFileDropAdapter(viewer));
   }
   
   /**
    * Enable in-place renames
    */
   private void enableInPlaceRename()
   {
      TreeViewerEditor.create(viewer, new ColumnViewerEditorActivationStrategy(viewer) {
         @Override
         protected boolean isEditorActivationEvent(ColumnViewerEditorActivationEvent event)
         {
            return event.eventType == ColumnViewerEditorActivationEvent.PROGRAMMATIC;
         }
      }, ColumnViewerEditor.DEFAULT);
      viewer.setCellEditors(new CellEditor[] { new TextCellEditor(viewer.getTree()) });
      viewer.setColumnProperties(new String[] { "name" }); 
      viewer.setCellModifier(new ICellModifier() {
         @Override
         public void modify(Object element, String property, Object value)
         {
            if (element instanceof Item)
               element = ((Item)element).getData();

            if (property.equals("name")) 
            {
               if (element instanceof AgentFile)
               {
                  doRename((AgentFile)element, value.toString());
               }
            }
         }

         @Override
         public Object getValue(Object element, String property)
         {
            if (property.equals("name")) 
            {
               if (element instanceof AgentFile)
               {
                  return ((AgentFile)element).getName();
               }
            }
            return null;
         }

         @Override
         public boolean canModify(Object element, String property)
         {
            return property.equals("name"); 
         }
      });
   }
   
   /**
    * Do actual rename
    * 
    * @param AgentFile
    * @param newName
    */
   private void doRename(final AgentFile agentFile, final String newName)
   {
      new Job("Rename file", this) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot rename file");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final NestedVerifyOverwrite verify =  new NestedVerifyOverwrite(getWindow().getShell(), agentFile.getType(), newName, true, true, false) {
               @Override
               public void executeAction() throws NXCException, IOException
               {
                  session.renameAgentFile(getObjectId(), agentFile.getFullName(), agentFile.getParent().getFullName() + "/" + newName, false); 
               }
               
               @Override
               public void executeSameFunctionWithOverwrite() throws IOException, NXCException
               {
                  session.renameAgentFile(getObjectId(), agentFile.getFullName(), agentFile.getParent().getFullName() + "/" + newName, true); 
               }
            };
            verify.run(viewer.getControl().getDisplay());
    
            if(verify.isOkPressed())
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if(verify.isOkPressed())
                        refreshFileOrDirectory();
                     agentFile.setName(newName);
                     viewer.refresh(agentFile, true);
                  }
               });
            }
         }
      }.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefreshDirectory = new Action(i18n.tr("Refresh this folder"), SharedIcons.REFRESH) {
         @Override
         public void run()
         {
            refreshFileOrDirectory();
         }
      };
      addKeyBinding("Ctrl+F5", actionRefreshDirectory);

      actionUploadFile = new Action(i18n.tr("&Upload file...")) {
         @Override
         public void run()
         {
            uploadFile(false);
         }
      };
      addKeyBinding("Ctrl+U", actionUploadFile);
      
      actionUploadFolder = new Action(i18n.tr("Upload &folder...")) {
         @Override
         public void run()
         {
            uploadFolder();
         }
      };

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteFile();
         }
      };

      actionRename = new Action(i18n.tr("&Rename")) {
         @Override
         public void run()
         {
            renameFile();
         }
      };
      addKeyBinding("Ctrl+R", actionRename);

      actionDownloadFile = new Action(i18n.tr("&Download...")) {
         @Override
         public void run()
         {
            startDownload();
         }
      };
      addKeyBinding("Ctrl+D", actionDownloadFile);

      actionTailFile = new Action(i18n.tr("&Follow changes")) {
         @Override
         public void run()
         {
            showFile(true, 8192);
         }
      };
      
      actionShowFile = new Action(i18n.tr("&Show")) {
         @Override
         public void run()
         {
            showFile(false, 0);
         }
      };

      actionFingerprintFile = new Action(i18n.tr("&Get fingerprint"))
      {
         @Override
         public void run()
         {
            getFileFingerprint();
         }
      };
      addKeyBinding("Ctrl+P", actionFingerprintFile);

      actionCreateDirectory = new Action(i18n.tr("&Create folder...")) {
         @Override
         public void run()
         {
            createFolder();
         }
      };
      addKeyBinding("Ctrl+N", actionCreateDirectory);
      
      actionCalculateFolderSize = new Action("Calculate folder &size") {
         @Override
         public void run()
         {
            calculateFolderSize();
         }
      };
      addKeyBinding("Ctrl+S", actionCalculateFolderSize);
      
      actionCopyFileName = new Action("Copy file &name") {
         @Override
         public void run()
         {
            copyFileName();
         }
      };
      addKeyBinding("Ctrl+C", actionCopyFileName);
      
      actionCopyFilePath = new Action("Copy file &path") {
         @Override
         public void run()
         {
            copyFilePath();
         }
      };
      addKeyBinding("Ctrl+shift+C", actionCopyFilePath);
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
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      if (selection.size() == 1)
      {
         if (((AgentFile)selection.getFirstElement()).isDirectory())
         {
            mgr.add(actionUploadFile);
            if (!Registry.IS_WEB_CLIENT)
               mgr.add(actionUploadFolder);
         }
         else
         {
            mgr.add(actionTailFile);
            mgr.add(actionFingerprintFile);
            mgr.add(actionShowFile);
         }
      }

      mgr.add(actionDownloadFile);
      
      if (isFolderOnlySelection(selection))
         mgr.add(actionCalculateFolderSize);
      mgr.add(new Separator());
      
      if (selection.size() == 1)
      {
         if (((AgentFile)selection.getFirstElement()).isDirectory())
         {
            mgr.add(actionCreateDirectory);
         }
         mgr.add(actionRename);
      }
      mgr.add(actionDelete);
      mgr.add(new Separator());
      if (selection.size() == 1)
      {
         mgr.add(actionCopyFileName);
         mgr.add(actionCopyFilePath);
      }
      mgr.add(new Separator());
      if ((selection.size() == 1) && ((AgentFile)selection.getFirstElement()).isDirectory())
      {
         mgr.add(new Separator());
         mgr.add(actionRefreshDirectory);
      }
   }
   
   /**
    * Check if given selection contains only folders
    * 
    * @param selection
    * @return
    */
   private boolean isFolderOnlySelection(IStructuredSelection selection)
   {
      for(Object o : selection.toList())
         if (!((AgentFile)o).isDirectory())
            return false;
      return true;
   }

   /**
    * Refresh file list
    */
   @Override
   public void refresh()
   {
      viewer.setInput(new ArrayList<AgentFile>(0));
      new Job(i18n.tr("Get server file list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AgentFile> files = session.listAgentFiles(null, "/", getObjectId()); 
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
            return i18n.tr("Cannot get file store content");
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

      new Job(i18n.tr("Reading remote directory"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               if (!((AgentFile)objects[i]).isDirectory())
                  objects[i] = ((AgentFile)objects[i]).getParent();

               final AgentFile sf = ((AgentFile)objects[i]);
               sf.setChildren(session.listAgentFiles(sf, sf.getFullName(), getObjectId()));

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
            return i18n.tr("Cannot read remote directory");
         }
      }.start();
   }
   
   /**
    * Upload local file to agent
    */
   private void uploadFile(final boolean overvrite)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();
      final AgentFile uploadFolder = ((AgentFile)objects[0]).isDirectory() ? ((AgentFile)objects[0]) : ((AgentFile)objects[0]).getParent();

      final StartClientToServerFileUploadDialog dlg = new StartClientToServerFileUploadDialog(getWindow().getShell());
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Upload file to agent"), this) {
            @Override
            protected void run(final IProgressMonitor monitor) throws Exception
            {
               List<File> fileList = dlg.getLocalFiles();
               for(int i = 0; i < fileList.size(); i++)
               {
                  final File localFile = fileList.get(i);
                  String remoteFile = fileList.get(i).getName();
                  if (fileList.size() == 1)
                     remoteFile = dlg.getRemoteFileName();
                  final String rFileName = remoteFile;

                  new NestedVerifyOverwrite(getWindow().getShell(), localFile.isDirectory() ? AgentFile.DIRECTORY : AgentFile.FILE, localFile.getName(), true, true, false) {
                     @Override
                     public void executeAction() throws NXCException, IOException
                     {                        
                        session.uploadLocalFileToAgent(getObjectId(), localFile, uploadFolder.getFullName() + "/" + rFileName, overvrite, new ProgressListener() { 
                           private long unitSize;
                           private int progress = 0;

                           @Override
                           public void setTotalWorkAmount(long workTotal)
                           {
                              unitSize = workTotal / 1000;
                              monitor.beginTask(i18n.tr("Upload file ") + localFile.getAbsolutePath(), 1000);
                           }

                           @Override
                           public void markProgress(long workDone)
                           {
                              int fullUnitsCompleted = (int)(workDone / unitSize);
                              if (fullUnitsCompleted > progress)
                              {
                                 monitor.worked(fullUnitsCompleted - progress);
                                 progress = fullUnitsCompleted;
                              }
                           }
                        });
                        monitor.done(); 
                     }

                     @Override
                     public void executeSameFunctionWithOverwrite() throws IOException, NXCException
                     {
                        session.uploadLocalFileToAgent(getObjectId(), localFile, uploadFolder.getFullName() + "/" + rFileName, true, new ProgressListener() { 
                           private long prevWorkDone = 0;

                           @Override
                           public void setTotalWorkAmount(long workTotal)
                           {
                              monitor.beginTask(i18n.tr("Upload file ") + localFile.getAbsolutePath(), (int)workTotal);
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
                  }.run(viewer.getControl().getDisplay());
               }

               uploadFolder.setChildren(session.listAgentFiles(uploadFolder, uploadFolder.getFullName(), getObjectId()));
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.refresh(uploadFolder, true);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot upload file to remote agent";
            }
         }.start();
      }
   }
   
   /**
    * Upload local folder to agent
    */
   private void uploadFolder()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      FileDownloadHelper.uploadFolder(getObjectId(), selection, this, viewer);
   }   

   /**
    * Delete selected file
    */
   private void deleteFile()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Delete confirmation"),
            i18n.tr("Are you sure you want to delete this file?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Delete file from server"), this) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error while deleting file.");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               final AgentFile sf = (AgentFile)objects[i];
               session.deleteAgentFile(getObjectId(), sf.getFullName());

               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     sf.getParent().removeChild(sf);
                     viewer.refresh(sf.getParent());
                  }
               });
            }
         }
      }.start();
   }

   /**
    * Show file and optionally follow changes
    */
   private void showFile(final boolean followChanges, final int offset)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentFile agentFile = (AgentFile)selection.getFirstElement();
      if (agentFile.isDirectory())
         return;

      final long maxFileSize = Math.min(offset, agentFile.getSize());
      new Job(followChanges ? i18n.tr("Download file from agent and start following changes") : i18n.tr("Download file from agent"), this) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            final AgentFileData file = session.downloadFileFromAgent(getObjectId(), agentFile.getFullName(), maxFileSize, followChanges, new ProgressListener() {
               @Override
               public void setTotalWorkAmount(long workTotal)
               {
                  monitor.beginTask(String.format(i18n.tr("Download file %s"), agentFile.getFullName()), (int)workTotal);
               }

               @Override
               public void markProgress(long workDone)
               {
                  monitor.worked((int)workDone);
               }
            });
            runInUIThread(() -> AgentFileViewer.createView(new ViewPlacement(AgentFileManager.this), getObjectId(), file, maxFileSize, followChanges, getObjectId()));
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Error downloading file %s from node %s"), agentFile.getFullName(), getObjectName());
         }
      }.start();
   }

   /**
    * Acquire file fingerprint with its size, hash and hex dump
    */
   private void getFileFingerprint()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentFile file = (AgentFile)selection.getFirstElement();
      new Job(i18n.tr("Getting file fingerprint"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final AgentFileFingerprint fingerprint = session.getAgentFileFingerprint(getObjectId(), file.getFullName());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  FileFingerprintDialog dlg = new FileFingerprintDialog(getWindow().getShell(), fingerprint);
                  dlg.open();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to get file fingerprint");
         }
      }.start();
   }

   /**
    * Download file from agent
    */
   private void startDownload()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      FileDownloadHelper.startDownload(getObjectId(), selection, this);
   }

   /**
    * Rename selected file
    */
   private void renameFile()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1)
         viewer.editElement(selection.getFirstElement(), 0);
   }

   /**
    * Move selected file
    */
   private void moveFile(final AgentFile target, final AgentFile object)
   {
      new Job(i18n.tr("Moving file"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            NestedVerifyOverwrite verify = new NestedVerifyOverwrite(getWindow().getShell(), object.getType(), object.getName(), true, true, false) {
               @Override
               public void executeAction() throws NXCException, IOException
               {
                  session.renameAgentFile(getObjectId(), object.getFullName(), target.getFullName() + "/" + object.getName(), false); 
               }

               @Override
               public void executeSameFunctionWithOverwrite() throws IOException, NXCException
               {
                  session.renameAgentFile(getObjectId(), object.getFullName(), target.getFullName() + "/" + object.getName(), true);                   
               }
            };
            verify.run(viewer.getControl().getDisplay());
            
            if (verify.isOkPressed())
            {
               target.setChildren(session.listAgentFiles(target, target.getFullName(), getObjectId()));
               object.getParent().setChildren(session.listAgentFiles(object.getParent(), object.getParent().getFullName(), getObjectId()));

               runInUIThread(() -> {
                  viewer.refresh(object.getParent(), true);
                  object.setParent(target);
                  viewer.refresh(object.getParent(), true);
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot move file");
         }
      }.start();
   }

   /**
    * Copy agent file
    * 
    * @param target where the file will be moved
    * @param object file being moved
    */
   private void copyFile(final AgentFile target, final AgentFile object)
   {
      new Job("Copying file", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            NestedVerifyOverwrite verify = new NestedVerifyOverwrite(getWindow().getShell(), object.getType(), object.getName(), true, true, false) {
               @Override
               public void executeAction() throws NXCException, IOException
               {
                  session.copyAgentFile(getObjectId(), object.getFullName(), target.getFullName() + "/" + object.getName(), false); 
               }

               @Override
               public void executeSameFunctionWithOverwrite() throws IOException, NXCException
               {
                  session.copyAgentFile(getObjectId(), object.getFullName(), target.getFullName() + "/" + object.getName(), true); 
               }
            };
            verify.run(viewer.getControl().getDisplay());

            if (verify.isOkPressed())
            {
               target.setChildren(session.listAgentFiles(target, target.getFullName(), getObjectId()));
               object.getParent().setChildren(session.listAgentFiles(object.getParent(), object.getParent().getFullName(), getObjectId()));

               runInUIThread(() -> {
                  viewer.refresh(object.getParent(), true);
                  object.setParent(target);
                  viewer.refresh(target, true);
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot copy file";
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
      final AgentFile parentFolder = ((AgentFile)objects[0]).isDirectory() ? ((AgentFile)objects[0]) : ((AgentFile)objects[0])
            .getParent();

      final CreateFolderDialog dlg = new CreateFolderDialog(getWindow().getShell());
      if (dlg.open() != Window.OK)
         return;
      
      final String newFolder = dlg.getNewName();
      
      new Job(i18n.tr("Creating folder"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         { 
            NestedVerifyOverwrite verify = new NestedVerifyOverwrite(getWindow().getShell(), AgentFile.DIRECTORY, newFolder, true, true, false) {
               @Override
               public void executeAction() throws NXCException, IOException
               {
                  session.createFolderOnAgent(getObjectId(), parentFolder.getFullName() + "/" + newFolder); 
               }
   
               @Override
               public void executeSameFunctionWithOverwrite() throws IOException, NXCException
               {
                  // do nothing
               }
            };
            verify.run(viewer.getControl().getDisplay());
            parentFolder.setChildren(session.listAgentFiles(parentFolder, parentFolder.getFullName(), getObjectId()));

            runInUIThread(() -> viewer.refresh(parentFolder, true));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create folder");
         }
      }.start();
   }
   
   /**
    * Show file size
    */
   private void calculateFolderSize()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      final List<AgentFile> files = new ArrayList<AgentFile>(selection.size());
      for(Object o : selection.toList())
         files.add((AgentFile)o);
      
      new Job("Calculate folder size", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AgentFile f : files)
            {
               f.setFileInfo(session.getAgentFileInfo(f));
            }
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.update(files.toArray(), null);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot calculate folder size";
         }
      }.start();
   }
   
   /**
    * Copy name of file to clipboard
    */
   private void copyFileName()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() == 1)
         WidgetHelper.copyToClipboard(((AgentFile)selection.getFirstElement()).getName());
   }

   /**
    * Copy full path to file to clipboard
    */
   private void copyFilePath()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() == 1)
         WidgetHelper.copyToClipboard(((AgentFile)selection.getFirstElement()).getFilePath());
   }

   /**
    * Custom implementation of ViewerDropAdapter for Agent File Manager
    */
   public class AgentFileDropAdapter extends ViewerDropAdapter
   {
      int operation = 0;

      /**
       * Agent file drop adapter constructor
       * 
       * @param viewer
       * @param mode Copy or Move
       */
      protected AgentFileDropAdapter(Viewer viewer)
      {
         super(viewer);
      }

      /**
       * @see org.eclipse.jface.viewers.ViewerDropAdapter#performDrop(java.lang.Object)
       */
      @Override
      public boolean performDrop(Object data)
      {
         IStructuredSelection selection = (IStructuredSelection)data;
         List<?> movableSelection = selection.toList();
         for(int i = 0; i < movableSelection.size(); i++)
         {
            AgentFile movableObject = (AgentFile)movableSelection.get(i);
            
            if (operation == DND.DROP_COPY)
               copyFile((AgentFile)getCurrentTarget(), movableObject);
            else
               moveFile((AgentFile)getCurrentTarget(), movableObject);

         }
         return true;
      }

      /**
       * @see org.eclipse.jface.viewers.ViewerDropAdapter#validateDrop(java.lang.Object, int, org.eclipse.swt.dnd.TransferData)
       */
      @Override
      public boolean validateDrop(Object target, int operation, TransferData transferType)
      {
         this.operation = operation;
         if ((target == null) || !LocalSelectionTransfer.getTransfer().isSupportedType(transferType))
            return false;

         IStructuredSelection selection = (IStructuredSelection)LocalSelectionTransfer.getTransfer().getSelection();
         if (selection.isEmpty())
            return false;

         for(final Object object : selection.toList())
         {
            if (!(object instanceof AgentFile))
               return false;
         }
         if (!(target instanceof AgentFile))
         {
            return false;
         }
         else
         {
            if (!((AgentFile)target).isDirectory())
            {
               return false;
            }
         }
         return true;
      }      
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if(object == null)
         return;
      
      refresh();
      String os = ((Node)session.findObjectById(getObjectId())).getSystemDescription(); //$NON-NLS-1$
      if (os.contains("Windows")) 
      {
         viewer.removeColumnById(COLUMN_GROUP);
         viewer.removeColumnById(COLUMN_ACCESS_RIGHTS);
      }
      else
      {
         viewer.addColumn(i18n.tr("Group"), 150);
         viewer.addColumn(i18n.tr("Access Rights"), 200);         
      }
   }
}
