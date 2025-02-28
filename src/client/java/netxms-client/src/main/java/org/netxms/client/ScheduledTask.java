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
package org.netxms.client;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

public class ScheduledTask
{
   private long id;
   private String taskHandlerId;
   private String key;
   private String schedule;
   private String parameters;
   private String comments;
   private Date executionTime;
   private Date lastExecutionTime;
   private int flags;
   private int owner;
   private long objectId;

   final static public int DISABLED = 1;
   final static public int EXECUTED = 2;
   final static public int RUNNING = 4;
   final static public int SYSTEM = 8;
   final static private String statusDescription[] = { "Disabled", "Completed", "Running" };

   /**
    * Create empty task object
    */
   public ScheduledTask()
   {
      id = 0;
      taskHandlerId = "";
      key = "";
      schedule = "";
      parameters = "";
      executionTime = new Date();
      lastExecutionTime = new Date();
      flags = 0;
      owner = 0;
      objectId = 0;
      comments = "";
   }

   /**
    * Create task object from NXCP message
    * 
    * @param msg NXCP message
    * @param base base field ID
    */
   public ScheduledTask(final NXCPMessage msg, long base)
   {
      id = msg.getFieldAsInt64(base);
      taskHandlerId = msg.getFieldAsString(base + 1);
      schedule = msg.getFieldAsString(base + 2);
      parameters = msg.getFieldAsString(base + 3);
      executionTime = msg.getFieldAsDate(base + 4);
      lastExecutionTime = msg.getFieldAsDate(base + 5);
      flags = msg.getFieldAsInt32(base + 6);
      owner = msg.getFieldAsInt32(base + 7);
      objectId = msg.getFieldAsInt64(base + 8);
      comments = msg.getFieldAsString(base + 9);
      key = msg.getFieldAsString(base + 10);
   }

   /**
    * Create new scheduled task object
    * 
    * @param taskHandlerId scheduled task handler ID
    * @param schedule schedule for recurrent task
    * @param parameters handler-specific parameters
    * @param comments task comments
    * @param executionTime execution time for "run once" tasks
    * @param flags task flags
    * @param objectId ID of NetXMS object associated with this task
    */
   public ScheduledTask(String taskHandlerId, String schedule, String parameters,
         String comments, Date executionTime, int flags, long objectId)
   {
      this.id = 0;
      this.taskHandlerId = taskHandlerId;
      this.key = "";
      this.schedule = schedule;
      this.parameters = parameters;
      this.executionTime = executionTime;
      this.lastExecutionTime = new Date(0);
      this.flags = flags;
      this.owner = 0;
      this.objectId = objectId;
      this.comments = comments;
   }
   
   /**
    * Fill NXCP message with task data
    * 
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt64(NXCPCodes.VID_SCHEDULED_TASK_ID, id);
      msg.setField(NXCPCodes.VID_TASK_HANDLER, taskHandlerId);
      if (schedule.isEmpty())
         msg.setField(NXCPCodes.VID_EXECUTION_TIME, executionTime);
      else
         msg.setField(NXCPCodes.VID_SCHEDULE, schedule);
      msg.setField(NXCPCodes.VID_PARAMETER, parameters);
      msg.setField(NXCPCodes.VID_LAST_EXECUTION_TIME, lastExecutionTime);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      msg.setField(NXCPCodes.VID_TASK_IS_DISABLED, isDisabled());
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);  
      msg.setField(NXCPCodes.VID_COMMENTS, comments);
      msg.setField(NXCPCodes.VID_TASK_KEY, key);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * Get ID of scheduled task handler.
    * 
    * @return scheduled task handler ID
    */
   public String getTaskHandlerId()
   {
      return taskHandlerId;
   }

   /**
    * Set ID of scheduled task handler.
    * 
    * @param taskHandlerId new scheduled task handler ID
    */
   public void setTaskHandlerId(String taskHandlerId)
   {
      this.taskHandlerId = taskHandlerId;
   }

   /**
    * Get recurring task schedule
    * 
    * @return recurring task schedule
    */
   public String getSchedule()
   {
      return schedule;
   }

   /**
    * Set recurring task schedule
    * 
    * @param schedule new recurring task schedule
    */
   public void setSchedule(String schedule)
   {
      this.schedule = schedule;
   }

   /**
    * @return the parameters
    */
   public String getParameters()
   {
      return parameters;
   }

   /**
    * @param parameters the parameters to set
    */
   public void setParameters(String parameters)
   {
      this.parameters = parameters;
   }

   /**
    * @return the executionTime
    */
   public Date getExecutionTime()
   {
      return executionTime;
   }

   /**
    * @param executionTime the executionTime to set
    */
   public void setExecutionTime(Date executionTime)
   {
      this.executionTime = executionTime;
   }

   /**
    * @return the lastExecutionTime
    */
   public Date getLastExecutionTime()
   {
      return lastExecutionTime;
   }

   /**
    * @param lastExecutionTime the lastExecutionTime to set
    */
   public void setLastExecutionTime(Date lastExecutionTime)
   {
      this.lastExecutionTime = lastExecutionTime;
   }

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @param flags the flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }

   /**
    * @return the owner
    */
   public int getOwner()
   {
      return owner;
   }

   /**
    * @param owner the owner to set
    */
   public void setOwner(int owner)
   {
      this.owner = owner;
   }   

   /**
    * @return the objectId
    */
   public long getObjectId()
   {
      return objectId;
   }
   
   /**
    * @param objectId the objectId to set
    */
   public void setObjectId(long objectId)
   {
      this.objectId = objectId;
   } 
   
   /**
    * Get task key
    * 
    * @return task key
    */
   public String getKey()
   {
      return key;
   }

   /**
    * Set task key
    * 
    * @param key new task key
    */
   public void setKey(String key)
   {
      this.key = key;
   }

   /**
    * Get task status
    *
    * @return task status
    */
   public String getStatus()
   {
      if ((flags & RUNNING) != 0)
         return statusDescription[2];
      if ((flags & EXECUTED) != 0)
         return statusDescription[1];
      if ((flags & DISABLED) != 0)
         return statusDescription[0];
      return "";
   }
   
   /**
    * Get task comments
    *
    * @return comments
    */
   public String getComments()
   {
      return comments;
   }
   
   /**
    * Set scheduled task comments
    * 
    * @param comments The comments to set
    */
   public void setComments(String comments)
   {
      this.comments = comments;
   }
   
   /**
    * Check if this task is a system task
    *
    * @return true if this task is a system task
    */
   public boolean isSystem()
   {
      return (flags & SYSTEM) != 0;
   }

   /**
    * Check if this task has completed execution
    *
    * @return true if this task has completed execution
    */
   public boolean isCompleted()
   {
      return (flags & EXECUTED) != 0;
   }

   /**
    * Check if this task is disabled
    *
    * @return true if this task is disabled
    */
   public boolean isDisabled()
   {
      return (flags & DISABLED) != 0;
   }

   /**
    * Check if this task is configured for recurring execution
    *
    * @return true if this task is configured for recurring execution
    */
   public boolean isRecurring()
   {
      return (schedule != null) && !schedule.isEmpty();
   }

   /**
    * Enable/disable task
    *
    * @param enabled true if task should be enabled
    */
   public void setEnabed(boolean enabled)
   {
      if (enabled)
         flags &= ~DISABLED;
      else
         flags |= DISABLED;   
   }
}
