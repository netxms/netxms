/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets.internal;

import java.io.StringWriter;
import java.io.Writer;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for obejct tool element
 */
public class ObjectToolsConfig extends DashboardElementConfig
{
   @Element(required=false)
   private String title = null;
   
   @ElementArray(required=false)
   private Tool[] tools = null;
   
   @Element(required=false)
   private int numColumns = 1;
   
   /**
    * Create object tool configuration object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static ObjectToolsConfig createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(ObjectToolsConfig.class, xml);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#createXml()
    */
   @Override
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
   /**
    * @return the title
    */
   public String getTitle()
   {
      return (title != null) ? title : "";
   }

   /**
    * @param title the title to set
    */
   public void setTitle(String title)
   {
      this.title = title;
   }

   /**
    * @return the tools
    */
   public Tool[] getTools()
   {
      return tools;
   }

   /**
    * @param tools the tools to set
    */
   public void setTools(Tool[] tools)
   {
      this.tools = tools;
   }

   /**
    * @return the numColumns
    */
   public int getNumColumns()
   {
      return numColumns;
   }

   /**
    * @param numColumns the numColumns to set
    */
   public void setNumColumns(int numColumns)
   {
      this.numColumns = numColumns;
   }

   /**
    * Tool configuration
    */
   @Root(name="tool")
   public static class Tool
   {
      @Element(required=false)
      public long objectId;
      
      @Element(required=false)
      public long toolId;
      
      @Element(required=false)
      public String name;
      
      @Element(required=false)
      public int color = 0;
      
      public Tool()
      {
         objectId = 0;
         toolId = 0;
         name = "";
      }
      
      public Tool(Tool src)
      {
         objectId = src.objectId;
         toolId = src.toolId;
         name = src.name;
      }
   }
}
