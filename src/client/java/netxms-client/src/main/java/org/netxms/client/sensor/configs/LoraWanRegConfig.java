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

import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * LoRaWAN specific configuration parameters
 */
@Root(name="config")
public class LoraWanRegConfig extends SensorRegistrationConfig
{
   public static final int OTAA = 0;
   public static final int APB = 1;
   public static final String[] REG_OPTION_NAMES = {"OTAA","APB"};

   @Element(required=true)
   public int registrationType;

   @Element(required=false)
   public String appEUI;

   @Element(required=false)
   public String appKey;

   @Element(required=false)
   public String nwkSKey;

   @Element(required=false)
   public String appSKey;  
   
}
