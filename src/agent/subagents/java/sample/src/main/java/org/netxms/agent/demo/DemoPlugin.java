/**
 * Java-Bridge NetXMS subagent - demo plugin
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
package org.netxms.agent.demo;

import org.netxms.agent.Action;
import org.netxms.agent.Config;
import org.netxms.agent.ListParameter;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
import org.netxms.agent.Plugin;
import org.netxms.agent.PushParameter;
import org.netxms.agent.SubAgent;
import org.netxms.agent.TableColumn;
import org.netxms.agent.TableParameter;
import org.netxms.agent.adapters.ListParameterAdapter;
import org.netxms.agent.adapters.ParameterAdapter;
import org.netxms.agent.adapters.PushParameterAdapter;
import org.netxms.agent.adapters.TableAdapter;

/**
 * Demo plugin
 */
public class DemoPlugin extends Plugin
{
   String configValue;
   
   /**
    * Constructor
    * 
    * @param config
    */
   public DemoPlugin(Config config)
   {
      super(config);
      configValue = config.getValue("/DEMO/EchoValue", "ERROR");
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getName()
    */
   @Override
   public String getName()
   {
      return "DEMO";
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getVersion()
    */
   @Override
   public String getVersion()
   {
      return "1.0";
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#init(org.netxms.agent.Config)
    */
   @Override
   public void init(Config config)
   {
      SubAgent.writeDebugLog(1, "DEMO: initialize: " + config.getValue("/DEMO/EchoValue", "ERROR"));
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#shutdown()
    */
   @Override
   public void shutdown()
   {
      SubAgent.writeDebugLog(1, "DEMO: shutdown called");
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getParameters()
    */
   @Override
   public Parameter[] getParameters()
   {
      return new Parameter[] { 
         new ParameterAdapter("Demo.Parameter1", "Parameter1 description", ParameterType.INT) {
            public String getValue(String argument)
            {
               return "1";
            }
         }, 
         new Parameter() {
            public String getName()
            {
               return "Demo.Parameter2";
            }
   
            public String getDescription()
            {
               return "Parameter2 description";
            }
   
            public ParameterType getType()
            {
               return ParameterType.INT64;
            }
   
            public String getValue(String argument)
            {
               return "2";
            }
         }, 
         new Parameter() {
            public String getName()
            {
               return "Demo.Parameter3";
            }
   
            public String getDescription()
            {
               return "Parameter3 description";
            }
   
            public ParameterType getType()
            {
               return ParameterType.STRING;
            }
   
            public String getValue(String argument)
            {
               return "String";
            }
         }, 
         new Parameter() {
            public String getName()
            {
               return "Demo.Config";
            }
   
            public String getDescription()
            {
               return "Echo config";
            }
   
            public ParameterType getType()
            {
               return ParameterType.STRING;
            }
   
            public String getValue(String argument)
            {
               return configValue;
            }
         },
         new ParameterAdapter("Demo.Error", "Always returns error", ParameterType.INT) {
            public String getValue(String argument)
            {
               return new String[] { "abc" }[1];
            }
         }, 
      };
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getPushParameters()
    */
   @Override
   public PushParameter[] getPushParameters()
   {
      return new PushParameter[] { 
         new PushParameterAdapter("PushParameter1", "PushParameter1 description", ParameterType.INT)
      };
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getListParameters()
    */
   @Override
   public ListParameter[] getListParameters()
   {
      return new ListParameter[] { 
         new ListParameterAdapter("ListParameter1", "Test list") {
            @Override
            public String[] getValue(String value)
            {
               return new String[] { "1", "2" };
            }
         } 
      };
   }

   /* (non-Javadoc)
    * @see org.netxms.agent.Plugin#getTableParameters()
    */
   @Override
   public TableParameter[] getTableParameters()
   {
      return new TableParameter[] { 
         new TableParameter() {
            @Override
            public String getName()
            {
               return "TableParameter1";
            }
   
            @Override
            public String getDescription()
            {
               return "TableParameter1 description";
            }
   
            @Override
            public TableColumn[] getColumns()
            {
               return new TableColumn[] { new TableColumn("Column1", ParameterType.STRING, true),
                     new TableColumn("Column2", ParameterType.INT, false) };
            }
   
            @Override
            public String[][] getValue(String parameter)
            {
               return new String[][] { { "Row1", "1" }, { "Row2", "2" } };
            }
         },
         new TableAdapter("TableParameter2", "TableParameter2 description", 
               new TableColumn[] { new TableColumn("Column1", ParameterType.STRING, true),
               new TableColumn("Column2", ParameterType.INT, false) }) {
            @Override
            public String[][] getValue(String parameter)
            {
               return new String[][] { { "Row1", "1" }, { "Row2", "2" } };
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
      return new Action[] { new Action() {
         @Override
         public String getName()
         {
            return "Action1";
         }

         @Override
         public String getDescription()
         {
            return "Action1 description";
         }

         @Override
         public boolean execute(String action, String[] args)
         {
            SubAgent.writeDebugLog(1, "DEMO ACTION EXECUTED");
            return true;
         }
      } };
   }
}
