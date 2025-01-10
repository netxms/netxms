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
package org.netxms.client.packages;

import java.util.Date;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.PackageDeploymentStatus;

/**
 * Information about package deployment job
 */
public class PackageDeploymentJob
{
   private long id;
   private long nodeId;
   private int userId;
   private PackageDeploymentStatus status;
   private Date creationTime;
   private Date executionTime;
   private Date completionTime;
   private long packageId;
   private String packageType;
   private String packageName;
   private String platform;
   private String version;
   private String packageFile;
   private String command;
   private String description;
   private String errorMessage;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public PackageDeploymentJob(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      nodeId = msg.getFieldAsInt64(baseId + 1);
      userId = msg.getFieldAsInt32(baseId + 2);
      status = PackageDeploymentStatus.getByValue(msg.getFieldAsInt32(baseId + 3));
      creationTime = msg.getFieldAsDate(baseId + 4);
      executionTime = msg.getFieldAsDate(baseId + 5);
      completionTime = msg.getFieldAsDate(baseId + 6);
      packageId = msg.getFieldAsInt64(baseId + 7);
      packageType = msg.getFieldAsString(baseId + 8);
      packageName = msg.getFieldAsString(baseId + 9);
      platform = msg.getFieldAsString(baseId + 10);
      version = msg.getFieldAsString(baseId + 11);
      packageFile = msg.getFieldAsString(baseId + 12);
      command = msg.getFieldAsString(baseId + 13);
      description = msg.getFieldAsString(baseId + 14);
      errorMessage = msg.getFieldAsString(baseId + 15);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the userId
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * @return the status
    */
   public PackageDeploymentStatus getStatus()
   {
      return status;
   }

   /**
    * Check if job is in active state (execution already started but not yet completed).
    *
    * @return true if job is in active state
    */
   public boolean isActive()
   {
      return (status != PackageDeploymentStatus.SCHEDULED) && (status != PackageDeploymentStatus.COMPLETED) && (status != PackageDeploymentStatus.FAILED) &&
            (status != PackageDeploymentStatus.CANCELLED);
   }

   /**
    * Check if job is in finished state.
    *
    * @return true if job is in finished state
    */
   public boolean isFinished()
   {
      return (status == PackageDeploymentStatus.COMPLETED) || (status == PackageDeploymentStatus.FAILED) || (status == PackageDeploymentStatus.CANCELLED);
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the executionTime
    */
   public Date getExecutionTime()
   {
      return executionTime;
   }

   /**
    * @return the completionTime
    */
   public Date getCompletionTime()
   {
      return completionTime;
   }

   /**
    * @return the packageId
    */
   public long getPackageId()
   {
      return packageId;
   }

   /**
    * @return the packageType
    */
   public String getPackageType()
   {
      return packageType;
   }

   /**
    * @return the packageName
    */
   public String getPackageName()
   {
      return packageName;
   }

   /**
    * @return the platform
    */
   public String getPlatform()
   {
      return platform;
   }

   /**
    * @return the version
    */
   public String getVersion()
   {
      return version;
   }

   /**
    * @return the packageFile
    */
   public String getPackageFile()
   {
      return packageFile;
   }

   /**
    * @return the command
    */
   public String getCommand()
   {
      return command;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return the errorMessage
    */
   public String getErrorMessage()
   {
      return errorMessage;
   }
}
