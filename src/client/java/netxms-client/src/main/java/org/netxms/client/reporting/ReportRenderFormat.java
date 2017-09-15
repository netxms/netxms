/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.client.reporting;

import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

/**
 * Formats for report rendering
 */
public enum ReportRenderFormat
{
	NONE(0, "none"), PDF(1, "pdf"), XLS(2, "xls");

	private final int code;
	private final String extension;

	private static final Map<Integer, ReportRenderFormat> lookupTable = new HashMap<Integer, ReportRenderFormat>(2);

	static
	{
		for(ReportRenderFormat element : EnumSet.allOf(ReportRenderFormat.class))
		{
			lookupTable.put(element.getCode(), element);
		}
	}

	/**
	 * @param code
	 * @param extenstion
	 */
	ReportRenderFormat(int code, String extenstion)
	{
		this.code = code;
		this.extension = extenstion;
	}

	/**
	 * @return the code
	 */
	public int getCode()
	{
		return code;
	}

	/**
	 * Get by code
	 */
	public static final ReportRenderFormat valueOf(Integer code)
	{
		return lookupTable.containsKey(code) ? lookupTable.get(code) : NONE;
	}

	/**
	 * @return the extension
	 */
	public String getExtension()
	{
		return extension;
	}
}
