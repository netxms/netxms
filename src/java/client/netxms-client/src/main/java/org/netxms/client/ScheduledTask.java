package org.netxms.client;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

public class ScheduledTask
{
   private long id;
   private String scheduledTaskId;
   private String schedule;
   private String parameters;
   private Date executionTime;
   private Date lastExecutionTime;
   private int flags;
   private long owner;
   private long objectId;
   
   final static public int DISABLED = 1;
   final static public int EXECUTED = 2;
   final static public int IN_PROGRES = 4;
   final static public int INTERNAL = 8;
   final static private String statusDescription[] = {"Disabled", "Completed", "Running"};
   
   public ScheduledTask()
   {
      id = 0;
      scheduledTaskId = "";
      schedule = "";
      parameters = "";
      executionTime = new Date();
      lastExecutionTime = new Date();
      flags = 0;
      owner = 0;
      objectId = 0;
   }
   
   public ScheduledTask(final NXCPMessage msg, long base)
   {
      id = msg.getFieldAsInt64(base);
      scheduledTaskId = msg.getFieldAsString(base+1);
      schedule = msg.getFieldAsString(base+2);
      parameters = msg.getFieldAsString(base+3);
      executionTime = msg.getFieldAsDate(base+4);
      lastExecutionTime = msg.getFieldAsDate(base+5);
      flags = msg.getFieldAsInt32(base+6);
      owner = msg.getFieldAsInt64(base+7);
      objectId = msg.getFieldAsInt64(base+8);
   }
   
   public ScheduledTask(String scheduledTaskId, String schedule, String parameters,
         Date executionTime, int flags, long objectId)
   {
      id = 0;
      this.scheduledTaskId = scheduledTaskId;
      this.schedule = schedule;
      this.parameters = parameters;
      this.executionTime = executionTime;
      lastExecutionTime = new Date(0);
      this.flags = flags;
      owner = 0;
      this.objectId = objectId;
   }
   
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_SCHEDULED_TASK_ID, (int)id);
      msg.setField(NXCPCodes.VID_TASK_HANDLER, scheduledTaskId);
      if(schedule.isEmpty())
         msg.setField(NXCPCodes.VID_EXECUTION_TIME, executionTime);
      else
         msg.setField(NXCPCodes.VID_SCHEDULE, schedule);
      msg.setField(NXCPCodes.VID_PARAMETER, parameters);
      msg.setField(NXCPCodes.VID_LAST_EXECUTION_TIME, lastExecutionTime);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)objectId);    
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
    * @return the scheduledTaskId
    */
   public String getScheduledTaskId()
   {
      return scheduledTaskId;
   }

   /**
    * @param scheduledTaskId the scheduledTaskId to set
    */
   public void setScheduledTaskId(String scheduledTaskId)
   {
      this.scheduledTaskId = scheduledTaskId;
   }

   /**
    * @return the schedule
    */
   public String getSchedule()
   {
      return schedule;
   }

   /**
    * @param schedule the schedule to set
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
   public long getOwner()
   {
      return owner;
   }

   /**
    * @param owner the owner to set
    */
   public void setOwner(long owner)
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
   
   public String getStatus()
   {
      if((flags & IN_PROGRES) > 0)
         return statusDescription[2];
      if((flags & EXECUTED) > 0)
         return statusDescription[1];
      if((flags & DISABLED) > 0)
         return statusDescription[0];
      return "";
   }
   
   public boolean isDisbaled()
   {
      return (flags & DISABLED) > 0;
   }

   public void setEnabed(boolean enabled)
   {
      if(enabled)
         flags &= ~DISABLED;
      else
         flags |= DISABLED;   
   }
}
