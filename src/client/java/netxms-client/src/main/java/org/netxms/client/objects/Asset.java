/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.Date;
import java.util.Map;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Asset object
 */
public class Asset extends GenericObject
{
   private long linkedObjectId;
   private Date lastUpdateTimestamp;
   private int lastUpdateUserId;
   private Map<String, String> properties;

	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public Asset(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);

      linkedObjectId = msg.getFieldAsInt64(NXCPCodes.VID_LINKED_OBJECT);
      lastUpdateTimestamp = msg.getFieldAsDate(NXCPCodes.VID_LAST_UPDATE_TIMESTAMP);
      lastUpdateUserId = msg.getFieldAsInt32(NXCPCodes.VID_LAST_UPDATE_UID);
      properties = msg.getStringMapFromFields(NXCPCodes.VID_ASSET_PROPERTIES_BASE, NXCPCodes.VID_NUM_ASSET_PROPERTIES);
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
	@Override
	public String getObjectClassName()
	{
      return "Asset";
	}

   /**
    * @return the linkedObjectId
    */
   public long getLinkedObjectId()
   {
      return linkedObjectId;
   }

   /**
    * @return the lastUpdateTimestamp
    */
   public Date getLastUpdateTimestamp()
   {
      return lastUpdateTimestamp;
   }

   /**
    * @return the lastUpdateUserId
    */
   public int getLastUpdateUserId()
   {
      return lastUpdateUserId;
   }

   /**
    * @return the properties
    */
   public Map<String, String> getProperties()
   {
      return properties;
   }
}
