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
package org.netxms.nxmc.modules.dashboards.config;

/**
 * Common interface for configurations of dashboard elements that take their data from server-side script.
 */
public interface ScriptedElementConfig
{
   /**
    * @return script to be executed on server
    */
   public String getScript();

   /**
    * @param script script to be executed on server
    */
   public void setScript(String script);

   /**
    * @return ID of script execution context object
    */
   public long getObjectId();

   /**
    * @param objectId ID of script execution context object
    */
   public void setObjectId(long objectId);
}
