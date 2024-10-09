/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.client.topology;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.NetworkPathFailureReason;

/**
 * Result of network path check
 */
public class NetworkPathCheckResult
{
   private NetworkPathFailureReason reason;
   private long rootCauseNodeId;
   private long rootCauseInterfaceId;

   /**
    * Create from NXCP message
    *
    * @param msg NXCP message
    */
   public NetworkPathCheckResult(NXCPMessage msg)
   {
      reason = NetworkPathFailureReason.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_PATH_CHECK_REASON));
      rootCauseNodeId = msg.getFieldAsInt64(NXCPCodes.VID_PATH_CHECK_NODE_ID);
      rootCauseInterfaceId = msg.getFieldAsInt64(NXCPCodes.VID_PATH_CHECK_INTERFACE_ID);
   }

   /**
    * @return the reason
    */
   public NetworkPathFailureReason getReason()
   {
      return reason;
   }

   /**
    * @return the rootCauseNodeId
    */
   public long getRootCauseNodeId()
   {
      return rootCauseNodeId;
   }

   /**
    * @return the rootCauseInterfaceId
    */
   public long getRootCauseInterfaceId()
   {
      return rootCauseInterfaceId;
   }
}
