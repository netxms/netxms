/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.util.HashMap;
import java.util.Map;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementMap;
import org.simpleframework.xml.Root;

/**
 * Report's parameter
 */
@Root(name="parameter", strict=false)
public class ReportParameter
{
	public static final int INTEGER = 0;
	public static final int STRING = 1;
	public static final int OBJECT_ID = 2;
	public static final int USER_ID = 3;
	public static final int OBJECT_LIST = 4;
	public static final int TIMESTAMP = 5;
	public static final int START_DATE = 6;
	public static final int END_DATE = 7;
	
	private static final Map<String, Integer> TYPE_MAP;
	
	static
	{
		TYPE_MAP = new HashMap<String, Integer>();
		TYPE_MAP.put("java.lang.Integer", INTEGER);
		TYPE_MAP.put("java.lang.String", STRING);
		TYPE_MAP.put("java.util.Date", TIMESTAMP);
		TYPE_MAP.put("timestamp", TIMESTAMP);
		TYPE_MAP.put("startDate", START_DATE);
		TYPE_MAP.put("endDate", END_DATE);
		TYPE_MAP.put("object", OBJECT_ID);
		TYPE_MAP.put("objectList", OBJECT_LIST);
		TYPE_MAP.put("user", USER_ID);
	}
	
	@Attribute
	private String name;

	@Attribute
	private String javaClass;
	
	@Element(name="parameterDescription", required=false)
	private String description;
	
	@Element(name="defaultValueExpression", required=false)
	private String defaultValue;
	
	@ElementMap(entry="property", key="name", value="value", inline=true, attribute=true)
	private Map<String, String> properties;
	
	/**
	 * Get display name for parameter
	 * 
	 * @return
	 */
	public String getDisplayName()
	{
		String value = properties.get("netxms.displayName");
		return (value != null) ? value : description;
	}

	/**
	 * Get data type for parameter
	 * 
	 * @return
	 */
	public int getDataType()
	{
		String nxtype = properties.get("netxms.type");
		Integer code = TYPE_MAP.get((nxtype != null) ? nxtype : javaClass);
		return (code != null) ? code : STRING;
	}

	/**
	 * Get column span for parameter
	 * 
	 * @return
	 */
	public int getColumnSpan()
	{
		String value = properties.get("netxms.layout.columnSpan");
		return (value != null) ? Integer.parseInt(value) : 1;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return (description != null) ? description : name;
	}

	/**
	 * @return the defaultValue
	 */
	public String getDefaultValue()
	{
		return defaultValue;
	}

	/**
	 * @return the javaClass
	 */
	public String getJavaClass()
	{
		return javaClass;
	}

	/**
	 * @return the properties
	 */
	public Map<String, String> getProperties()
	{
		return properties;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
