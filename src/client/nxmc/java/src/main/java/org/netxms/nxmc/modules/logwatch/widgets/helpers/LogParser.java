/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.logwatch.widgets.helpers;

import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.xml.XMLTools;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.ElementMap;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;

/**
 * Log parser configuration
 */
@Root(name = "parser", strict = false)
public class LogParser
{   
   @Attribute(required = false)
	private String processALL = null;

   @Attribute(required = false)
   private Integer checkInterval = null;

   @Attribute(required = false)
   private String name = null;

   @ElementMap(entry = "macro", key = "name", attribute = true, required = false)
   private HashMap<String, String> macros = new HashMap<String, String>(0);

   @ElementList(required = false, entry = "file", inline = true)
   private ArrayList<LogParserFile> file = new ArrayList<LogParserFile>(0);

   @ElementList(required = false)
   private ArrayList<LogParserRule> rules = new ArrayList<LogParserRule>(0);

	private LogParserType type;

   /**
	 * Create log parser object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static LogParser createFromXml(final String xml) throws Exception
	{
      return ((xml == null) || xml.isEmpty()) ? new LogParser() : XMLTools.createFromXml(LogParser.class, xml);
	}

	/**
	 * Create XML from configuration.
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
	 */
	public String createXml() throws Exception
	{
      Serializer serializer = XMLTools.createSerializer();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}
	
	/**
    * @return the isSyslogParser
    */
   public LogParserType getParserType()
   {
      return type;
   }

   /**
    * @param parserType the type of parser: policy, syslog, win_event
    */
   public void setSyslogParser(LogParserType parserType)
   {
      type = parserType;
      if (type != LogParserType.POLICY)
      {
         file.clear();
      }

      for(LogParserRule rule : rules)
      {
         rule.fixFieldsForType(type);
      }
   }
   
   /**
    * @return filename
    */
   public ArrayList<LogParserFile> getFiles()
   {
      return file;
   }
   
   /**
    * @param filename
    */
   public void setFiles(ArrayList<LogParserFile> files)
   {
      this.file = files;
   }

   /**
    * Get file check interval
    *
    * @return file check interval
    */
   public Integer getFileCheckInterval()
	{
      return checkInterval;
	}

	/**
    * Set file check interval
    *
    * @param checkInterval new file check interval
    */
   public void setFileCheckInterval(Integer checkInterval)
	{
      this.checkInterval = checkInterval;
	}

	/**
    * @return the processALL
    */
   public boolean getProcessALL()
   {
      return stringToBoolean(processALL);
   }

   /**
    * @param processALL the processALL to set
    */
   public void setProcessALL(boolean processALL)
   {
      this.processALL = booleanToString(processALL);
   }
   
	/**
	 * @return
	 */
	public List<LogParserRule> getRules()
	{
		return rules;
	}

	/**
	 * @return
	 */
	public Map<String, String> getMacros()
	{
		return macros;
	}
	
	/**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * Convert string attribute value to boolean
    * 
    * @param value string value
    * @return boolean value
    */
   public static boolean stringToBoolean(final String value)
	{
	   if (value != null)
      {
	      try
	      {
	         if (Integer.parseInt(value) != 0)
	         {
	            return true;
	         }
	      }
         catch(Exception e)
         {
            if (value.equalsIgnoreCase("true") || value.equalsIgnoreCase("yes"))
               return true;
         }
      }
      return false;
	}
	
	/**
	 * Convert boolean to string value for attribute
	 * 
	 * @param value boolean value
	 * @return corresponding string
	 */
	public static String booleanToString(boolean value)
	{
	   return value ? "true" : null;
	}
}
