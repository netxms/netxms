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

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.packages.PackageDeploymentJob;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.DeploymentJobManager;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for deployment job list
 */
public class DeploymentJobLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(DeploymentJobLabelProvider.class);
   private final String[] states = {
         i18n.tr("Scheduled"), i18n.tr("Pending"), i18n.tr("Initializing"), i18n.tr("Transferring file"), i18n.tr("Installing"),
         i18n.tr("Waiting for agent"), i18n.tr("Completed"), i18n.tr("Failed"), i18n.tr("Cancelled")
   };

   private NXCSession session = Registry.getSession();
   private TableViewer viewer;

   /**
    * Create provider.
    *
    * @param viewer parent viewer
    */
   public DeploymentJobLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      PackageDeploymentJob job = (PackageDeploymentJob)element;
      switch(columnIndex)
      {
         case DeploymentJobManager.COL_COMPLETION_TIME:
            if ((job.getCompletionTime() == null) || (job.getCompletionTime().getTime() == 0))
               return "";
            return DateFormatFactory.getDateTimeFormat().format(job.getCompletionTime());
         case DeploymentJobManager.COL_CREATION_TIME:
            if ((job.getCreationTime() == null) || (job.getCreationTime().getTime() == 0))
               return "";
            return DateFormatFactory.getDateTimeFormat().format(job.getCreationTime());
         case DeploymentJobManager.COL_DESCRIPTION:
            return job.getDescription();
         case DeploymentJobManager.COL_ERROR_MESSAGE:
            return job.getErrorMessage();
         case DeploymentJobManager.COL_EXECUTION_TIME:
            if ((job.getExecutionTime() == null) || (job.getExecutionTime().getTime() == 0))
               return "";
            return DateFormatFactory.getDateTimeFormat().format(job.getExecutionTime());
         case DeploymentJobManager.COL_FILE:
            return job.getPackageFile();
         case DeploymentJobManager.COL_ID:
            return Long.toString(job.getId());
         case DeploymentJobManager.COL_NODE:
            return session.getObjectName(job.getNodeId());
         case DeploymentJobManager.COL_PACKAGE_ID:
            return Long.toString(job.getPackageId());
         case DeploymentJobManager.COL_PACKAGE_NAME:
            return job.getPackageName();
         case DeploymentJobManager.COL_PACKAGE_TYPE:
            return job.getPackageType();
         case DeploymentJobManager.COL_PLATFORM:
            return job.getPlatform();
         case DeploymentJobManager.COL_STATUS:
            try
            {
               return states[job.getStatus().getValue()];
            }
            catch(ArrayIndexOutOfBoundsException e)
            {
               return i18n.tr("Unknown");
            }
         case DeploymentJobManager.COL_USER:
            AbstractUserObject user = session.findUserDBObjectById(job.getUserId(), new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : ("[" + Long.toString(job.getUserId()) + "]");
         case DeploymentJobManager.COL_VERSION:
            return job.getVersion();
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      viewer = null;
      super.dispose();
   }
}
