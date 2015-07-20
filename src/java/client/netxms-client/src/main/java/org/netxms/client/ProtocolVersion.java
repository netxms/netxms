/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.util.Arrays;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * NetXMS client protocol version
 */
public final class ProtocolVersion
{
   // Versions
   public static final int BASE = 47;
   public static final int ALARMS = 1;
   public static final int PUSH = 1;
   public static final int TRAP = 1;
   public static final int MOBILE = 1;
   public static final int FULL = 2;
   
   // Indexes
   public static final int INDEX_BASE = 0;
   public static final int INDEX_ALARMS = 1;
   public static final int INDEX_PUSH = 2;
   public static final int INDEX_TRAP = 3;
   public static final int INDEX_MOBILE = 4;
   public static final int INDEX_FULL = 5;
   
   private static final int[] CURRENT_VERSION = { BASE, ALARMS, PUSH, TRAP, MOBILE, FULL };
   
   private long versions[];
   
   /**
    * Create protocol version information from NXCP message
    * 
    * @param msg
    */
   protected ProtocolVersion(NXCPMessage msg)
   {
      versions = msg.getFieldAsUInt32Array(NXCPCodes.VID_PROTOCOL_VERSION_EX);
      if (versions == null)
      {
         versions = new long[1];
         versions[0] = msg.getFieldAsInt32(NXCPCodes.VID_PROTOCOL_VERSION);
      }
   }
   
   /**
    * Get version of given component
    * 
    * @param index
    * @return
    */
   public int getVersion(int index)
   {
      return (index >= 0) && (index < versions.length) ? (int)versions[index] : 0;
   }
   
   /**
    * Check if version of given component match current version
    * 
    * @param index
    * @return
    */
   public boolean isCorrectVersion(int index)
   {
      if ((index < 0) || (index >= CURRENT_VERSION.length))
         return false;
      return getVersion(index) == CURRENT_VERSION[index];
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ProtocolVersion [versions=" + Arrays.toString(versions) + "]";
   }
}
