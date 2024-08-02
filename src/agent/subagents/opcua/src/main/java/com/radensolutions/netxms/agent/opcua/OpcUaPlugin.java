/**
 * OPC UA subagent
 * Copyright (C) 2017-2021 Raden Solutions
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
package com.radensolutions.netxms.agent.opcua;

import java.util.HashMap;
import java.util.Map;
import org.netxms.agent.Action;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
import org.netxms.agent.Plugin;
import org.netxms.agent.PluginInitException;
import org.netxms.agent.adapters.ActionAdapter;
import org.netxms.agent.adapters.ParameterAdapter;
import org.netxms.bridge.Config;
import org.netxms.bridge.ConfigEntry;
import org.netxms.bridge.Platform;

/**
 * OPC-UA subagent plugin
 */
public class OpcUaPlugin extends Plugin
{
   public static final String DEBUG_TAG = "opcua";

   private Map<String, ServerConnection> servers = new HashMap<String, ServerConnection>();
   private int timeout = 5000;

   /**
    * Create plugin instance
    * 
    * @param config agent configuration object
    */
   public OpcUaPlugin(Config config)
   {
      super(config);
   }

   /**
    * @see org.netxms.agent.Plugin#getName()
    */
   @Override
   public String getName()
   {
      return "OPCUA";
   }

   /**
    * @see org.netxms.agent.Plugin#getVersion()
    */
   @Override
   public String getVersion()
   {
      return "1.0";
   }

   /**
    * @see org.netxms.agent.Plugin#init(org.netxms.agent.Config)
    */
   @Override
   public void init(Config config) throws PluginInitException
   {
      super.init(config);
      
      timeout = config.getValueInt("/OPCUA/Timeout", 5000);
      
      ConfigEntry e = config.getEntry("/OPCUA/Server");
      if (e == null)
         throw new PluginInitException("OPCUA servers not defined");
         
      for(int i = 0; i < e.getValueCount(); i++)
      {
         addServer(e.getValue(i));
      }
   }
   
   /**
    * Add server from config
    * Format is
    *    name:url
    *    name:login@url
    *    name:login/password@url
    * 
    * @param config
    * @throws PluginInitException
    */
   private void addServer(String config) throws PluginInitException
   {
      String[] parts = config.split(":", 2);
      if (parts.length != 2)
         throw new PluginInitException("Invalid server configuration entry \"" + config + "\"");

      ServerConnection s;
      if (parts[1].contains("@"))
      {
         String[] uparts = parts[1].split("@", 2);
         String login;
         String password;
         if (uparts[0].contains("/"))
         {
            String[] lparts = uparts[0].split("/", 2);
            login = lparts[0];
            password = lparts[1];
         }
         else
         {
            login = uparts[0];
            password = "";
         }
         s = new ServerConnection(parts[0].trim(), uparts[1].trim(), login, password, timeout);
      }
      else
      {
         s = new ServerConnection(parts[0].trim(), parts[1].trim(), null, null, timeout);
      }
      servers.put(s.getName(), s);
      Platform.writeDebugLog(DEBUG_TAG, 3, "Added OPC UA server connection " + s.getName() + " (" + s.getUrl() + ")");
   }

   /**
    * Get server by name
    * 
    * @param name
    * @return
    */
   private ServerConnection getServer(String name)
   {
      if ((name == null) || name.isEmpty())
         return null;
      return servers.get(name);
   }

   /**
    * @see org.netxms.agent.Plugin#getParameters()
    */
   @Override
   public Parameter[] getParameters()
   {
      return new Parameter[] {
            new ParameterAdapter("OPCUA.NodeValue(*)", "Value of OPC-UA node {instance}", ParameterType.STRING) {
               @Override
               public String getValue(String param) throws Exception
               {
                  ServerConnection server = getServer(getArgument(param, 0));
                  if (server == null)
                     return null;
                  
                  String node = getArgument(param, 1);
                  if ((node == null) || node.isEmpty())
                     return null;

                  return server.getNodeValue(node);
               }
            }
      };
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getActions()
    */
   @Override
   public Action[] getActions()
   {
      return new Action[] {
            new ActionAdapter("OPCUA.WriteNode", "Write OPC-UA node") {
               @Override
               public boolean execute(String action, String[] args) throws Exception
               {
                  if (args.length < 3)
                     return false;

                  ServerConnection server = getServer(args[0]);
                  if (server == null)
                     return false;
                  
                  return server.writeNode(args[1], args[2]);
               }
            }
      };
   }
}
