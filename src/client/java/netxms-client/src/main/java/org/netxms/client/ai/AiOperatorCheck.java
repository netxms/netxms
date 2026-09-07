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
package org.netxms.client.ai;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.AiCheckAction;
import org.netxms.client.constants.AiCheckVerdict;

/**
 * Standing check of an AI operator instance - NXSL script run by the server on schedule without LLM
 * involvement. Configuration fields have setters; run state fields are read-only.
 */
public class AiOperatorCheck
{
   private int id;
   private int instanceId;
   private String name;
   private String description;
   private boolean enabled;
   private boolean locked;
   private boolean createdByModel;
   private String source;
   private int interval;
   private long objectId;
   private AiCheckAction action;
   private int cooldown;
   private int renotifyInterval;
   private Date lastRun;
   private AiCheckVerdict lastVerdict;
   private Date lastFire;
   private String lastPayload;
   private int consecutiveErrors;
   private int runCount;
   private Date creationTime;
   private Date modificationTime;
   private String compileError;

   /**
    * Create new standing check definition with default settings (for subsequent creation on server).
    *
    * @param instanceId owning AI operator instance ID
    * @param name check name
    */
   public AiOperatorCheck(int instanceId, String name)
   {
      id = 0;
      this.instanceId = instanceId;
      this.name = name;
      description = "";
      enabled = true;
      locked = false;
      createdByModel = false;
      source = "";
      interval = 300;
      objectId = 0;
      action = AiCheckAction.WAKE;
      cooldown = 0;
      renotifyInterval = 0;
      lastRun = null;
      lastVerdict = AiCheckVerdict.NONE;
      lastFire = null;
      lastPayload = "";
      consecutiveErrors = 0;
      runCount = 0;
      creationTime = null;
      modificationTime = null;
      compileError = "";
   }

   /**
    * Create standing check object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base ID for fields
    */
   public AiOperatorCheck(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      instanceId = msg.getFieldAsInt32(baseId + 1);
      name = msg.getFieldAsString(baseId + 2);
      description = msg.getFieldAsString(baseId + 3);
      enabled = msg.getFieldAsBoolean(baseId + 4);
      locked = msg.getFieldAsBoolean(baseId + 5);
      createdByModel = msg.getFieldAsBoolean(baseId + 6);
      source = msg.getFieldAsString(baseId + 7);
      interval = msg.getFieldAsInt32(baseId + 8);
      objectId = msg.getFieldAsInt64(baseId + 9);
      action = AiCheckAction.getByValue(msg.getFieldAsInt32(baseId + 10));
      cooldown = msg.getFieldAsInt32(baseId + 11);
      renotifyInterval = msg.getFieldAsInt32(baseId + 12);
      lastRun = msg.getFieldAsDate(baseId + 13);
      lastVerdict = AiCheckVerdict.getByValue(msg.getFieldAsInt32(baseId + 14));
      lastFire = msg.getFieldAsDate(baseId + 15);
      lastPayload = msg.getFieldAsString(baseId + 16);
      consecutiveErrors = msg.getFieldAsInt32(baseId + 17);
      runCount = msg.getFieldAsInt32(baseId + 18);
      creationTime = msg.getFieldAsDate(baseId + 19);
      modificationTime = msg.getFieldAsDate(baseId + 20);
      compileError = msg.getFieldAsString(baseId + 21);
   }

   /**
    * Fill NXCP message with configuration data for create/modify request.
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldUInt32(NXCPCodes.VID_AI_OPERATOR_ID, instanceId);
      msg.setFieldUInt32(NXCPCodes.VID_CHECK_ID, id);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_ENABLED, enabled);
      msg.setField(NXCPCodes.VID_LOCKED, locked);
      msg.setField(NXCPCodes.VID_SCRIPT, source);
      msg.setFieldInt32(NXCPCodes.VID_POLLING_INTERVAL, interval);
      msg.setFieldUInt32(NXCPCodes.VID_OBJECT_ID, objectId);
      msg.setFieldInt32(NXCPCodes.VID_ACTION_TYPE, action.getValue());
      msg.setFieldInt32(NXCPCodes.VID_COOLDOWN, cooldown);
      msg.setFieldInt32(NXCPCodes.VID_RENOTIFY_INTERVAL, renotifyInterval);
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return owning AI operator instance ID
    */
   public int getInstanceId()
   {
      return instanceId;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @return true if the check is scheduled
    */
   public boolean isEnabled()
   {
      return enabled;
   }

   /**
    * @param enabled true to schedule the check
    */
   public void setEnabled(boolean enabled)
   {
      this.enabled = enabled;
   }

   /**
    * @return true if the check is locked against modification by the operator
    */
   public boolean isLocked()
   {
      return locked;
   }

   /**
    * @param locked true to lock the check against modification by the operator
    */
   public void setLocked(boolean locked)
   {
      this.locked = locked;
   }

   /**
    * @return true if the check was created by the operator itself
    */
   public boolean isCreatedByModel()
   {
      return createdByModel;
   }

   /**
    * @return NXSL source code
    */
   public String getSource()
   {
      return source;
   }

   /**
    * @param source NXSL source code to set
    */
   public void setSource(String source)
   {
      this.source = source;
   }

   /**
    * @return run interval in seconds
    */
   public int getInterval()
   {
      return interval;
   }

   /**
    * @param interval run interval in seconds (minimum 30)
    */
   public void setInterval(int interval)
   {
      this.interval = interval;
   }

   /**
    * @return ID of object bound as $object (0 = none)
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @param objectId ID of object to bind as $object (0 = none)
    */
   public void setObjectId(long objectId)
   {
      this.objectId = objectId;
   }

   /**
    * @return action applied when the check fires
    */
   public AiCheckAction getAction()
   {
      return action;
   }

   /**
    * @param action action to apply when the check fires
    */
   public void setAction(AiCheckAction action)
   {
      this.action = action;
   }

   /**
    * @return minimum seconds between two action applications
    */
   public int getCooldown()
   {
      return cooldown;
   }

   /**
    * @param cooldown minimum seconds between two action applications
    */
   public void setCooldown(int cooldown)
   {
      this.cooldown = cooldown;
   }

   /**
    * @return seconds after which the action is applied again while the check stays fired (0 = edge only)
    */
   public int getRenotifyInterval()
   {
      return renotifyInterval;
   }

   /**
    * @param renotifyInterval seconds after which the action is applied again while the check stays fired (0 = edge only)
    */
   public void setRenotifyInterval(int renotifyInterval)
   {
      this.renotifyInterval = renotifyInterval;
   }

   /**
    * @return time of the last run (null if never run)
    */
   public Date getLastRun()
   {
      return lastRun;
   }

   /**
    * @return verdict of the last run
    */
   public AiCheckVerdict getLastVerdict()
   {
      return lastVerdict;
   }

   /**
    * @return time the check last fired (null if never)
    */
   public Date getLastFire()
   {
      return lastFire;
   }

   /**
    * @return JSON payload of the last fired run (title, severity, details) or error of the last failed run
    */
   public String getLastPayload()
   {
      return lastPayload;
   }

   /**
    * @return number of consecutive failed runs
    */
   public int getConsecutiveErrors()
   {
      return consecutiveErrors;
   }

   /**
    * @return total number of runs
    */
   public int getRunCount()
   {
      return runCount;
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the modificationTime
    */
   public Date getModificationTime()
   {
      return modificationTime;
   }

   /**
    * @return compilation diagnostic if the stored source no longer compiles (empty otherwise)
    */
   public String getCompileError()
   {
      return compileError;
   }
}
