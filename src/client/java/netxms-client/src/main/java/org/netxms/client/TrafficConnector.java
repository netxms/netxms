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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;

/**
 * Traffic connector registered on the server, with the credential fields it expects
 */
public class TrafficConnector
{
   private String name;
   private List<TrafficCredentialField> credentialFields;

   /**
    * Create connector descriptor from NXCP message. Record layout: baseId = name,
    * +1 = credential field count, credential fields start at baseId + 0x10 with stride 0x10.
    *
    * @param msg NXCP message
    * @param baseId base field ID of this record
    */
   public TrafficConnector(final NXCPMessage msg, final long baseId)
   {
      name = msg.getFieldAsString(baseId);
      int count = msg.getFieldAsInt32(baseId + 1);
      credentialFields = new ArrayList<TrafficCredentialField>(count);
      long fieldId = baseId + 0x10;
      for(int i = 0; i < count; i++, fieldId += 0x10)
         credentialFields.add(new TrafficCredentialField(msg, fieldId));
   }

   /**
    * @return connector name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return credential field descriptors (empty if connector takes free-form JSON)
    */
   public List<TrafficCredentialField> getCredentialFields()
   {
      return credentialFields;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return name;
   }
}
