/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.ElementMap;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Log parser configuration
 */
@Root(name="parser", strict=false)
public class LogParser
{
   public static int PARSER_SYSLOG = 0;
   public static int PARSER_OTHER = 1;
   
	@Attribute(required=false)
	private String processALL = null;
	
	@Attribute(required=false)
	private Integer trace = null;
	
	@Element(required=false)
	private LogParserFile file = new LogParserFile();
   
	@ElementList(required=false)
	private ArrayList<LogParserRule> rules = new ArrayList<LogParserRule>(0);
	
	@ElementMap(entry="macro", key="name", attribute=true, required=false)
	private HashMap<String, String> macros = new HashMap<String, String>(0);
	
	private boolean isSyslogParser;

   /**
	 * Create log parser object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static LogParser createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(LogParser.class, xml);
	}

	/**
	 * Create XML from configuration.
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
	 */
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}
	
	/**
    * @return the isSyslogParser
    */
   public boolean isSyslogParser()
   {
      return isSyslogParser;
   }

   /**
    * @param isSyslogParser the isSyslogParser to set
    */
   public void setSyslogParser(boolean isSyslogParser)
   {
      this.isSyslogParser = isSyslogParser;
      if(isSyslogParser)
      {
         file.setFile(null);
         file.setEncoding(null);
      }

      for(LogParserRule rule : rules)
      {
         rule.updateFieldsCorrectly(isSyslogParser);
      }
   }
   
   /**
    * @return filename
    */
   public String getFile()
   {
      return file.getFile();
   }
   
   /**
    * @param filename
    */
   public void setFile(String file)
   {
      this.file.setFile(file);
   }
   
   /**
    * @return file encoding
    */
   public String getEncoding()
   {
      return file.getEncoding();
   }
   
   /**
    * @param encoding
    */
   public void setEncoding(String encoding)
   {
      file.setEncoding(encoding);
   }

   /**
	 * @return
	 */
	public Integer getTrace()
	{
		return trace;
	}

	/**
	 * @param trace
	 */
	public void setTrace(Integer trace)
	{
		this.trace = trace;
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
	
	public static boolean stringToBoolean(final String value)
	{
	   if(value != null)
      {
	      try
	      {
	         if(Integer.parseInt(value) > 0)
	         {
	            return true;
	         }
	      }
         catch(Exception ex)
         {
            {
               if(value.equalsIgnoreCase("true") || value.equalsIgnoreCase("yes"))
                  return true;
            }
         }
      }
      return false;
	}
	
	public static String booleanToString(boolean value)
	{
	   return value ? "true" : null;
	}
	
	public static String integerToString(Integer value)
	{
	   return value == null ? "" : Integer.toString(value);
	}
}
