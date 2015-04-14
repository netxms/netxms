/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.filemanager.views.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerFile;
import org.netxms.ui.eclipse.filemanager.Activator;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Content provider for event tree
 */
public class ViewAgentFilesProvider implements ITreeContentProvider
{
   private Viewer viewer;
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
	   this.viewer = viewer;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(final Object parentElement)
	{
	   if(((ServerFile)parentElement).getChildren() == null)
	   {
         ConsoleJob job = new ConsoleJob(Messages.get().ViewAgentFilesProvider_JobTitle, null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               final ServerFile[] files = session.listAgentFiles(((ServerFile)parentElement), ((ServerFile)parentElement).getFullName(), ((ServerFile)parentElement).getNodeId());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     ((ServerFile)parentElement).setChildren(files);
                     ((StructuredViewer)viewer).refresh(parentElement);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().ViewAgentFilesProvider_JobError;
            }
         };
         job.setUser(false);
         job.start();
         return new ServerFile[] { new ServerFile(Messages.get().ViewAgentFilesProvider_Loading, ServerFile.PLACEHOLDER, (ServerFile)parentElement, ((ServerFile)parentElement).getNodeId()) };
	   }
		return ((ServerFile)parentElement).getChildren();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		return ((ServerFile)element).getParent();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		return ((ServerFile)element).isDirectory();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}

	/* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
    */
   @Override
   public Object[] getElements(Object inputElement)
   {
      List<ServerFile> list = new ArrayList<ServerFile>();
      for(ServerFile e : (ServerFile[])inputElement)
         if (e.getParent() == null)
            list.add(e);      
      return list.toArray();
   }
}
