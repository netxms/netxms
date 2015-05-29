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

/**
 * Base class for Java agent plugins
 */
public abstract class Plugin
{
   /**
    * Constructor used by PluginManager
    * 
    * @param config
    */
   public Plugin(Config config)
   {
   }

   /**
    * Get plugin name
    * 
    * @return
    */
   public abstract String getName();

   /**
    * Get plugin version
    * 
    * @return
    */
   public abstract String getVersion();

   /**
    * Initialize plugin
    * 
    * @param config
    */
   public void init(Config config)
   {
   }

   /**
    * Shutdown plugin
    */
   public void shutdown()
   {
   }

   /**
    * Get list of supported parameters
    * 
    * @return
    */
   public Parameter[] getParameters()
   {
      return new Parameter[0];
   }

   /**
    * Get list of supported push parameters
    * 
    * @return
    */
   public PushParameter[] getPushParameters()
   {
      return new PushParameter[0];
   }

   /**
    * Get list of supported lists
    * 
    * @return
    */
   public ListParameter[] getListParameters()
   {
      return new ListParameter[0];
   }

   /**
    * Get list of supported tables
    * 
    * @return
    */
   public TableParameter[] getTableParameters()
   {
      return new TableParameter[0];
   }

   /**
    * Get list of supported actions
    * 
    * @return
    */
   public Action[] getActions()
   {
      return new Action[0];
   }
}
