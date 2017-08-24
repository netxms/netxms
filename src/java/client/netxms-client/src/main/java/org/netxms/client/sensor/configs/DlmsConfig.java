/**
 * NetXMS - open source network management system
 * Copyright (C) 2017 Victor Kirhenshtein
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
package org.netxms.client.sensor.configs;

import java.util.ArrayList;
import java.util.List;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;

/**
 * LoRaWAN specific configuration parameters
 */
@Root(name="config")
public class DlmsConfig extends SensorConfig
{
   
   
   @ElementList(inline=true)
   public List<DlmsConverterCredentials> connections;
   
   public DlmsConfig()
   {
      connections = new ArrayList<DlmsConverterCredentials>();
   }
   
   public void addConnection(DlmsConverterCredentials conn)
   {
      connections.add(conn);
   }
}
