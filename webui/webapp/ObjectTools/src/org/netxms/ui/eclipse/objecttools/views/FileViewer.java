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
package org.netxms.ui.eclipse.objecttools.views;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * File viewer
 */
public class FileViewer extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.FileViewer"; //$NON-NLS-1$
	
	private long nodeId;
	private String remoteFileName;
   private String fileID;
	private File currentFile;
	private Text textViewer;
	private final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private boolean follow;
	private ConsoleJob monitorJob;
   private Action actionClear;
	private Action actionScrollLock;
	
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
		textViewer = new Text(parent, SWT.H_SCROLL | SWT.V_SCROLL);
		textViewer.setEditable(false);
		textViewer.setFont(JFaceResources.getTextFont());
		
      createActions();
      contributeToActionBars();
      createPopupMenu();
      activateContext();
	}
	
   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.objecttools.context.FileViewer"); //$NON-NLS-1$
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
            textViewer.setText(""); //$NON-NLS-1$
         }
      };
      actionClear.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.clear_output"); //$NON-NLS-1$
      handlerService.activateHandler(actionClear.getActionDefinitionId(), new ActionHandler(actionClear));

      actionScrollLock = new Action(Messages.get().FileViewer_ScrollLock, Action.AS_CHECK_BOX) { 
         @Override
         public void run()
         {
         }
      };
      actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
      actionScrollLock.setChecked(false);
      actionScrollLock.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.scroll_lock"); //$NON-NLS-1$
      handlerService.activateHandler(actionScrollLock.getActionDefinitionId(), new ActionHandler(actionScrollLock));
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
    * @param manager
    *           Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    *           Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
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
      Menu menu = menuMgr.createContextMenu(textViewer);
      textViewer.setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   private void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
   }

   /* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		textViewer.setFocus();
	}

	/**
	 * @param file
	 */
   public void showFile(File file, boolean follow, String id)
	{
		currentFile = file;
      fileID = id;
		textViewer.setText(loadFile(currentFile));
		this.follow = follow;
		if (follow)
		{
   		monitorJob = new ConsoleJob(Messages.get().FileViewer_Download_File_Updates, null, Activator.PLUGIN_ID, null) {
   		   private boolean continueWork = true;
   		   
   		   @Override
   		   protected void canceling() 
   		   {
   		      continueWork = false;
   		   }
   		   
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               while(continueWork)
               {
                  final String s = session.waitForFileTail(fileID, 3000);
                  if (s != null)
                  {
                     runInUIThread(new Runnable() {                  
                        @Override
                        public void run()
                        {
                           if (!textViewer.isDisposed())
                           {
                              textViewer.append(s); 
                           }
                        }
                     });
                  }                    
               }
            }
   
            @Override
            protected String getErrorMessage()
            {
               return String.format(Messages.get().ObjectToolsDynamicMenu_DownloadError, remoteFileName, nodeId);
            }
   		};
   		monitorJob.setUser(false);
   		monitorJob.setSystem(true);
   		monitorJob.start();
		}
	}
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      if (follow)
      {
         monitorJob.cancel();
         final ConsoleJob job = new ConsoleJob(Messages.get().FileViewer_Stop_File_Monitoring, null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.cancelFileMonitoring(nodeId, fileID);
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
      super.dispose();
   }
	
	/**
	 * @param file
	 */
	private String loadFile(File file)
	{
		StringBuilder content = new StringBuilder();
		FileReader reader = null;
		char[] buffer = new char[32768];
		try
		{
			reader = new FileReader(file);
			int size = 0;
			while(size < 8192000)
			{
				int count = reader.read(buffer);
				if (count == -1)
					break;
				if (count == buffer.length)
				{
					content.append(buffer);
				}
				else
				{
					content.append(Arrays.copyOf(buffer, count));
				}
				size += count;
			}
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}
		finally
		{
			if (reader != null)
			{
				try
				{
					reader.close();
				}
				catch(IOException e)
				{
				}
			}
		}
		return content.toString();
	}
}
