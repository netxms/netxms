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

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.FirmwareRole;

/**
 * Information about firmware (or operating system image) running on a device
 */
public class FirmwareInfo
{
   private FirmwareRole role;
   private int componentIndex;
   private String name;
   private String platform;
   private String version;
   private String rawVersion;

   /**
    * Create firmware information from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public FirmwareInfo(NXCPMessage msg, long baseId)
   {
      role = FirmwareRole.getByValue(msg.getFieldAsInt32(baseId));
      componentIndex = msg.getFieldAsInt32(baseId + 1);
      name = msg.getFieldAsString(baseId + 2);
      platform = msg.getFieldAsString(baseId + 3);
      version = msg.getFieldAsString(baseId + 4);
      rawVersion = msg.getFieldAsString(baseId + 5);
   }

   /**
    * Get role of this firmware on the device.
    *
    * @return firmware role
    */
   public FirmwareRole getRole()
   {
      return role;
   }

   /**
    * Get index of device component this firmware belongs to.
    *
    * @return component index or -1 if this is firmware of the device itself
    */
   public int getComponentIndex()
   {
      return componentIndex;
   }

   /**
    * Check if this is main firmware of the device itself (and not of one of its components).
    *
    * @return true if this is main firmware of the device itself
    */
   public boolean isMainDeviceFirmware()
   {
      return (role == FirmwareRole.MAIN) && (componentIndex == -1);
   }

   /**
    * Get display name of this firmware (like "Junos OS" or "RouterBOOT").
    *
    * @return display name
    */
   public String getName()
   {
      return (name != null) ? name : "";
   }

   /**
    * Get firmware platform ID (like "junos" or "routeros"). Versions of the same platform are comparable.
    *
    * @return platform ID or empty string if platform is not known
    */
   public String getPlatform()
   {
      return (platform != null) ? platform : "";
   }

   /**
    * Get firmware version. If platform is known, version is in canonical form of platform's version scheme, otherwise it is
    * the same as raw version.
    *
    * @return firmware version
    */
   public String getVersion()
   {
      return (version != null) ? version : "";
   }

   /**
    * Get firmware version exactly as reported by the device.
    *
    * @return raw firmware version
    */
   public String getRawVersion()
   {
      return (rawVersion != null) ? rawVersion : "";
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "FirmwareInfo [role=" + role + ", componentIndex=" + componentIndex + ", name=" + name + ", platform=" + platform + ", version=" + version +
            ", rawVersion=" + rawVersion + "]";
   }
}
