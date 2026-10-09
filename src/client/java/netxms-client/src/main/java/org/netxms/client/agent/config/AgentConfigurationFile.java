/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.client.agent.config;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Agent configuration content (master configuration file or effective configuration) read from node.
 * Content can be read directly from agent or, if agent is not reachable, from server-side cache. Cached
 * content is read-only and carries timestamp of last cache update.
 */
public class AgentConfigurationFile
{
   private String content;
   private boolean cached;
   private Date cacheTime;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    */
   public AgentConfigurationFile(NXCPMessage msg)
   {
      content = msg.getFieldAsString(NXCPCodes.VID_CONFIG_FILE);
      cached = msg.getFieldAsBoolean(NXCPCodes.VID_READ_ONLY);
      cacheTime = cached ? msg.getFieldAsDate(NXCPCodes.VID_TIMESTAMP) : null;
   }

   /**
    * Get configuration content.
    *
    * @return configuration content
    */
   public String getContent()
   {
      return content;
   }

   /**
    * Check if content was served from server-side cache because agent is not reachable.
    *
    * @return true if content was served from server-side cache
    */
   public boolean isCached()
   {
      return cached;
   }

   /**
    * Get time of last cache update. Valid only when content was served from cache.
    *
    * @return time of last cache update or null if content was read directly from agent
    */
   public Date getCacheTime()
   {
      return cacheTime;
   }
}
