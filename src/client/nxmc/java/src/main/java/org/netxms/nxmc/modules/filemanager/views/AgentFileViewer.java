/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentFileData;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.helpers.LineStyler;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.widgets.BaseFileViewer;
import org.netxms.nxmc.modules.filemanager.widgets.DynamicFileViewer;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * File viewer
 */
public class AgentFileViewer extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(AgentFileViewer.class);

   private DynamicFileViewer viewer;
   private AgentFileData remoteFile;
   private String remoteFileName;
   private long maxFileSize;
   private boolean followChanges;
   private Action actionClear;
   private Action actionScrollLock;
   private Action actionCopy;
   private Action actionSelectAll;
   private Action actionFind;
   private boolean loadFile = false;

   /**
    * Create new agent file viewer.
    *
    * @param objectId node object ID
    * @param remoteFile remote file handle
    * @param maxFileSize maximum download size, 0 == UNLIMITED
    * @param followChanges true to follow file changes
    * @param contextId context ID
    */
   protected AgentFileViewer(long objectId, AgentFileData remoteFile, long maxFileSize, boolean followChanges, long contextId)
   {
      super(LocalizationHelper.getI18n(AgentFileViewer.class).tr("Remote File"), ResourceManager.getImageDescriptor("icons/object-views/file-view.png"), remoteFile.getRemoteName(), objectId,
            contextId, false);
      this.remoteFile = remoteFile;
      remoteFileName = remoteFile.getRemoteName();
      this.followChanges = followChanges;
      this.maxFileSize = maxFileSize;
      setName(remoteFile.getRemoteName());
   }

   /**
    * Clone constructor
    */
   protected AgentFileViewer()
   {
      super(LocalizationHelper.getI18n(AgentFileViewer.class).tr("Remote File"), ResourceManager.getImageDescriptor("icons/object-views/file-view.png"), null, 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AgentFileViewer view = (AgentFileViewer)super.cloneView();
      view.remoteFile = remoteFile;
      view.remoteFileName = remoteFileName;
      view.followChanges = followChanges;
      view.maxFileSize = maxFileSize;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      AgentFileViewer origin = (AgentFileViewer)view;
      viewer.setLineStyler(origin.viewer.getLineStyler());
      viewer.replaceContent(origin.viewer, followChanges);
      if (followChanges)
      {
         Job job = new Job(i18n.tr("Download file from agent and start following changes"), this) {
            @Override
            protected void run(final IProgressMonitor monitor) throws Exception
            {
               final AgentFileData file = session.downloadFileFromAgent(getObjectId(), remoteFile.getRemoteName(), maxFileSize, true, null);              
               runInUIThread(() -> {
                  remoteFile = file;
                  viewer.startTracking(remoteFile.getMonitorId(), getObjectId(), remoteFile.getRemoteName());
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Error downloading file {0} from node {1}", remoteFile.getRemoteName(), getObjectName());
            }
         };
         job.setUser(false);
         job.start();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {       
	   viewer = new DynamicFileViewer(parent, SWT.NONE, this);
      viewer.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionCopy.setEnabled(viewer.canCopy());
         }
      });

      createActions();
      createPopupMenu();
      if (loadFile)
         refresh();
	}

   /**
    * Create actions
    */
   private void createActions()
   {
      actionClear = new Action(i18n.tr("&Clear output"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            viewer.clear();
         }
      };
      addKeyBinding("Ctrl+L", actionClear);

      actionScrollLock = new Action(i18n.tr("&Scroll lock"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            viewer.setScrollLock(actionScrollLock.isChecked());
         }
      };
      actionScrollLock.setImageDescriptor(ResourceManager.getImageDescriptor("icons/scroll-lock.png"));
      actionScrollLock.setChecked(false);

      actionCopy = new Action(i18n.tr("&Copy")) {
         @Override
         public void run()
         {
            viewer.copy();
         }
      };
      actionCopy.setEnabled(false);
      actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.filemanager.commands.copy");
      addKeyBinding("Ctrl+C", actionCopy);

      actionSelectAll = new Action(i18n.tr("Select &all")) {
         @Override
         public void run()
         {
            viewer.selectAll();
         }
      };
      addKeyBinding("Ctrl+A", actionSelectAll);
      
      actionFind = new Action(i18n.tr("&Find"), SharedIcons.FIND) {
         @Override
         public void run()
         {
            viewer.showSearchBar();
         }
      };
      addKeyBinding("Ctrl+F", actionFind);      
   }

   /**
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionFind);
      if (followChanges)
      {
         manager.add(new Separator());
         manager.add(actionClear);
         manager.add(actionScrollLock);
      }
      manager.add(new Separator());
   }

   /**
    * Create pop-up menu
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
      Menu menu = menuMgr.createContextMenu(viewer.getTextControl());
      viewer.getTextControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   private void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionFind);
      if (followChanges)
      {
         manager.add(new Separator());
         manager.add(actionClear);
         manager.add(actionScrollLock);
      }
      manager.add(new Separator());
      manager.add(actionSelectAll);
      manager.add(actionCopy);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.setFocus();
   }

   /**
    * Create new file viewer view. Checks that file does not exceed allowed size. In case if file is too large asks if it should be
    * opened partly.
    * 
    * @param viewPlacement placement information for new view
    * @param nodeId owning node ID
    * @param maxFileSize
    * @param file file information
    * @param maxFileSize maximum download size, 0 == unlimited
    * @param followChanges true to follow file changes
    * @param contextId additional context ID
    * @return true if view was created successfully
    *
    * @throws PartInitException
    */
   public static boolean createView(ViewPlacement viewPlacement, final long nodeId, final AgentFileData file, long maxFileSize, boolean followChanges, long contextId)
   {
      return createView(viewPlacement, nodeId, file, maxFileSize, followChanges, false, contextId, null);
   }

	/**
    * Create new file viewer view with custom line styler. Will check that file does not exceed maximum allowed size, and ask user
    * to show only part of the file if file is too large.
    *
    * @param viewPlacement placement information for new view
    * @param nodeId owning node ID
    * @param maxFileSize
    * @param file file information
    * @param maxFileSize maximum download size, 0 == unlimited
    * @param followChanges true to follow file changes
    * @param ignoreContext true to ignore context
    * @param contextId additional context ID
    * @param lineStyler line styler
    * @return true if view was created successfully
    */
   public static boolean createView(ViewPlacement viewPlacement, final long nodeId, final AgentFileData file,
         long maxFileSize, boolean followChanges, boolean ignoreContext, long contextId, LineStyler lineStyler)
	{
      I18n i18n = LocalizationHelper.getI18n(AgentFileViewer.class);
	   boolean exceedSize = file.getFile().length() > BaseFileViewer.MAX_FILE_SIZE;
	   if (exceedSize &&
          !MessageDialogHelper.openConfirm(viewPlacement.getWindow().getShell(), i18n.tr("Large File"),
                i18n.tr("File is too large to be displayed in full. Click OK to see beginning of the file.")))
      {
	      if (followChanges)
	      {
            final NXCSession session = Registry.getSession();
            final Job job = new Job(i18n.tr("Stopping file monitor"), null) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  session.cancelFileMonitoring(file.getMonitorId());
               }

               @Override
               protected String getErrorMessage()
               {
                  return i18n.tr("Cannot stop file monitor");
               }
            };
            job.setUser(false);
            job.setSystem(true);
            job.start();
         }
         return false;
      }

      AgentFileViewer fileView = new AgentFileViewer(nodeId, file, maxFileSize, followChanges, contextId);
      viewPlacement.openView(fileView);
      fileView.viewer.setLineStyler(lineStyler);
      fileView.viewer.showFile(file.getFile(), followChanges);
	   if (followChanges)
	   {
         fileView.viewer.startTracking(file.getMonitorId(), nodeId, file.getRemoteName());
	   }
	   fileView.setName(file.getRemoteName()); 
	   return true;
	}
   
   

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);  
      memento.set("remoteFileName", remoteFileName);  
      memento.set("maxFileSize", maxFileSize);
      memento.set("followChanges", followChanges);  
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      remoteFileName = memento.getAsString("remoteFileName");
      followChanges = memento.getAsBoolean("followChanges", false);
      maxFileSize = memento.getAsLong("maxFileSize", 0);
      loadFile = true;     
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(followChanges ? i18n.tr("Download file from agent and start following changes") : i18n.tr("Download file from agent"), this) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            final AgentFileData file = session.downloadFileFromAgent(getObjectId(), remoteFileName, maxFileSize, followChanges, new ProgressListener() {
               @Override
               public void setTotalWorkAmount(long workTotal)
               {
                  monitor.beginTask(String.format(i18n.tr("Download file %s"), remoteFileName), (int)workTotal);
               }

               @Override
               public void markProgress(long workDone)
               {
                  monitor.worked((int)workDone);
               }
            });

            runInUIThread(() -> {
               remoteFile = file;
               viewer.showFile(file.getFile(), followChanges);
               if (followChanges)
                  viewer.startTracking(remoteFile.getMonitorId(), getObjectId(), remoteFile.getRemoteName());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Error downloading file %s from node %s"), remoteFileName, getObjectName());
         }
      }.start();
   }
}
