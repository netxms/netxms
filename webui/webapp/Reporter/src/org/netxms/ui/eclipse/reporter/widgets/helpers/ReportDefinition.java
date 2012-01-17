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

import java.util.List;

import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Jasper report definition 
 */
@Root(name="jasperReport", strict=false)
public class ReportDefinition
{
	@ElementList(inline=true)
	private List<ReportParameter> parameters;

	/**
	 * Create report definition object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static ReportDefinition createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(ReportDefinition.class, xml.replaceAll("class=\"([a-zA-Z0-9\\._]+)\"", "javaClass=\"$1\""));
	}

	/**
	 * @return the parameters
	 */
	public List<ReportParameter> getParameters()
	{
		return parameters;
	}
}
