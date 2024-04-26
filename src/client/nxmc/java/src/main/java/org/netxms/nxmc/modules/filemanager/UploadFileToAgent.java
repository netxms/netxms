/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
 */package org.netxms.nxmc.modules.filemanager;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.server.ServerFile;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.dialogs.StartServerToAgentFileUploadDialog;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Action: upload file from server to agent(s)
 *
 */
public class UploadFileToAgent extends ObjectAction<AbstractObject>
{
   private final I18n i18n = LocalizationHelper.getI18n(UploadFileToAgent.class);

   /**
    * Create action for linking asset to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public UploadFileToAgent(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(AbstractObject.class, LocalizationHelper.getI18n(UploadFileToAgent.class).tr("Upload file..."), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/upload.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<AbstractObject> objects)
   {
      final NXCSession session = Registry.getSession();
      Set<Long> nodes = new HashSet<Long>();
      for (Object object : objects)
      {
         if (object instanceof AbstractNode)
         {
            nodes.add(((AbstractObject)object).getObjectId());
         }
         else if ((object instanceof Collector) || (object instanceof Container) || (object instanceof ServiceRoot) || 
               (object instanceof Subnet)  || (object instanceof EntireNetwork))
         {
            Set<AbstractObject> set = ((AbstractObject)object).getAllChildren(AbstractObject.OBJECT_NODE);
            for(AbstractObject o : set)
               nodes.add(o.getObjectId());
         }
      }

      boolean canScheduleFileUpload = (session.getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD) > 0;
		final StartServerToAgentFileUploadDialog dlg = new StartServerToAgentFileUploadDialog(getShell(), canScheduleFileUpload);
		if (dlg.open() == Window.OK)
		{
			final Long[] nodeIdList = nodes.toArray(new Long[nodes.size()]);
			new Job(i18n.tr("Initiate file upload to agent"), null, getMessageArea()) {
				@Override
				protected String getErrorMessage()
				{
					return i18n.tr("Cannot start file upload job");
				}

				@Override
				protected void run(IProgressMonitor monitor) throws Exception
				{
				   boolean multipleFiles = dlg.getServerFiles().size() > 1;
				   for(ServerFile sf : dlg.getServerFiles())
				   {
   				   String remoteFileName = dlg.getRemoteFileName();
   				   if (!remoteFileName.isEmpty())
   				   {
                     if (remoteFileName.endsWith("/") || remoteFileName.endsWith("\\"))
   				      {
   				         remoteFileName += sf.getName();
   				      }
   				      else if (multipleFiles)
   				      {
                        remoteFileName += "/" + sf.getName();
   				      }
   				   }
   				   else 
   				   {
   				      if (!dlg.isScheduled())
   				         remoteFileName = null;
   				   }
   					for(int i = 0; i < nodeIdList.length; i++)
   					{
   					   if (dlg.isScheduled())
   					   {
   					      ScheduledTask task = dlg.getScheduledTask();
                        String parameters = sf.getName() + "," + remoteFileName;
   					      task.setParameters(parameters);
   					      task.setObjectId(nodeIdList[i]);
   					      session.addScheduledTask(task);
   					   }
   					   else
   					   {
                        session.uploadFileToAgent(nodeIdList[i], sf.getName(), remoteFileName);
   					   }
   					}
				   }
				}
			}.start();
		}
	}

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      if (!selection.isEmpty())
      {
         for (Object obj : ((IStructuredSelection)selection).toList())
         {
            if (!(((obj instanceof AbstractNode) && ((AbstractNode)obj).hasAgent()) || 
                  (obj instanceof Collector) || (obj instanceof Container) || (obj instanceof ServiceRoot) || (obj instanceof Subnet) ||
                (obj instanceof EntireNetwork)))
            {
               return false;
            }
         }
      }
      else
      {
         return false;
      }
      return true;
   }
}
