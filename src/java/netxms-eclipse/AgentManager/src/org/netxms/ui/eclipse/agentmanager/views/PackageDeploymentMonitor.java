/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.views;

import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.packages.PackageDeploymentListener;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.agentmanager.views.helpers.DeploymentStatus;
import org.netxms.ui.eclipse.agentmanager.views.helpers.DeploymentStatusComparator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.DeploymentStatusLabelProvider;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Package deployment monitor
 */
public class PackageDeploymentMonitor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.PackageDeploymentMonitor"; //$NON-NLS-1$
	
	public static final int COLUMN_NODE = 0;
	public static final int COLUMN_STATUS = 1;
	public static final int COLUMN_ERROR = 2;
	
	private SortableTableViewer viewer;
	private long packageId;
	private Map<Long, DeploymentStatus> statusList = new HashMap<Long, DeploymentStatus>();
	private Action actionRedeploy;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { Messages.get().PackageDeploymentMonitor_ColumnNode, Messages.get().PackageDeploymentMonitor_ColumnStatus, Messages.get().PackageDeploymentMonitor_ColumnMessage };
		final int[] widths = { 200, 110, 400 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NODE, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new DeploymentStatusLabelProvider());
		viewer.setComparator(new DeploymentStatusComparator());
		
	   createActions();
		contributeToActionBars();
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
		getSite().getShell().getDisplay().asyncExec(new Runnable() {
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
	   actionRedeploy = new Action("Restart failed installation", SharedIcons.RESTART) { 
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
	   
	      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	      ConsoleJob job = new ConsoleJob(Messages.get().PackageManager_DeployAgentPackage, null, Activator.PLUGIN_ID, null) {
	         @Override
	         protected void runInternal(IProgressMonitor monitor) throws Exception
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
	                        MessageDialogHelper.openInformation(getSite().getShell(), Messages.get().PackageManager_Information, Messages.get().PackageManager_PkgDepCompleted);
	                     }
	                  });
	               }
	            });
	         }
	         
	         @Override
	         protected String getErrorMessage()
	         {
	            return Messages.get().PackageManager_DepStartError;
	         }
	      };
	      job.setUser(false);
	      job.start();
	   
	}
	
	  /**
    * Contribute actions to action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }
	
	/**
    * Fill local pulldown menu
    * @param manager menu manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionRedeploy);
   }
   
   /**
    * Fill local toolbar
    * @param manager menu manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionRedeploy);
   }

   /**
    * @param packageId the packageId to set
    */
   public void setPackageId(long packageId)
   {
      this.packageId = packageId;
   }
}
