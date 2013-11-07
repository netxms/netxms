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
	@Attribute(required=false)
	private String name = null;
	
	@Attribute(required=false)
	private Integer trace = null;
	
	@Element(required=false)
	private String file = null;
	
	@ElementList(required=false)
	private ArrayList<LogParserRule> rules = new ArrayList<LogParserRule>(0);
	
	@ElementMap(entry="macro", key="name", attribute=true, required=false)
	private HashMap<String, String> macros = new HashMap<String, String>(0);

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
	 * @return
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return
	 */
	public int getTrace()
	{
		return trace;
	}

	/**
	 * @param trace
	 */
	public void setTrace(int trace)
	{
		this.trace = trace;
	}

	/**
	 * @return
	 */
	public String getFile()
	{
		return file;
	}

	/**
	 * @param file
	 */
	public void setFile(String file)
	{
		this.file = file;
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
}
