/**
 * Java-Bridge NetXMS subagent
 * Copyright (C) 2013 TEMPEST a.s.
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
package org.netxms.agent;

import org.netxms.bridge.Config;
import org.netxms.bridge.Platform;

/**
 * Base class for Java agent plugins
 */
public abstract class Plugin
{
   /**
    * Constructor used by PluginManager
    * 
    * @param config agent configuration
    */
   public Plugin(Config config)
   {
   }

   /**
    * Get plugin name
    * 
    * @return plugin name
    */
   public abstract String getName();

   /**
    * Get plugin version
    * 
    * @return plugin version
    */
   public abstract String getVersion();

   /**
    * Initialize plugin
    * 
    * @param config agent configuration
    * @throws PluginInitException when plugin initialization failed
    */
   public void init(Config config) throws PluginInitException
   {
      Platform.writeDebugLog(6, "JAVA/" + getName() + ": initializing");
   }

   /**
    * Shutdown plugin
    */
   public void shutdown()
   {
      Platform.writeDebugLog(6, "JAVA/" + getName() + ": shutdown called");
   }

   /**
    * Get list of supported parameters
    * 
    * @return list of supported parameters
    */
   public Parameter[] getParameters()
   {
      return new Parameter[0];
   }

   /**
    * Get list of supported push parameters
    * 
    * @return list of supported push parameters
    */
   public PushParameter[] getPushParameters()
   {
      return new PushParameter[0];
   }

   /**
    * Get list of supported lists
    * 
    * @return list of supported lists
    */
   public ListParameter[] getListParameters()
   {
      return new ListParameter[0];
   }

   /**
    * Get list of supported tables
    * 
    * @return list of supported tables
    */
   public TableParameter[] getTableParameters()
   {
      return new TableParameter[0];
   }

   /**
    * Get list of supported actions
    * 
    * @return list of supported actions
    */
   public Action[] getActions()
   {
      return new Action[0];
   }
}
