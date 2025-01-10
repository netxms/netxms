/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.packages.PackageDeploymentJob;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.modules.agentmanagement.views.DeploymentJobManager;

/**
 * Filter for deployment job manager view
 */
public class DeploymentJobFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;
   private boolean hideInactive = false;

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      PackageDeploymentJob job = (PackageDeploymentJob)element;
      
      if (hideInactive && !job.isActive())
         return false;
      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      
      if (job.getPackageName().toLowerCase().contains(filterString))
         return true;

      if (job.getPackageFile().toLowerCase().contains(filterString))
         return true;

      if (job.getDescription().toLowerCase().contains(filterString))
         return true;
      
      if (Registry.getSession().getObjectName(job.getNodeId()).toLowerCase().contains(filterString))
         return true;

      if (job.getPlatform().toLowerCase().contains(filterString))
         return true;

      if (job.getErrorMessage().toLowerCase().contains(filterString))
         return true;
      
      String status = ((ITableLabelProvider)((TableViewer)viewer).getLabelProvider()).getColumnText(job, DeploymentJobManager.COL_STATUS);
      if (status.toLowerCase().contains(filterString))
         return true;

      return false;
   }

   /**
    * Set filter string.
    * 
    * @param filterString new filter string
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }

   /**
    * Show/hide inactive jobs
    * 
    * @param hide true to hide inactive jobs
    */
   public void setHideInactive(boolean hide)
   {
      this.hideInactive = hide;
   }
}
