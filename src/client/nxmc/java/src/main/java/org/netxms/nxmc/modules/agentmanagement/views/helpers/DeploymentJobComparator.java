/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.packages.PackageDeploymentJob;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.agentmanagement.views.DeploymentJobManager;

/**
 * Deployment job comparator
 */
public class DeploymentJobComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      PackageDeploymentJob job1 = (PackageDeploymentJob)e1;
      PackageDeploymentJob job2 = (PackageDeploymentJob)e2;
      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case DeploymentJobManager.COL_COMPLETION_TIME:
            result = (job1.getCompletionTime() != null) ?
                  ((job2.getCompletionTime() != null) ? 
                        job1.getCompletionTime().compareTo(job2.getCompletionTime()) : 1) :
                           ((job2.getCompletionTime() == null) ? 0 : -1);
            break;
         case DeploymentJobManager.COL_CREATION_TIME:
            result = (job1.getCreationTime() != null) ?
                  ((job2.getCreationTime() != null) ? 
                        job1.getCreationTime().compareTo(job2.getCreationTime()) : 1) :
                           ((job2.getCreationTime() == null) ? 0 : -1);
            break;
         case DeploymentJobManager.COL_DESCRIPTION:
            result = job1.getDescription().compareToIgnoreCase(job2.getDescription());
            break;
         case DeploymentJobManager.COL_ERROR_MESSAGE:
            result = job1.getErrorMessage().compareToIgnoreCase(job2.getErrorMessage());
            break;
         case DeploymentJobManager.COL_EXECUTION_TIME:
            result = (job1.getExecutionTime() != null) ?
                  ((job2.getExecutionTime() != null) ? 
                        job1.getExecutionTime().compareTo(job2.getExecutionTime()) : 1) :
                           ((job2.getExecutionTime() == null) ? 0 : -1);
            break;
         case DeploymentJobManager.COL_FILE:
            result = job1.getPackageFile().compareToIgnoreCase(job2.getPackageFile());
            break;
         case DeploymentJobManager.COL_ID:
            result = Long.compare(job1.getId(), job2.getId());
            break;
         case DeploymentJobManager.COL_NODE:
            NXCSession session = Registry.getSession();
            result = session.getObjectName(job1.getNodeId()).compareToIgnoreCase(session.getObjectName(job2.getNodeId()));
            break;
         case DeploymentJobManager.COL_PACKAGE_ID:
            result = Long.compare(job1.getPackageId(), job2.getPackageId());
            break;
         case DeploymentJobManager.COL_PACKAGE_NAME:
            result = job1.getPackageName().compareToIgnoreCase(job2.getPackageName());
            break;
         case DeploymentJobManager.COL_PACKAGE_TYPE:
            result = job1.getPackageType().compareToIgnoreCase(job2.getPackageType());
            break;
         case DeploymentJobManager.COL_PLATFORM:
            result = job1.getPlatform().compareToIgnoreCase(job2.getPlatform());
            break;
         case DeploymentJobManager.COL_STATUS:
            result = job1.getStatus().toString().compareTo(job2.getStatus().toString());
            break;
         case DeploymentJobManager.COL_USER:
            result = Long.compare(job1.getUserId(), job2.getUserId()); // FIXME: sort by user name
            break;
         case DeploymentJobManager.COL_VERSION:
            result = job1.getVersion().compareToIgnoreCase(job2.getVersion());
            break;
         default:
            result = 0;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
