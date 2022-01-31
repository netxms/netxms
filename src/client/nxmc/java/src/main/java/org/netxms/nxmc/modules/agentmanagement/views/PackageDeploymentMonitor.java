/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.packages.PackageDeploymentListener;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentStatus;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentStatusComparator;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentStatusLabelProvider;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Package deployment monitor
 */
public class PackageDeploymentMonitor extends ObjectView
{
   private static I18n i18n = LocalizationHelper.getI18n(PackageDeploymentMonitor.class);
	public static final String ID = "PackageDeploymentMonitor"; //$NON-NLS-1$
	
	public static final int COLUMN_NODE = 0;
	public static final int COLUMN_STATUS = 1;
	public static final int COLUMN_ERROR = 2;
	
	private SortableTableViewer viewer;
	private long packageId;
	private Set<Long> applicableObjects;
	private Map<Long, DeploymentStatus> statusList = new HashMap<Long, DeploymentStatus>();
	private Action actionRedeploy;	

   public PackageDeploymentMonitor()
   {
      super(i18n.tr("Package Deployment Monitor"), ResourceManager.getImageDescriptor("icons/object-views/package_deploy.gif"), ID + UUID.randomUUID().toString(), false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      PackageDeploymentMonitor view = (PackageDeploymentMonitor)super.cloneView();
      view.packageId = packageId;
      view.applicableObjects = applicableObjects;
      //TODO: implement correct copy action
      return view;
   }
   
   
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
   @Override
   protected void createContent(Composite parent)
	{
		final String[] names = { i18n.tr("Node"), i18n.tr("Status"), i18n.tr("Message") };
		final int[] widths = { 200, 110, 400 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NODE, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new DeploymentStatusLabelProvider());
		viewer.setComparator(new DeploymentStatusComparator());
		
	   createActions();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/**
	 * @param nodeId
	 * @param status
	 * @param message
	 */
	public void viewStatusUpdate(final long nodeId, final int status, final String message)
	{
		getWindow().getShell().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				DeploymentStatus s = statusList.get(nodeId);
				if (s == null)
				{
					s = new DeploymentStatus(nodeId, status, message);
					statusList.put(nodeId, s);
					viewer.setInput(statusList.values().toArray());
				}
				else
				{
					s.setStatus(status);
					s.setMessage(message);
					viewer.update(s, null);
				}
			}
		});
	}
	
	private void createActions()
	{
	   actionRedeploy = new Action(i18n.tr("Restart failed installation"), SharedIcons.RESTART) { 
         @Override
         public void run()
         {
            redeployPackages();
         }
      };
	}
	
	private void redeployPackages()
	{
	   final Set<Long> objects = new HashSet<Long>();
	   Collection<DeploymentStatus> array =  statusList.values();
	   for(DeploymentStatus obj : array)
	   {
	      if(obj.getStatus() == 4) //status failed
	      {
	         objects.add(obj.getNodeObject().getObjectId());
	      }
	   }
	   
	      final NXCSession session = (NXCSession)Registry.getSession();
	      Job job = new Job(i18n.tr("Deploy agent package"), this) {
	         protected void run(IProgressMonitor monitor) throws Exception
	         {
	            session.deployPackage(packageId, objects.toArray(new Long[objects.size()]), new PackageDeploymentListener() {
	               
	               @Override
	               public void statusUpdate(long nodeId, int status, String message)
	               {
                     viewStatusUpdate(nodeId, status, message);
	               }
	               
	               @Override
	               public void deploymentStarted()
	               {
	                 
	               }
	               
	               @Override
	               public void deploymentComplete()
	               {
	                  runInUIThread(new Runnable() {
	                     @Override
	                     public void run()
	                     {
	                        MessageDialogHelper.openInformation(getWindow().getShell(), i18n.tr("Information"), i18n.tr("Package deployment completed"));
	                     }
	                  });
	               }
	            });
	         }
	         
	         @Override
	         protected String getErrorMessage()
	         {
	            return i18n.tr("Cannot start package deployment");
	         }
	      };
	      job.setUser(false);
	      job.start();
	   
	}
   
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      manager.add(actionRedeploy);
      manager.add(new Separator());
      super.fillLocalToolbar(manager);
   }

   /**
    * @param packageId the packageId to set
    */
   public void setPackageId(long packageId)
   {
      this.packageId = packageId;
   }

   /**
    * @param packageId the packageId to set
    */
   public void setApplicableObjects(final Set<Long> objects)
   {
      this.applicableObjects = objects;
   }

   @Override
   public boolean isValidForContext(Object context)
   {
      if (context != null && context instanceof AbstractObject)
      {
         for (long objectId : applicableObjects)
         {
            if (((AbstractObject)context).isChildOf(objectId) || ((AbstractObject)context).getObjectId() == objectId)
               return true;
         }
      }
      return false;
   }

   @Override
   public boolean isCloseable()
   {
      return true;
   }  
}
