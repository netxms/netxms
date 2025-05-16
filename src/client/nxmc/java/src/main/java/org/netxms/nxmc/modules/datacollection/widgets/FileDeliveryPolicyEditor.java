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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.FilePermissionDialog;
import org.netxms.nxmc.modules.datacollection.views.PolicyEditorView;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.FileDeliveryPolicy;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.FileDeliveryPolicyComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.FileDeliveryPolicyContentProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.FileDeliveryPolicyLabelProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.PathElement;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
/**
 * Editor for file delivery policy
 */
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

public class FileDeliveryPolicyEditor extends AbstractPolicyEditor
{
   private static final Logger logger = LoggerFactory.getLogger(FileDeliveryPolicyEditor.class);

   private final I18n i18n = LocalizationHelper.getI18n(FileDeliveryPolicyEditor.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_GUID = 1;
   public static final int COLUMN_DATE = 2;
   public static final int COLUMN_USER = 3;
   public static final int COLUMN_GROUP = 4;
   public static final int COLUMN_PERMISSIONS = 5;

   private SortableTreeViewer fileTree;
   private Set<PathElement> rootElements = new HashSet<PathElement>();
   private Action actionAddRoot;
   private Action actionAddDirectory;
   private Action actionAddFile;
   private Action actionDelete;
   private Action actionRename;
   private Action actionUpdate;
   private Action actionEditPermissions;
   private Set<String> filesForDeletion = new HashSet<String>();
   private Set<String> notSavedFiles = new HashSet<String>();
   private boolean fileTransferInProgress = false;

   /**
    * @param parent
    * @param style
    */
   public FileDeliveryPolicyEditor(Composite parent, int style, AgentPolicy policy, PolicyEditorView view)
   {
      super(parent, style, policy, view);

      setLayout(new FillLayout());

      final String[] columnNames = { i18n.tr("Name"), i18n.tr("GUID"), i18n.tr("Date"), i18n.tr("User"), i18n.tr("Group"), i18n.tr("Permissions") };
      final int[] columnWidths = { 300, 300, 200, 150, 150, 200 };
      fileTree = new SortableTreeViewer(this, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      fileTree.setContentProvider(new FileDeliveryPolicyContentProvider());
      fileTree.setLabelProvider(new FileDeliveryPolicyLabelProvider());
      fileTree.setComparator(new FileDeliveryPolicyComparator());
      fileTree.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            IStructuredSelection selection = fileTree.getStructuredSelection();
            if (selection.size() == 1)
            {
               PathElement element = (PathElement)selection.getFirstElement();
               if (element.isFile())
                  changePermissions();
               else
                  fileTree.expandToLevel(element, 1);
            }
         }
      });

      createActions();
      createPopupMenu();

      fileTree.setInput(rootElements);
      updateControlFromPolicy();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAddRoot = new Action(i18n.tr("&Add root directory..."), ResourceManager.getImageDescriptor("icons/add_directory.png")) {
         @Override
         public void run()
         {
            addElement(null);
         }
      };

      actionAddDirectory = new Action(i18n.tr("Add d&irectory..."), ResourceManager.getImageDescriptor("icons/add_directory.png")) {
         @Override
         public void run()
         {
            addDirectory();
         }
      };

      actionAddFile = new Action(i18n.tr("Add &file..."), ResourceManager.getImageDescriptor("icons/add_file.png")) {
         @Override
         public void run()
         {
            addFile();
         }
      };

