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
package org.netxms.nxmc.modules.agentmanagement;

import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.netxms.client.packages.PackageDeploymentListener;
import org.netxms.nxmc.modules.agentmanagement.views.PackageDeploymentMonitor;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentStatus;

/**
 * Listener interface for package deployment progress for one package deployment
 */
public class PackageDeployment implements PackageDeploymentListener
{
   private Set<PackageDeploymentMonitor> monitors;
   private Map<Long, DeploymentStatus> statusList = new HashMap<Long, DeploymentStatus>();

   /**
    * Constructor with first view
    * 
    * @param monitor
    */
   public PackageDeployment(PackageDeploymentMonitor monitor)
   {
      monitors = new HashSet<PackageDeploymentMonitor>();
      monitors.add(monitor);
   }

   /**
    * Status update for node.
    * 
    * @param nodeId node object ID
    * @param status status code (defined in PackageDeploymentListener interface)
    * @param message error message, if applicable (otherwise null)
    */
   public void statusUpdate(long nodeId, int status, String message)
   {
      synchronized(monitors)
      {
         DeploymentStatus s = statusList.get(nodeId);
         if (s == null)
         {
            s = new DeploymentStatus(nodeId, status, message);
            statusList.put(nodeId, s);
         }
         else
         {
            s.setStatus(status);
            s.setMessage(message);
         }
         for(PackageDeploymentMonitor monitor : monitors)
         {
            monitor.viewStatusUpdate();
         }
      }
   }

   /**
    * Register one more monitor
    * 
    * @param monitor
    */
   public void addMonitor(PackageDeploymentMonitor monitor)
   {
      synchronized(monitors)
      {
         monitors.add(monitor);
         monitor.viewStatusUpdate();
      }
   }

   /**
    * Register one more monitor
    * 
    * @param monitor
    */
   public void removeMonitor(PackageDeploymentMonitor monitor)
   {
      synchronized(monitors)
      {
         monitors.remove(monitor);
      }
   }

   /**
    * @see org.netxms.client.packages.PackageDeploymentListener#deploymentStarted()
    */
   @Override
   public void deploymentStarted()
   {
   }

   /**
    * @see org.netxms.client.packages.PackageDeploymentListener#deploymentComplete()
    */
   @Override
   public void deploymentComplete()
   {
      synchronized(monitors)
      {
         for(PackageDeploymentMonitor monitor : monitors)
         {
            monitor.deploymentComplete();
         }
      }
   }

   /**
    * Get current deployment list
    * 
    * @return current deployment list
    */
   public Collection<DeploymentStatus> getDeployments()
   {
      Collection<DeploymentStatus> deployments;
      synchronized(monitors)
      {
         deployments = statusList.values();
      }
      return deployments;
   }
}
