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
package org.netxms.nxmc.modules.filemanager.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentFileData;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
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
   private static final I18n i18n = LocalizationHelper.getI18n(AgentFileViewer.class);
	public static final String ID = "org.netxms.ui.eclipse.filemanager.views.AgentFileViewer"; //$NON-NLS-1$
	
   private DynamicFileViewer viewer;
   private String remoteFileName;
   private boolean followChanges;
   private Action actionClear;
   private Action actionScrollLock;
   private Action actionCopy;
   private Action actionSelectAll;
   private Action actionFind;

   /**
    *  TODO: ? add file name
    *  
    * @param objectId
    * @param file
    * @param followChanges
    */
   public AgentFileViewer(long objectId, AgentFileData file, boolean followChanges)
   {
      super(i18n.tr("Remote File"), ResourceManager.getImageDescriptor("icons/object-views/file-view.png"), file.getRemoteName(), objectId, false);
      remoteFileName = file.getRemoteName();
      this.followChanges = followChanges;
      setName(remoteFileName);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AgentFileViewer view = (AgentFileViewer)super.cloneView();
      remoteFileName = view.remoteFileName;
      followChanges = view.followChanges;
      return view;
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {       
	   viewer = new DynamicFileViewer(parent, SWT.NONE, this);
	   viewer.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionCopy.setEnabled(viewer.canCopy());
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });

      createActions();
      createPopupMenu();
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
    * Create new file viewer view. Checks that file does not exceed allowed size.
    * In case if file is too large asks if it should be opened partly. 
    * @throws PartInitException 
    */
   public static boolean createView(View view, final long nodeId, final AgentFileData file, boolean followChanges) 
   {
      return createView(view, nodeId, file, followChanges, false, null);
   }
   
	/**
    * Create new file viewer view with custom line styler. Checks that file does not exceed allowed size.
	 * In case if file is too large asks if it should be opened partly. 
	 */
   public static boolean createView(View view, final long nodeId, final AgentFileData file, boolean followChanges, boolean ignoreContext, BaseFileViewer.LineStyler lineStyler)
	{
	   boolean exceedSize = file.getFile().length() > BaseFileViewer.MAX_FILE_SIZE;
	   if (exceedSize && 
	         !MessageDialogHelper.openConfirm(view.getWindow().getShell(), i18n.tr("File is too large"),
                  i18n.tr("File is too large to be displayed in full. Click OK to see beginning of the file.")))
      {
	      if (followChanges)
	      {
            final NXCSession session = Registry.getSession();
            final Job job = new Job(i18n.tr("Stopping file monitor"), null) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  session.cancelFileMonitoring(nodeId, file.getId());
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

      Perspective p = view.getPerspective();   
      AgentFileViewer fileView = new AgentFileViewer(nodeId, file, followChanges);
      if (p != null)
      {
         p.addMainView(fileView, true, ignoreContext);
      }
      else
      {
         PopOutViewWindow window = new PopOutViewWindow(fileView);
         window.open();
      }

      fileView.viewer.setLineStyler(lineStyler);
      fileView.viewer.showFile(file.getFile(), followChanges);
	   if (followChanges)
	   {
	      fileView.viewer.startTracking(nodeId, file.getId(), file.getRemoteName());
	   }
	   fileView.setName(file.getRemoteName()); 
	   return true;
	}
}
