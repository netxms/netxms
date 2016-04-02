/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AgentFileData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.filemanager.Activator;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.widgets.BaseFileViewer;
import org.netxms.ui.eclipse.filemanager.widgets.DynamicFileViewer;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * File viewer
 */
public class AgentFileViewer extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.filemanager.views.AgentFileViewer"; //$NON-NLS-1$
	
   private final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   private DynamicFileViewer viewer;
   private String remoteFileName;
   private long nodeId;
   private boolean followChanges;
   private Action actionClear;
   private Action actionScrollLock;
   private Action actionCopy;
   private Action actionSelectAll;
   private Action actionFind;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		// Secondary ID must by in form nodeId&remoteFileName
		String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
		if (parts.length != 2)
			throw new PartInitException("Internal error"); //$NON-NLS-1$
		
		nodeId = Long.parseLong(parts[0]);
		AbstractObject object = session.findObjectById(nodeId);
		if ((object == null) || (object.getObjectClass() != AbstractObject.OBJECT_NODE))
			throw new PartInitException(Messages.get().FileViewer_InvalidObjectID);
		
		try
		{
			remoteFileName = URLDecoder.decode(parts[1], "UTF-8"); //$NON-NLS-1$
		}
		catch(UnsupportedEncodingException e)
		{
			throw new PartInitException("Internal error", e); //$NON-NLS-1$
		}
		
		setPartName(object.getObjectName() + ": " + remoteFileName); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
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

	   activateContext();

      createActions();
      contributeToActionBars();
      createPopupMenu();
	}
	
   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.filemanager.context.AgentFileViewer"); //$NON-NLS-1$
      }
   }
	
   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

      actionClear = new Action(Messages.get().FileViewer_ClearOutput, SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            viewer.clear();
         }
      };
      actionClear.setActionDefinitionId("org.netxms.ui.eclipse.filemanager.commands.clear_output"); //$NON-NLS-1$
      handlerService.activateHandler(actionClear.getActionDefinitionId(), new ActionHandler(actionClear));

      actionScrollLock = new Action(Messages.get().FileViewer_ScrollLock, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            viewer.setScrollLock(actionScrollLock.isChecked());
         }
      };
      actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
      actionScrollLock.setChecked(false);
      actionScrollLock.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.scroll_lock"); //$NON-NLS-1$
      handlerService.activateHandler(actionScrollLock.getActionDefinitionId(), new ActionHandler(actionScrollLock));

      actionCopy = new Action("&Copy") {
         @Override
         public void run()
         {
            viewer.copy();
         }
      };
      actionCopy.setEnabled(false);
      actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.filemanager.commands.copy"); //$NON-NLS-1$
      handlerService.activateHandler(actionCopy.getActionDefinitionId(), new ActionHandler(actionCopy));

      actionSelectAll = new Action("Select &all") {
         @Override
         public void run()
         {
            viewer.selectAll();
         }
      };
      actionSelectAll.setActionDefinitionId("org.netxms.ui.eclipse.filemanager.commands.select_all"); //$NON-NLS-1$
      handlerService.activateHandler(actionSelectAll.getActionDefinitionId(), new ActionHandler(actionSelectAll));
      
      actionFind = new Action("&Find", SharedIcons.FIND) {
         @Override
         public void run()
         {
            viewer.showSearchBar();
         }
      };
      actionFind.setActionDefinitionId("org.netxms.ui.eclipse.filemanager.commands.find"); //$NON-NLS-1$
      handlerService.activateHandler(actionFind.getActionDefinitionId(), new ActionHandler(actionFind));
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
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionFind);
      if (followChanges)
      {
         manager.add(new Separator());
         manager.add(actionClear);
         manager.add(actionScrollLock);
      }
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
   
	/* (non-Javadoc)
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
   public static boolean createView(String secondaryId, final long nodeId, final AgentFileData file, boolean followChanges) throws PartInitException
   {
      return createView(secondaryId, nodeId, file, followChanges, null);
   }
   
	/**
    * Create new file viewer view with custom line styler. Checks that file does not exceed allowed size.
	 * In case if file is too large asks if it should be opened partly. 
	 * @throws PartInitException 
	 */
	public static boolean createView(String secondaryId, final long nodeId, final AgentFileData file, boolean followChanges, BaseFileViewer.LineStyler lineStyler) throws PartInitException
	{
      final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
	   boolean exceedSize = file.getFile().length() > BaseFileViewer.MAX_FILE_SIZE;
	   if (exceedSize && 
	         !MessageDialogHelper.openConfirm(window.getShell(), "File is too large",
                  "File is too large to be displayed in full. Click OK to see beginning of the file."))
      {
	      if (followChanges)
	      {
	         final NXCSession session = (NXCSession)ConsoleSharedData.getSession();

   	      final ConsoleJob job = new ConsoleJob(Messages.get().FileViewer_Stop_File_Monitoring, null, Activator.PLUGIN_ID, null) {
               @Override
               protected void runInternal(IProgressMonitor monitor) throws Exception
               {
                  session.cancelFileMonitoring(nodeId, file.getId());
               }
               
               @Override
               protected String getErrorMessage()
               {
                  return Messages.get().FileViewer_Cannot_Stop_File_Monitoring;
               }
            };
            job.setUser(false);
            job.setSystem(true);
            job.start();
         }
         return false;
      }
	   AgentFileViewer view = (AgentFileViewer)window.getActivePage().showView(AgentFileViewer.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
	   view.viewer.setLineStyler(lineStyler);
	   view.followChanges = followChanges;
	   view.viewer.showFile(file.getFile());
	   if (followChanges)
	   {
	      view.viewer.startTracking(nodeId, file.getId(), file.getRemoteName());
	   }
	   return true;
	}
}
