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

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AgentFile;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.widgets.TextViewWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * File viewer
 */
public class FileViewer extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.FileViewer"; //$NON-NLS-1$
	
   private final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   private TextViewWidget textViewer;
   private String remoteFileName;
   private long nodeId;
	
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
	   textViewer = new TextViewWidget(parent, SWT.BORDER, remoteFileName, nodeId, this);   
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
	
	/* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      textViewer.setFocus();
   }
	
	/**
	 * Method that checks that file does not exceed allowed size.
	 * In case if file is too large asks if it should be opened partly. 
	 * @throws PartInitException 
	 */
	public static boolean createView(IWorkbenchWindow window, Shell shell, final AgentFile file, boolean tail, int offset, String secondaryId, final long nodeId) throws PartInitException
	{
	   boolean exceedSize = file.getFile().length() > TextViewWidget.MAX_FILE_SIZE;
	   if(exceedSize && !MessageDialogHelper.openConfirm(shell, "File is too large",
                  "File is too large to be fully shown. Click OK to see beginning of the file."))
      {
	      if(tail)
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
	   FileViewer view = (FileViewer)window.getActivePage().showView(FileViewer.ID, secondaryId,
            IWorkbenchPage.VIEW_ACTIVATE);
	   view.textViewer.showFile(file.getFile(), tail, file.getId(), offset, exceedSize);
	   
	   return true;
	}	 
}
