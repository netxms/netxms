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
package org.netxms.ui.android.main.activities.helpers;

import java.util.Date;
import org.simpleframework.xml.convert.Converter;
import org.simpleframework.xml.stream.InputNode;
import org.simpleframework.xml.stream.OutputNode;

/**
 * Converter for date serialization/deserialization
 */
public class XmlDateConverter implements Converter<Date>
{
	/* (non-Javadoc)
	 * @see org.simpleframework.xml.convert.Converter#read(org.simpleframework.xml.stream.InputNode)
	 */
	@Override
	public Date read(InputNode node) throws Exception
	{
		long n;
		try
		{
			n = Long.parseLong(node.getValue());
		}
		catch(NumberFormatException e)
		{
			n = System.currentTimeMillis();
		}
		return new Date(n);
	}

	/* (non-Javadoc)
	 * @see org.simpleframework.xml.convert.Converter#write(org.simpleframework.xml.stream.OutputNode, java.lang.Object)
	 */
	@Override
	public void write(OutputNode node, Date object) throws Exception
	{
		node.setValue(Long.toString(object.getTime()));
	}
}
