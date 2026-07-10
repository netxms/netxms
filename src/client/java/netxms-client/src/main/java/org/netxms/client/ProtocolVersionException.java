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

import org.netxms.client.constants.RCC;

/**
 * Thrown when server's communication protocol version does not match client's one. Carries server's product version in addition to
 * raw protocol versions, so that user interface can suggest a matching client.
 */
public class ProtocolVersionException extends NXCException
{
   private static final long serialVersionUID = 3168442598861516097L;

   private ProtocolVersion protocolVersion;
   private String serverVersion;
   private String serverBuild;

   /**
    * Create new protocol version exception.
    *
    * @param protocolVersion protocol versions reported by server
    * @param serverVersion server's product version
    * @param serverBuild server's build tag (can be null)
    */
   public ProtocolVersionException(ProtocolVersion protocolVersion, String serverVersion, String serverBuild)
   {
      super(RCC.BAD_PROTOCOL, protocolVersion.toString());
      this.protocolVersion = protocolVersion;
      this.serverVersion = serverVersion;
      this.serverBuild = serverBuild;
   }

   /**
    * Get protocol versions reported by server.
    *
    * @return protocol versions reported by server
    */
   public ProtocolVersion getProtocolVersion()
   {
      return protocolVersion;
   }

   /**
    * Get server's product version.
    *
    * @return server's product version
    */
   public String getServerVersion()
   {
      return serverVersion;
   }

   /**
    * Get server's build tag.
    *
    * @return server's build tag or null if not provided by server
    */
   public String getServerBuild()
   {
      return serverBuild;
   }
}
