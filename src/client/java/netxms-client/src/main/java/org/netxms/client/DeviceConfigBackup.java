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

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Device configuration backup data
 */
public class DeviceConfigBackup
{
   private long id;
   private Date timestamp;
   private byte[] runningConfig;
   private long runningConfigSize;
   private byte[] runningConfigHash;
   private byte[] startupConfig;
   private long startupConfigSize;
   private byte[] startupConfigHash;
   private boolean isBinary;

   /**
    * Create from NXCP message for list response (metadata only, using base ID offset)
    *
    * @param msg NXCP message
    * @param baseId base field ID for this list entry
    */
   public DeviceConfigBackup(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      timestamp = msg.getFieldAsDate(baseId + 1);
      runningConfigSize = msg.getFieldAsInt64(baseId + 2);
      runningConfigHash = msg.getFieldAsBinary(baseId + 3);
      startupConfigSize = msg.getFieldAsInt64(baseId + 4);
      startupConfigHash = msg.getFieldAsBinary(baseId + 5);
      runningConfig = null;
      startupConfig = null;
      isBinary = false;
   }

   /**
    * Create from NXCP message for single backup response (full content)
    *
    * @param msg NXCP message
    */
   public DeviceConfigBackup(NXCPMessage msg)
   {
      id = msg.getFieldAsInt64(NXCPCodes.VID_BACKUP_ID);
      timestamp = msg.getFieldAsDate(NXCPCodes.VID_TIMESTAMP);
      isBinary = msg.getFieldAsBoolean(NXCPCodes.VID_IS_BINARY);
      runningConfigSize = msg.getFieldAsInt64(NXCPCodes.VID_RUNNING_CONFIG_SIZE);
      runningConfigHash = msg.getFieldAsBinary(NXCPCodes.VID_RUNNING_CONFIG_HASH);
      runningConfig = msg.getFieldAsBinary(NXCPCodes.VID_CONFIG_FILE_DATA);
      startupConfigSize = msg.getFieldAsInt64(NXCPCodes.VID_STARTUP_CONFIG_SIZE);
      startupConfigHash = msg.getFieldAsBinary(NXCPCodes.VID_STARTUP_CONFIG_HASH);
      startupConfig = msg.getFieldAsBinary(NXCPCodes.VID_STARTUP_CONFIG);
   }

   /**
    * @return backup ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return backup timestamp
    */
   public Date getTimestamp()
   {
      return timestamp;
   }

   /**
    * @return running configuration content or null if not available (list query)
    */
   public byte[] getRunningConfig()
   {
      return runningConfig;
   }

   /**
    * @return running configuration size in bytes
    */
   public long getRunningConfigSize()
   {
      return runningConfigSize;
   }

   /**
    * @return running configuration SHA-256 hash
    */
   public byte[] getRunningConfigHash()
   {
      return runningConfigHash;
   }

   /**
    * @return startup configuration content or null if not available (list query)
    */
   public byte[] getStartupConfig()
   {
      return startupConfig;
   }

   /**
    * @return startup configuration size in bytes
    */
   public long getStartupConfigSize()
   {
      return startupConfigSize;
   }

   /**
    * @return startup configuration SHA-256 hash
    */
   public byte[] getStartupConfigHash()
   {
      return startupConfigHash;
   }

   /**
    * @return true if configuration content is binary (not text)
    */
   public boolean isBinary()
   {
      return isBinary;
   }
}