      actionRename = new Action(i18n.tr("&Rename..."), SharedIcons.RENAME) {
         @Override
         public void run()
         {
            renameElement();
         }
      };

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteElements();
         }
      };

      actionUpdate = new Action(i18n.tr("&Update..."), ResourceManager.getImageDescriptor("icons/update_file.png")) {
         @Override
         public void run()
         {
            updateFile();
         }
      };

      actionEditPermissions = new Action(i18n.tr("&Permissions..."), ResourceManager.getImageDescriptor("icons/permissions.png")) {
         @Override
         public void run()
         {
            changePermissions();
         }
      };
   }

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener((m) -> fillContextMenu(m));

      Menu menu = manager.createContextMenu(fileTree.getControl());
      fileTree.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param manager Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      IStructuredSelection selection = fileTree.getStructuredSelection();
      if (selection.isEmpty())
      {
         manager.add(actionAddRoot);
      }
      else if (selection.size() == 1)
      {
         PathElement e = (PathElement)selection.getFirstElement();
         if (e.isFile())
         {
            manager.add(actionUpdate);
         }
         else
         {
            manager.add(actionAddDirectory);
            manager.add(actionAddFile);
         }
         manager.add(actionRename);
         manager.add(actionEditPermissions);
         manager.add(actionDelete);
      }
      else
      {
         manager.add(actionDelete);
      }
   }

   /**
    * Enable/disable file transfer mode
    *
    * @param enable true to enable
    */
   private void enableFileTransferMode(boolean enable)
   {
      fileTransferInProgress = enable;
      actionAddFile.setEnabled(!enable);
      view.enableSaveAction(!enable);
      layout(true, true);
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updateControlFromPolicy()
    */
   @Override
   public void updateControlFromPolicy()
   {
      Set<PathElement> newElementSet = new HashSet<PathElement>();
      try
      {
         FileDeliveryPolicy policyData = FileDeliveryPolicy.createFromXml(policy.getContent());
         newElementSet.addAll(Arrays.asList(policyData.elements));
      }
      catch(Exception e)
      {
         logger.error("Cannot parse file delivery policy XML", e);
      }

      checkForMissingElements(rootElements, newElementSet, false); 
      checkForMissingElements(newElementSet, rootElements, true); 
      fileTree.refresh(true);
   }

   /**
    * Check for missing elements
    *
    * @param newElements new element set
    * @param originalElements original element set
    * @param createMissing true if missing elements should be created
    */
   private void checkForMissingElements(Set<PathElement> newElements, Set<PathElement> originalElements, boolean createMissing)
   {
      Iterator<PathElement> iter = newElements.iterator();
      while (iter.hasNext())
      {
         PathElement newElement = iter.next();
         PathElement originalElement = null;
         for (PathElement e : originalElements)
         {
            if (newElement.getName().equals(e.getName()))
            {
               originalElement = e;
               break;
            }
         }
         if (originalElement == null)
         {
            if (createMissing)
            {
               fileTree.refresh(newElement, true);
               originalElements.add(newElement);
            }
            else
            {
               iter.remove();
            }
         }
         else if (newElement.isFile() != originalElement.isFile() && createMissing)
         {
            originalElements.add(newElement);
         }
         else if (!newElement.isFile())
         {            
            checkForMissingElements(newElement.getChildrenSet(), originalElement.getChildrenSet(), createMissing);
         }
      }
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updatePolicyFromControl()
    */
   @Override
   public void updatePolicyFromControl()
   {
      FileDeliveryPolicy data = new FileDeliveryPolicy();
      data.elements = rootElements.toArray(new PathElement[rootElements.size()]);
      try
      {
         policy.setContent(data.createXml());
      }
      catch(Exception e)
      {
         logger.error("Error serializing file delivery policy", e);
      }
   }

   /**
    * Add new directory to currently selected element
    */
   private void addDirectory()
   {
      IStructuredSelection selection = fileTree.getStructuredSelection();
      if ((selection.size() != 1) || ((PathElement)selection.getFirstElement()).isFile())
         return;

      addElement((PathElement)selection.getFirstElement());
   }

   /**
    * Add file
    */
   private void addFile()
   {
      IStructuredSelection selection = fileTree.getStructuredSelection();
      if ((selection.size() != 1) || ((PathElement)selection.getFirstElement()).isFile())
         return;

      FileDialog dlg = new FileDialog(getShell(), SWT.OPEN | SWT.MULTI);
      String selectionFolder = dlg.open();
      if (selectionFolder == null)
         return;

      final List<PathElement> uploadList = new ArrayList<PathElement>();
      for(String name : dlg.getFileNames())
      {
         File f = (name.charAt(0) == File.separatorChar) ? new File(name) : new File(new File(selectionFolder).getParentFile(), name);
         if (f.exists())
         {
            PathElement e = ((PathElement)selection.getFirstElement()).findChild(f.getName());
            if (e != null)
            {
               if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("File overwrite confirmation"),
                     i18n.tr("File named {0} already exists. Do you want to overwrite it?", f.getName())))
                  continue;
               e.setFile(f);
            }
            else
            {
               e = new PathElement((PathElement)selection.getFirstElement(), f.getName(), f, UUID.randomUUID(), new Date());  
               notSavedFiles.add(e.getGuid().toString());             
            }
            uploadList.add(e);
         }
         else
         {
            logger.info("File does not exist: " + name);          
         }
      }

      if (!uploadList.isEmpty())
      {
         fileTree.refresh();
         fireModifyListeners();
         enableFileTransferMode(true);

         logger.info("FileDeliveryPolicyEditor: " + uploadList.size() + " files to upload");          
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Uploading files"), view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               long totalBytes = 0;
               for(PathElement e : uploadList)
                  totalBytes += e.getLocalFile().length();

               monitor.beginTask("Uploading files", (int)(totalBytes / 16384)); // Count 16K blocks
               for(PathElement e : uploadList)
               {
                  logger.info("FileDeliveryPolicyEditor: uploading file " + e.getName() + " from " + e.getLocalFile());
                  monitor.subTask(e.getName());
                  session.uploadFileToServer(e.getLocalFile(), "FileDelivery-" + e.getGuid().toString(), new ProgressListener() {
                     long completed = 0;

                     @Override
                     public void setTotalWorkAmount(long workTotal)
                     {
                     }

                     @Override
                     public void markProgress(long workDone)
                     {
                        monitor.worked((int)((workDone - completed) / 16384));
                        completed = workDone;
                     }
                  });
               }
               monitor.done();
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot upload file");
            }

            @Override
            protected void jobFinalize()
            {
               runInUIThread(() -> {
                  enableFileTransferMode(false);
               });
            }
         }.start();
      }
   }

   /**
    * Delete selected file
    */
   private void deleteFile(final String name)
   {
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Deleting file"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.deleteServerFile("FileDelivery-" + name);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete file");
         }
      }.start();
   }

   /**
    * Update file content
    */
   private void updateFile()
   {
      IStructuredSelection selection = fileTree.getStructuredSelection();
      if ((selection.size() != 1) || !((PathElement)selection.getFirstElement()).isFile())
         return;

      FileDialog dlg = new FileDialog(getShell(), SWT.OPEN);
      String fileName = dlg.open();
      if (fileName == null)
         return;

      final File file = new File(fileName);
      if (!file.exists())
         return;

      enableFileTransferMode(true);

      final PathElement e = (PathElement)selection.getFirstElement();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Uploading file"), view) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            session.uploadFileToServer(file, "FileDelivery-" + e.getGuid().toString(), new ProgressListener() {
               long completed = 0;

               @Override
               public void setTotalWorkAmount(long workTotal)
               {
                  monitor.beginTask("Uploading file", (int)(file.length() / 16384));
                  monitor.subTask(e.getName());
               }

               @Override
               public void markProgress(long workDone)
               {
                  monitor.worked((int)((workDone - completed) / 16384));
                  completed = workDone;
               }
            });
            monitor.done();
            runInUIThread(() -> {
               ((PathElement)selection.getFirstElement()).updateCreationTime();
               fileTree.refresh(true);
               fireModifyListeners();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot upload file");
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> {
               enableFileTransferMode(false);
            });
         }
      }.start();
   }

   /**
    * Add new element
    */
   private void addElement(PathElement parent)
   {
      InputDialog dlg = new InputDialog(getShell(), (parent == null) ? i18n.tr("New Root Directory") : i18n.tr("New Directory"), i18n.tr("Enter name for new directory"), "", new IInputValidator() {
         @Override
         public String isValid(String newText)
         {
            if (newText.isEmpty())
               return i18n.tr("Name cannot be empty");
            if (parent != null)
               for (PathElement el : parent.getChildren())
               {
                  if(newText.equalsIgnoreCase(el.getName()))
                     return i18n.tr("Directory with this name already exists");
               }
            return null;
         }
      });
      if (dlg.open() != Window.OK)
         return;

      PathElement e = new PathElement(parent, dlg.getValue());
      if (parent == null)
      {
         rootElements.add(e);
         fileTree.refresh(true);
      }
      else
      {
         fileTree.refresh();
      }

      fireModifyListeners();
   }

   /**
    * Rename selected element
    */
   private void renameElement()
   {
      IStructuredSelection selection = fileTree.getStructuredSelection();
      if (selection.size() != 1)
         return;

      PathElement element = (PathElement)selection.getFirstElement();
      InputDialog dlg = new InputDialog(getShell(), element.isFile() ? i18n.tr("Rename File") : i18n.tr("Rename Directory"), i18n.tr("Enter new name"), element.getName(), new IInputValidator() {
         @Override
         public String isValid(String newText)
         {
            if (newText.isEmpty())
               return i18n.tr("Name cannot be empty");
            if (element.getParent() != null)
               for (PathElement el : element.getParent().getChildren())
               {
                  if(!el.equals(element) && newText.equalsIgnoreCase(el.getName()))
                     return i18n.tr("File or directory with this name already exists");
               }
            return null;
         }
      });
      if (dlg.open() != Window.OK)
         return;

      ((PathElement)selection.getFirstElement()).updateCreationTime(); 
      if (element.getParent() == null)
      {
         rootElements.remove(element);
         element.setName(dlg.getValue());
         rootElements.add(element);
         fileTree.refresh();
      }
      else
      {
         element.setName(dlg.getValue());
         fileTree.update(element, null);
      }
      
      fireModifyListeners();
   }

   /**
    * Change file permissions
    */
   private void changePermissions()
   {
      IStructuredSelection selection = fileTree.getStructuredSelection();
      if (selection.size() != 1)
         return;

      PathElement element = (PathElement)selection.getFirstElement();
      FilePermissionDialog dlg = new FilePermissionDialog(getShell(), element.getPermissions(), element.getOwner(), element.getOwnerGroup());
      if (dlg.open() != Window.OK)
         return;

      element.setPermissions(dlg.getPermissions());
      element.setOwner(dlg.getOwner());
      element.setOwnerGroup(dlg.getGroup());
      fileTree.update(element, null);

      fireModifyListeners();
   }

   /**
    * Delete selected elements
    */
   private void deleteElements()
   {
      IStructuredSelection selection = fileTree.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Delete Files"), i18n.tr("Delete selected files?")))
         return;

      for(Object o : selection.toList())
      {
         PathElement element = (PathElement)o;
         
         if (element.isFile())
            deleteFiles(element);
         
         if (element.getParent() == null)
         {
            rootElements.remove(o);
         }
         else
         {
            element.remove();
         }
      }

      fileTree.refresh(true);
      fireModifyListeners();
   }
   
   /**
    * Delete files under folder
    */
   private void deleteFiles(PathElement element)
   {
      if (element.isFile())
      {
         filesForDeletion.add(element.getGuid().toString());
      }
      else
      {
         for(PathElement el : element.getChildren())
         {
            deleteFiles(el);
         }
      }
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#fillLocalPullDown(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   public void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionAddRoot);
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   public void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionAddRoot);
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#isSaveAllowed()
    */
   @Override
   public String isSaveAllowed()
   {
      return fileTransferInProgress ? i18n.tr("File transfer is in progress") : null;
   }

   /**
    * Callback that will be called on policy save
    */
   public void onSave()
   {
      notSavedFiles.clear();
      for (String name : filesForDeletion)
      {
         deleteFile(name);
      }
      filesForDeletion.clear();
   }

   /**
    * Callback that will be called on save discard
    */
   public void onDiscard()
   {
      for (String name : notSavedFiles)
      {
         deleteFile(name);
      }
      notSavedFiles.clear();
   }
}
