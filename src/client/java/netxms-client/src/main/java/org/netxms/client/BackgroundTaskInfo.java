/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.BackgroundTaskState;

/**
 * State information for server-side background task
 */
public class BackgroundTaskInfo
{
   private BackgroundTaskState state;
   private int progress;
   private String description;
   private String failureReason;

   /**
    * Create task information from NXCP message.
    *
    * @param msg NXCP message
    */
   public BackgroundTaskInfo(NXCPMessage msg)
   {
      state = BackgroundTaskState.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_STATE));
      progress = msg.getFieldAsInt32(NXCPCodes.VID_PROGRESS);
      description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
      failureReason = msg.getFieldAsString(NXCPCodes.VID_FAILURE_REASON);
   }

   /**
    * Get task state.
    *
    * @return task state
    */
   public BackgroundTaskState getState()
   {
      return state;
   }

   /**
    * Get task progress in percents (0-100).
    *
    * @return task progress in percents
    */
   public int getProgress()
   {
      return progress;
   }

   /**
    * Get task description.
    *
    * @return task description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Get failure reason for failed task.
    *
    * @return failure reason (empty string if task has not failed)
    */
   public String getFailureReason()
   {
      return failureReason;
   }

   /**
    * Check if task is finished (either successfully or not).
    *
    * @return true if task is finished
    */
   public boolean isFinished()
   {
      return (state == BackgroundTaskState.COMPLETED) || (state == BackgroundTaskState.FAILED);
   }

   /**
    * Check if task has failed.
    *
    * @return true if task has failed
    */
   public boolean isFailed()
   {
      return state == BackgroundTaskState.FAILED;
   }
}
