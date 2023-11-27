/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.filemanager.views.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Content provider for event tree
 */
public class ViewAgentFilesProvider implements ITreeContentProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ViewAgentFilesProvider.class);
   
   private Viewer viewer;
   private NXCSession session = Registry.getSession();
   
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
	   if (((AgentFile)parentElement).getChildren() == null)
	   {
         Job job = new Job(i18n.tr("Reading remote directory"), null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final List<AgentFile> files = session.listAgentFiles(((AgentFile)parentElement), ((AgentFile)parentElement).getFullName(), ((AgentFile)parentElement).getNodeId());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     ((AgentFile)parentElement).setChildren(files);
                     ((StructuredViewer)viewer).refresh(parentElement);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot read remote directory");
            }
         };
         job.setUser(false);
         job.start();
         return new AgentFile[] { new AgentFile(i18n.tr("Loading..."), AgentFile.PLACEHOLDER, (AgentFile)parentElement, ((AgentFile)parentElement).getNodeId()) };
	   }
		return ((AgentFile)parentElement).getChildren().toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		return ((AgentFile)element).getParent();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		return ((AgentFile)element).isDirectory();
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
   @SuppressWarnings("unchecked")
   @Override
   public Object[] getElements(Object inputElement)
   {
      List<AgentFile> list = new ArrayList<AgentFile>();
      for(AgentFile e : (List<AgentFile>)inputElement)
         if (e.getParent() == null)
            list.add(e);      
      return list.toArray();
   }
}
