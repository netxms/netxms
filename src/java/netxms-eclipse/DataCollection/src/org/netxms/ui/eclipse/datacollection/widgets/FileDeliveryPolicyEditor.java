/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.FileDeliveryPolicy;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.FileDeliveryPolicyContentProvider;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.FileDeliveryPolicyLabelProvider;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.PathElement;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Editor for file delivery policy
 */
public class FileDeliveryPolicyEditor extends AbstractPolicyEditor
{
   private TreeViewer fileTree;
   private Set<PathElement> rootElements = new HashSet<PathElement>();
   private Action actionAddRoot;
   private Action actionAddDirectory;
   private Action actionAddFile;
   private Action actionDelete;
   private Action actionRename;
   private Action actionUpdate;

   /**
    * @param parent
    * @param style
    */
   public FileDeliveryPolicyEditor(Composite parent, int style, AgentPolicy policy)
   {
      super(parent, style, policy);
      
      setLayout(new FillLayout());
      
      fileTree = new TreeViewer(this, SWT.NONE);
      fileTree.setContentProvider(new FileDeliveryPolicyContentProvider());
      fileTree.setLabelProvider(new FileDeliveryPolicyLabelProvider());
      fileTree.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            PathElement p1 = (PathElement)e1;
            PathElement p2 = (PathElement)e2;
            if (p1.isFile() && !p2.isFile())
               return 1;
            if (!p1.isFile() && p2.isFile())
               return -1;
            return p1.getName().compareToIgnoreCase(p2.getName());
         }
      });
      
      actionAddRoot = new Action("&Add root directory...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            addElement(null);
         }
      };

      actionAddDirectory = new Action("Add d&irectory...") {
         @Override
         public void run()
         {
            addDirectory();
         }
      };
      
      actionAddFile = new Action("Add &file...") {
         @Override
         public void run()
         {
            addFile();
         }
      };
      
      actionRename = new Action("&Rename...") {
         @Override
         public void run()
         {
         }
      };
      
      actionDelete = new Action("&Delete") {
         @Override
         public void run()
         {
         }
      };
      
      actionUpdate = new Action("&Update...") {
         @Override
         public void run()
         {
         }
      };
      
      createPopupMenu();
      
      updateControlFromPolicy();
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
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(fileTree.getControl());
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
         manager.add(actionDelete);
      }
      else
      {
         manager.add(actionDelete);
      }
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updateControlFromPolicy()
    */
   @Override
   protected void updateControlFromPolicy()
   {
      rootElements.clear();
      try
      {
         FileDeliveryPolicy policyData = FileDeliveryPolicy.createFromXml(getPolicy().getContent());
         rootElements.addAll(Arrays.asList(policyData.elements));
      }
      catch(Exception e)
      {
         Activator.logError("Cannot parse file delivery policy XML", e);
      }
      fileTree.setInput(rootElements.toArray());
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updatePolicyFromControl()
    */
   @Override
   public AgentPolicy updatePolicyFromControl()
   {
      FileDeliveryPolicy data = new FileDeliveryPolicy();
      data.elements = rootElements.toArray(new PathElement[rootElements.size()]);
      try
      {
         getPolicy().setContent(data.createXml());
      }
      catch(Exception e)
      {
         Activator.logError("Error serializing file delivery policy", e);
      }
      return getPolicy();
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
      if (dlg.open() == null)
         return;
      
      final List<PathElement> uploadList = new ArrayList<PathElement>();
      for(String name : dlg.getFileNames())
      {
         File f = new File(name);
         if (f.exists())
         {
            PathElement e = new PathElement((PathElement)selection.getFirstElement(), f.getName(), f, UUID.randomUUID());
            uploadList.add(e);
         }
      }
      
      if (!uploadList.isEmpty())
      {
         fileTree.refresh();
         fireModifyListeners();
         
         final NXCSession session = ConsoleSharedData.getSession();
         new ConsoleJob("Upload files", null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               monitor.beginTask("Upload files", uploadList.size());
               for(PathElement e : uploadList)
               {
                  monitor.subTask(e.getName());
                  session.uploadFileToServer(e.getLocalFile(), "FileDelivery-" + e.getGuid().toString(), null);
                  monitor.worked(1);
               }
               monitor.done();
            }
            
            @Override
            protected String getErrorMessage()
            {
               return "Cannot upload file";
            }
         }.start();
      }
   }
   
   /**
    * Add new element
    */
   private void addElement(PathElement parent)
   {
      InputDialog dlg = new InputDialog(getShell(), (parent == null) ? "New Root Element" : "New element", "Enter name for new element", "", new IInputValidator() {
         @Override
         public String isValid(String newText)
         {
            if (newText.isEmpty())
               return "Name cannot be empty";
            return null;
         }
      });
      if (dlg.open() != Window.OK)
         return;
      
      PathElement e = new PathElement(parent, dlg.getValue());
      if (parent == null)
      {
         rootElements.add(e);
         fileTree.setInput(rootElements.toArray());
      }
      else
      {
         fileTree.refresh();
      }
      
      fireModifyListeners();
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
    * @see org.eclipse.jface.text.IFindReplaceTarget#canPerformFind()
    */
   @Override
   public boolean canPerformFind()
   {
      // TODO Auto-generated method stub
      return false;
   }

   /**
    * @see org.eclipse.jface.text.IFindReplaceTarget#findAndSelect(int, java.lang.String, boolean, boolean, boolean)
    */
   @Override
   public int findAndSelect(int widgetOffset, String findString, boolean searchForward, boolean caseSensitive, boolean wholeWord)
   {
      // TODO Auto-generated method stub
      return 0;
   }

   /**
    * @see org.eclipse.jface.text.IFindReplaceTarget#getSelection()
    */
   @Override
   public Point getSelection()
   {
      // TODO Auto-generated method stub
      return null;
   }

   /**
    * @see org.eclipse.jface.text.IFindReplaceTarget#getSelectionText()
    */
   @Override
   public String getSelectionText()
   {
      // TODO Auto-generated method stub
      return null;
   }

   /**
    * @see org.eclipse.jface.text.IFindReplaceTarget#isEditable()
    */
   @Override
   public boolean isEditable()
   {
      // TODO Auto-generated method stub
      return false;
   }

   /**
    * @see org.eclipse.jface.text.IFindReplaceTarget#replaceSelection(java.lang.String)
    */
   @Override
   public void replaceSelection(String text)
   {
      // TODO Auto-generated method stub

   }

    /**
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#isFindAndReplaceRequired()
    */
   @Override
   public boolean isFindAndReplaceRequired()
   {
      // TODO Auto-generated method stub
      return false;
   }

}
